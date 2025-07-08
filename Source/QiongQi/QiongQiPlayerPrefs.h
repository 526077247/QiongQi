// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "QiongQiSaveGame.h"
#include "Binding.hpp"
#include "QiongQiPlayerPrefs.generated.h"

/**
 * 
 */
UCLASS()
class QIONGQI_API UQiongQiPlayerPrefs : public UObject
{
	GENERATED_BODY()
public:

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static void SetString(const FString& Key, const FString& Value);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static void SetFloat(const FString& Key, float Value);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static void SetInt(const FString& Key, int32 Value);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static void SetBool(const FString& Key, bool Value);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static FString GetString(const FString& Key, const FString& DefaultValue = "");

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static float GetFloat(const FString& Key, float DefaultValue = 0.0f);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static int32 GetInt(const FString& Key, int32 DefaultValue = 0);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static bool GetBool(const FString& Key, bool DefaultValue = false);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static void SaveData();

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static void Delete(const FString& Key);

    UFUNCTION(BlueprintCallable, Category = "PlayerPrefs")
    static UQiongQiSaveGame* LoadSaveGame();
private:
    static UQiongQiSaveGame* data;
};
