// Fill out your copyright notice in the Description page of Project Settings.


#include "QiongQiGameInstance.h"


void UQiongQiGameInstance::Init()
{
	Super::Init();
}

void UQiongQiGameInstance::OnStart()
{
	Super::OnStart();
	GameScript = MakeShared<puerts::FJsEnv>(std::make_unique<puerts::DefaultJSModuleLoader>(TEXT("JavaScript")), std::make_shared<puerts::FDefaultLogger>(), 8080);
	//GameScript->WaitDebugger();
	TArray<TPair<FString, UObject*>> Arguments;
	Arguments.Add(TPair<FString, UObject*>(TEXT("GameInstance"), this));
    
	GameScript->Start("Init", Arguments);
}

void UQiongQiGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UQiongQiGameInstance::Tick(float DeltaTime)
{
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