// Copyright (c) 2026 QiongQi. All rights reserved.

#include "UeDownloadHelper.h"

#include "RuntimeChunkDownloader.h"
#include "FileToMemoryDownloader.h"
#include "BaseTypes/FlibPakHelper.h"

#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"

UUeDownloadHelper* UUeDownloadHelper::Instance = nullptr;
const int64 UUeDownloadHelper::DownloadChunkSize = 2 * 1024 * 1024; // 2MB 分块

UUeDownloadHelper* UUeDownloadHelper::GetInstance()
{
	if (!Instance)
	{
		Instance = NewObject<UUeDownloadHelper>();
		Instance->AddToRoot();
	}
	return Instance;
}

FString UUeDownloadHelper::GetCdnPakDir()
{
	const FString Dir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Paks"));
	IFileManager::Get().MakeDirectory(*Dir, true);
	return Dir;
}

void UUeDownloadHelper::MountLocalCdnPaks()
{
	const FString PakDir = GetCdnPakDir();
	const FString Wildcard = FPaths::Combine(PakDir, TEXT("*.pak"));

	TArray<FString> PakFiles;
	IFileManager::Get().FindFiles(PakFiles, *Wildcard, true, false);

	// 按文件名排序保证挂载顺序稳定
	PakFiles.Sort();

	// 与 TS UpdateSetting.CDN_PAK_ORDER=100 对齐，高于首包默认 0，保证 CDN 新资源优先
	const int32 BaseOrder = 100;
	int32 MountedCount = 0;
	for (int32 i = 0; i < PakFiles.Num(); ++i)
	{
		const FString PakPath = FPaths::Combine(PakDir, PakFiles[i]);
		const int32 PakOrder = BaseOrder + i;
		if (MountPak(PakPath, PakOrder))
		{
			++MountedCount;
			UE_LOG(LogTemp, Log, TEXT("[UeDownloadHelper] 已挂载本地 CDN pak: %s (PakOrder=%d)"), *PakPath, PakOrder);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[UeDownloadHelper] 挂载本地 CDN pak 失败: %s (PakOrder=%d)"), *PakPath, PakOrder);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[UeDownloadHelper] MountLocalCdnPaks 完成：共 %d 个 pak，成功 %d 个"), PakFiles.Num(), MountedCount);
}

