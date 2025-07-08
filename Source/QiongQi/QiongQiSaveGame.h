// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "QiongQiSaveGame.generated.h"

/**
 * 
 */
UCLASS()
class QIONGQI_API UQiongQiSaveGame : public USaveGame
{
	GENERATED_BODY()
public:
    UPROPERTY(VisibleAnywhere, Category = "PlayerPrefs")
    TMap<FString, FString> StringData;

    UPROPERTY(VisibleAnywhere, Category = "PlayerPrefs")
    TMap<FString, float> FloatData;

    UPROPERTY(VisibleAnywhere, Category = "PlayerPrefs")
    TMap<FString, int32> IntData;

    UPROPERTY(VisibleAnywhere, Category = "PlayerPrefs")
    TMap<FString, bool> BoolData;

    // ±£´æÎªµ¥Àý
    static const FString SlotName;
};
