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

	virtual void Init() override;
	virtual void OnStart() override;
	virtual void Shutdown() override;

protected:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;
private:
	TSharedPtr<puerts::FJsEnv> GameScript;
};