void UUeDownloadHelper::DownloadFile(const FString& Url, const FString& SaveName, float TimeoutSeconds,
	FOnDownloadCompleted OnCompleted, FQOnDownloadProgress OnProgress)
{
	// 回调转发到游戏线程（Http/Future 回调可能不在游戏线程）
	auto FireCompleted = [OnCompleted](bool bSuccess, const FString& InPath)
	{
		if (IsInGameThread())
		{
			OnCompleted.ExecuteIfBound(bSuccess, InPath);
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [OnCompleted, bSuccess, InPath]()
			{
				OnCompleted.ExecuteIfBound(bSuccess, InPath);
			});
		}
	};
	auto FireProgress = [OnProgress](float Progress01, int64 BytesReceived, int64 TotalBytes)
	{
		if (IsInGameThread())
		{
			OnProgress.ExecuteIfBound(Progress01, BytesReceived, TotalBytes);
		}
		else
		{
			AsyncTask(ENamedThreads::GameThread, [OnProgress, Progress01, BytesReceived, TotalBytes]()
			{
				OnProgress.ExecuteIfBound(Progress01, BytesReceived, TotalBytes);
			});
		}
	};

	const FString SavePath = FPaths::Combine(GetCdnPakDir(), SaveName);
	const int64 ExistingSize = FPaths::FileExists(SavePath) ? IFileManager::Get().FileSize(*SavePath) : 0;
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] DownloadFile: %s -> %s (本地已存在 %lld 字节)"), *Url, *SavePath, ExistingSize);

	// 取消上一个未完成的下载，持有当前下载器引用防析构
	if (ActiveDownloader.IsValid())
	{
		ActiveDownloader->CancelDownload();
	}
	ActiveDownloader = MakeShareable(new FRuntimeChunkDownloader());
	const TSharedPtr<FRuntimeChunkDownloader> Downloader = ActiveDownloader;

	// 1. HEAD 获取远端文件总大小
	Downloader->GetContentSize(Url, TimeoutSeconds)
		.Next([this, Downloader, SavePath, ExistingSize, Url, TimeoutSeconds, FireCompleted, FireProgress](int64 ContentSize) mutable
		{
			UE_LOG(LogTemp, Log, TEXT("[HotUpdate] DownloadFile: 远端大小 %lld 字节"), ContentSize);
			if (ContentSize <= 0)
			{
				UE_LOG(LogTemp, Warning, TEXT("[HotUpdate] DownloadFile: 远端大小无效(%lld)，下载失败"), ContentSize);
				FireCompleted(false, SavePath);
				return;
			}

			// 本地文件大小与远端一致（或更大）时视为已完整，无需下载（文件级断点）
			if (ExistingSize >= ContentSize)
			{
				UE_LOG(LogTemp, Log, TEXT("[HotUpdate] DownloadFile: 本地文件已完整，跳过下载"));
				FireCompleted(true, SavePath);
				return;
			}

			// 2. 打开追加写句柄（bAppend=true 定位到文件末尾，实现字节级续传）
			IFileHandle* FileHandle = FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*SavePath, true);
			if (!FileHandle)
			{
				FireCompleted(false, SavePath);
				return;
			}

			const int64 Start = ExistingSize;
			const int64 End = FMath::Min(Start + DownloadChunkSize, ContentSize) - 1;

			// 3. 从本地已下载位置开始分块下载，逐块追加写入
			Downloader->DownloadFilePerChunk(Url, TimeoutSeconds, TEXT("application/octet-stream"), DownloadChunkSize,
				FInt64Vector2(Start, End),
				[FireProgress, ExistingSize, ContentSize](int64 BytesReceived, int64 /*TotalSize*/)
				{
					const int64 ReceivedTotal = ExistingSize + BytesReceived;
					const float Progress01 = FMath::Clamp(static_cast<float>(ReceivedTotal) / static_cast<float>(ContentSize), 0.f, 1.f);
					FireProgress(Progress01, ReceivedTotal, ContentSize);
				},
				[FileHandle](TArray64<uint8>&& ChunkData)
				{
					if (FileHandle && ChunkData.Num() > 0)
					{
						FileHandle->Write(ChunkData.GetData(), ChunkData.Num());
					}
				})
				.Next([this, FileHandle, SavePath, FireCompleted](EDownloadToMemoryResult Result) mutable
				{
					if (FileHandle)
					{
						delete FileHandle;
						FileHandle = nullptr;
					}
					const bool bSuccess = (Result == EDownloadToMemoryResult::Success || Result == EDownloadToMemoryResult::SucceededByPayload);
					UE_LOG(LogTemp, Log, TEXT("[HotUpdate] DownloadFile: 下载完成(%s): %s"), bSuccess ? TEXT("成功") : TEXT("失败"), *SavePath);
					FireCompleted(bSuccess, SavePath);
				});
		});
}

FString UUeDownloadHelper::CalcFileMd5(const FString& FilePath)
{
	if (!FPaths::FileExists(FilePath))
	{
		return FString();
	}

	IFileHandle* Handle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*FilePath);
	if (!Handle)
	{
		return FString();
	}

	FMD5 Md5;
	const int64 ChunkSize = 1 << 20; // 1MB 分块
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(ChunkSize);

	int64 Remaining = Handle->Size();
	while (Remaining > 0)
	{
		const int64 ToRead = FMath::Min<int64>(ChunkSize, Remaining);
		if (!Handle->Read(Buffer.GetData(), ToRead))
		{
			delete Handle;
			return FString();
		}
		Md5.Update(Buffer.GetData(), ToRead);
		Remaining -= ToRead;
	}
	delete Handle;

	uint8 Digest[16];
	Md5.Final(Digest);
	return BytesToHex(Digest, 16);
}

bool UUeDownloadHelper::MountPak(const FString& PakPath, int32 PakOrder)
{
	if (!FPaths::FileExists(PakPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("[HotUpdate] MountPak: 文件不存在: %s"), *PakPath);
		return false;
	}
	const bool bMounted = UFlibPakHelper::MountPak(PakPath, PakOrder);
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] MountPak: %s (PakOrder=%d) %s"), *PakPath, PakOrder, bMounted ? TEXT("成功") : TEXT("失败"));
	return bMounted;
}

