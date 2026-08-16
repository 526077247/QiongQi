// Copyright (c) 2026 QiongQi. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UeDownloadHelper.generated.h"

/** 下载完成回调：bSuccess 是否成功，FilePath 目标文件路径 */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnDownloadCompleted, bool, bSuccess, const FString&, FilePath);

/** 下载进度回调：Progress01 总进度 0~1（含续传量），BytesReceived 已接收字节，TotalBytes 总字节 */
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FQOnDownloadProgress, float, Progress01, int64, BytesReceived, int64, TotalBytes);

/**
 * CDN 资源下载桥（TS 经 $Delegate 调用，风格对齐 UeHttpHelper）。
 *
 * 能力：
 *  - 断点续传：下载前检查本地临时文件大小，构造 Range 请求剩余部分并追加写入（文件级 md5 跳过由 TS 层负责）
 *  - 后台下载：基于 FHttpModule 异步完成，不阻塞游戏线程
 *  - pak 挂载：调用 HotPatcher 的 UFlibPakHelper::MountPak，高 PakOrder 保证 CDN 新资源优先
 *  - 本地版本记录：Saved/Paks/version.json 读写
 */
UCLASS()
class QIONGQI_API UUeDownloadHelper : public UObject
{
	GENERATED_BODY()

public:
	/** 获取单例（AddToRoot 防 GC） */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	static UUeDownloadHelper* GetInstance();

	/**
	 * 下载文件到 Saved/Paks/{SaveName}（支持断点续传）。
	 * 若本地已存在部分文件，将构造 Range: bytes={本地大小}- 请求剩余部分并追加写入；
	 * 服务器不支持 Range（返回 200 全量）时自动从头覆盖写入；本地大小与远端一致时直接成功。
	 * @param Url            文件地址
	 * @param SaveName       保存文件名（含扩展名），保存于 Saved/Paks/
	 * @param TimeoutSeconds 请求超时（秒）
	 * @param OnCompleted    完成回调（游戏线程）
	 * @param OnProgress     进度回调（游戏线程，Progress01 为含续传量的总进度）
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	void DownloadFile(const FString& Url, const FString& SaveName, float TimeoutSeconds,
		FOnDownloadCompleted OnCompleted, FQOnDownloadProgress OnProgress);

	/** 计算文件 MD5（分块读取，大文件安全），文件不存在或计算失败返回空串 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	FString CalcFileMd5(const FString& FilePath);

	/** 运行时挂载 pak（UFlibPakHelper::MountPak），PakOrder 越大优先级越高（CDN 新资源优先） */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	bool MountPak(const FString& PakPath, int32 PakOrder);

	/** 读取本地已更新版本号（Saved/Paks/version.json 的 version 字段，纯数字），无记录返回 -1 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	int64 GetLocalVersion();

	/** 保存本地版本 json（Saved/Paks/version.json） */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	bool SaveLocalVersion(const FString& VersionJson);

	/**
	 * 包内版本号与本地版本对齐（游戏初始化时调用）：
	 * 读取随包固化的包内版本号（Config/DefaultGame.ini 的 [QiongQi] ResourceVersion），
	 * 与 GetLocalVersion() 对比，保留较大值按 TS saveLocalVersion 同款 JSON 写回本地版本记录。
	 * 防止包内版本号高于本地记录（如本地版本缺失/被清/回退）时重复下载已随包的内容。
	 * @return 本地版本被提升并写回成功返回 true；包内无版本号、本地已不低于包内或写失败返回 false。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	bool SyncLocalVersionFromPackage();

	/** CDN 资源本地根目录（Saved/Paks/，不存在自动创建） */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	FString GetCdnPakDir();

	/**
	 * 挂载本地已下载的 CDN pak（Saved/Paks/*.pak）。
	 * 遍历按文件名排序保证顺序稳定，逐个以 PakOrder=100+i 挂载（高于首包默认 0），
	 * 使二次启动时文件系统层即命中最新资源（含新 Code）。失败不中断。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	void MountLocalCdnPaks();

	/** CDN 对外平台名（Windows→pc、Android→android、IOS→ios），与打包端 CDN 目录 {渠道}_{平台} 及版本清单 platform 字段一致 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	FString GetPlatformName();

	/**
	 * 当前渠道（读取 Config/DefaultGame.ini 的 [QiongQi] ChannelName，由打包面板整包前写入）。
	 * 无渠道或读取失败时回退 "Default"，与面板默认渠道保持一致。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	FString GetChannel();

	/**
	 * 当前包是否为"全量资源打入首包"模式（读取 Config/DefaultGame.ini 的 [QiongQi] FullInFirstPak，
	 * 由打包面板整包前写入）。全量进首包：首包已含全部资源，本地版本对齐包内版本后无需再拉 CDN 累积增量，
	 * TS 更新流程据此在"本地版本已不低于最新版本"时短路跳过下载。
	 * 无配置（旧包/读取失败）时回退 false（按 CDN 模式处理，不短路），避免 CDN 包误短路导致资源缺失。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Download")
	bool IsFullInFirstPak();

private:
	static UUeDownloadHelper* Instance;

	/** 当前活跃下载器（持有引用，防止异步回调期间被析构取消） */
	TSharedPtr<class FRuntimeChunkDownloader> ActiveDownloader;

	/** 单个下载分块大小（字节） */
	static const int64 DownloadChunkSize;
};
