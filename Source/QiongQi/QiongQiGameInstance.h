// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "JsEnv.h"
#include "Tickable.h"
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "QiongQiGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNotifyUpdate);

/**
 * 
 */
UCLASS()
class QIONGQI_API UQiongQiGameInstance : public UGameInstance, public FTickableGameObject
{
	GENERATED_BODY()

	UPROPERTY()
	FNotifyUpdate NotifyUpdate;
	UPROPERTY()
	float GameDeltaTime;

	virtual void Init() override;
	virtual void OnStart() override;
	virtual void Shutdown() override;

	/**
	 * 重启整个 JS 虚拟机（Puerts JsEnv）。
	 * 热更下载新 Code 后调用：先解绑 Tick 委托，再异步销毁旧 JsEnv 并以相同参数重建、
	 * 重新 Start("Start")，新虚拟机 require 缓存清空、加载已挂载的最新 Code。
	 * 必须异步执行（JS 调用栈 unwind 后），同步销毁会崩溃。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|JsEnv")
	void RestartJsEnv();

	/**
	 * 获取当前网络连接状态（通过 FGenericPlatformMisc::GetNetworkConnectionStatus()）。
	 * 返回 ENetworkConnectionStatus：0=Unknown 1=Disabled 2=Local 3=Connected
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Network")
	int32 GetNetworkConnectionStatus() const;

	/**
	 * 当前是否编辑器环境（GIsEditor：编辑器内运行/PIE 为 true，打包游戏为 false）。
	 * TS 层 Define.IsEditor() 由此获取，替代原先的硬编码常量。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Environment")
	bool IsEditorEnvironment() const;

	/**
	 * 当前包是否为打包面板选择的 Debug 包（读取 DefaultGame.ini 的 [QiongQi] IsDebugPackage，
	 * 由打包面板整包前固化：Debug 包=true，Release 包=false，无配置/旧包回退 false）。
	 * TS 层 Define.Debug 由此获取，用于调试模式（记忆服务器/跳过更新检查等）。
	 */
	UFUNCTION(BlueprintCallable, Category = "QiongQi|Environment")
	bool IsDebugPackage() const;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
private:
	TSharedPtr<puerts::FJsEnv> GameScript;
};
