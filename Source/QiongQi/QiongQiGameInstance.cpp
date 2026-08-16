// Fill out your copyright notice in the Description page of Project Settings.


#include "QiongQiGameInstance.h"
#include "UeDownloadHelper.h"

#include "Async/Async.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"


void UQiongQiGameInstance::Init()
{
	Super::Init();
}

void UQiongQiGameInstance::OnStart()
{
	Super::OnStart();

	// 二次启动：先挂载本地已下载的 CDN pak（PakOrder=100+，高于首包），
	// 保证 JsEnv 加载的是最新 Code，而非首包旧代码
	UUeDownloadHelper::GetInstance()->MountLocalCdnPaks();
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] OnStart: 本地 CDN pak 挂载完成，创建 JsEnv(8080)"));

	// 包内版本号与本地版本对齐：保留较大值写回本地版本记录，
	// 防止本地记录缺失/回退时重复下载已随包的内容
	UUeDownloadHelper::GetInstance()->SyncLocalVersionFromPackage();

	GameScript = MakeShared<puerts::FJsEnv>(std::make_unique<puerts::DefaultJSModuleLoader>(TEXT("JavaScript")), std::make_shared<puerts::FDefaultLogger>(), 8080);
	//GameScript->WaitDebugger();
	TArray<TPair<FString, UObject*>> Arguments;
	Arguments.Add(TPair<FString, UObject*>(TEXT("GameInstance"), this));

	GameScript->Start("Start", Arguments);
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] OnStart: JsEnv.Start(\"Start\") 已调用，启动代码加载完成"));
}

void UQiongQiGameInstance::RestartJsEnv()
{
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] RestartJsEnv: 热更完成，准备重启 JS 虚拟机"));

	// 先解绑旧 JsEnv 的 Tick 委托，防止广播到已销毁对象
	NotifyUpdate.Clear();

	// 必须异步：TS await 链调用时 v8 仍在执行，同步销毁会崩溃；
	// AsyncTask 保证 JS 调用栈 unwind 后执行
	AsyncTask(ENamedThreads::GameThread, [WeakThis = TWeakObjectPtr<UQiongQiGameInstance>(this)]()
	{
		if (!WeakThis.IsValid())
		{
			return;
		}
		UQiongQiGameInstance* Self = WeakThis.Get();

		// 仅回收 UMG Widget UObject，不调用 RemoveAllViewportWidgets（会清除 SGameLayerManager
		// 导致 AddToViewport 失效）。Slate Widget 树由 JS 侧 UIManager.destroyLayer 清理。
		CollectGarbage(RF_NoFlags);
		CollectGarbage(RF_NoFlags);

		// 销毁旧 JsEnv（v8 隔离区，require 缓存清空，所有 JS 均为最新）
		Self->GameScript.Reset();
		UE_LOG(LogTemp, Log, TEXT("[HotUpdate] RestartJsEnv: 旧 JsEnv 已销毁，重建新虚拟机"));

		// 按 OnStart 相同参数重建 JsEnv，新虚拟机加载已挂载的最新 Code
		Self->GameScript = MakeShared<puerts::FJsEnv>(
			std::make_unique<puerts::DefaultJSModuleLoader>(TEXT("JavaScript")),
			std::make_shared<puerts::FDefaultLogger>(), 8080);

		TArray<TPair<FString, UObject*>> Arguments;
		Arguments.Add(TPair<FString, UObject*>(TEXT("GameInstance"), Self));
		Self->GameScript->Start("Start", Arguments);
		UE_LOG(LogTemp, Log, TEXT("[HotUpdate] RestartJsEnv: 新 JsEnv 已 Start(\"Start\")，最新 Code 生效"));
	});
}

void UQiongQiGameInstance::Shutdown()
{
	UE_LOG(LogTemp, Log, TEXT("[HotUpdate] Shutdown: JsEnv 随游戏退出销毁"));
	Super::Shutdown();
}

int32 UQiongQiGameInstance::GetNetworkConnectionStatus() const
{
	return static_cast<int32>(FGenericPlatformMisc::GetNetworkConnectionStatus());
}

bool UQiongQiGameInstance::IsEditorEnvironment() const
{
	return GIsEditor;
}

bool UQiongQiGameInstance::IsDebugPackage() const
{
	// 打包类型由打包面板在整包前固化到 DefaultGame.ini 的 [QiongQi] IsDebugPackage：
	// Debug 包=true，Release 包=false。无配置（旧包）回退 false，与 Release 行为一致。
	int32 IsDebugPackage = 0;
	GConfig->GetInt(TEXT("QiongQi"), TEXT("IsDebugPackage"), IsDebugPackage, GGameIni);
	return IsDebugPackage == 1;
}

void UQiongQiGameInstance::Tick(float DeltaTime)
{
	GameDeltaTime = DeltaTime;
	NotifyUpdate.Broadcast();
}

bool UQiongQiGameInstance::IsTickable() const
{
	return IsValid(GetWorld());
}

TStatId UQiongQiGameInstance::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UQiongQiGameInstance, STATGROUP_Tickables);
}