FString UUeDownloadHelper::GetPlatformName()
{
	// CDN 对外平台名（Windows→pc、Android→android、IOS→ios）：
	// 与打包端 Release/CDN 目录 {渠道}_{平台} 及版本清单 platform 字段保持一致，
	// 仅用于拼接 CDN URL（{channel}_{platform}/{版本号}.json、update_{platform}.list）。
	const FString UePlatform = FPlatformProperties::PlatformName();
	if (UePlatform == TEXT("Android"))
	{
		return TEXT("android");
	}
	if (UePlatform == TEXT("IOS"))
	{
		return TEXT("ios");
	}
	return TEXT("pc");
}

FString UUeDownloadHelper::GetChannel()
{
	FString Channel;
	// 渠道由打包面板在整包前写入 DefaultGame.ini（[QiongQi] ChannelName）
	GConfig->GetString(TEXT("QiongQi"), TEXT("ChannelName"), Channel, GGameIni);
	if (Channel.IsEmpty())
	{
		Channel = TEXT("Default");
	}
	return Channel;
}

bool UUeDownloadHelper::IsFullInFirstPak()
{
	// 标志由打包面板在整包前写入 DefaultGame.ini（[QiongQi] FullInFirstPak）
	int32 FullInFirstPak = 0;
	GConfig->GetInt(TEXT("QiongQi"), TEXT("FullInFirstPak"), FullInFirstPak, GGameIni);
	// 无配置（旧包/读取失败）回退 false（按 CDN 模式处理，不短路），仅显式写入 1 时视为全量进首包
	const bool bFull = (FullInFirstPak == 1);
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] IsFullInFirstPak: %s"), bFull ? TEXT("是") : TEXT("否"));
	return bFull;
}

int64 UUeDownloadHelper::GetLocalVersion()
{
	const FString VersionPath = FPaths::Combine(GetCdnPakDir(), TEXT("version.json"));
	FString JsonStr;
	if (!FPaths::FileExists(VersionPath) || !FFileHelper::LoadFileToString(JsonStr, *VersionPath))
	{
		return -1;
	}

	TSharedPtr<FJsonObject> Root;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
	if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
	{
		return -1;
	}
	const int64 Version = (int64)Root->GetNumberField(TEXT("version"));
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] GetLocalVersion: 本地版本 %lld (%s)"), Version, *VersionPath);
	return Version;
}

bool UUeDownloadHelper::SaveLocalVersion(const FString& VersionJson)
{
	const FString VersionPath = FPaths::Combine(GetCdnPakDir(), TEXT("version.json"));
	const bool bSaved = FFileHelper::SaveStringToFile(VersionJson, *VersionPath);
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] SaveLocalVersion: %s (%s)"), bSaved ? TEXT("成功") : TEXT("失败"), *VersionJson);
	return bSaved;
}

bool UUeDownloadHelper::SyncLocalVersionFromPackage()
{
	// 包内版本号：打包面板整包前写入 Config/DefaultGame.ini（[QiongQi] ResourceVersion），随包固化
	FString PackageVersionStr;
	GConfig->GetString(TEXT("QiongQi"), TEXT("ResourceVersion"), PackageVersionStr, GGameIni);
	if (PackageVersionStr.IsEmpty() || !PackageVersionStr.IsNumeric())
	{
		UE_LOG(LogTemp, Log, TEXT("[HotUpdate] SyncLocalVersionFromPackage: 包内无有效版本号(%s)，跳过对齐"), *PackageVersionStr);
		return false;
	}
	const int64 PackageVersion = FCString::Atoi64(*PackageVersionStr);

	const int64 LocalVersion = GetLocalVersion();
	if (LocalVersion >= PackageVersion)
	{
		UE_LOG(LogTemp, Log, TEXT("[HotUpdate] SyncLocalVersionFromPackage: 本地版本 %lld >= 包内版本 %lld，无需提升"), LocalVersion, PackageVersion);
		return false;
	}

	// 与 TS ServerConfigManager.saveLocalVersion 同款 JSON 结构，保证后续 GetLocalVersion 可读
	const FString Manifest = FString::Printf(
		TEXT("{\"channel\":\"%s\",\"platform\":\"%s\",\"version\":%lld,\"files\":[]}"),
		*GetChannel(), *GetPlatformName(), PackageVersion);
	const bool bSaved = SaveLocalVersion(Manifest);
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] SyncLocalVersionFromPackage: 本地版本 %lld -> 包内版本 %lld 已提升并写回 (%s)"),
		LocalVersion, PackageVersion, bSaved ? TEXT("成功") : TEXT("失败"));
	return bSaved;
}
