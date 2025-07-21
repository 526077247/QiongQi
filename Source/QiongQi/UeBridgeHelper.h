// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UeBridgeHelper.generated.h"

/**
 * 
 */
UCLASS()
class QIONGQI_API UUeBridgeHelper : public UObject
{
	GENERATED_BODY()
public:
	// 获取单例实例
	UFUNCTION(BlueprintCallable, Category = "Bridge Helper")
	static UUeBridgeHelper* GetInstance();

	UFUNCTION(BlueprintCallable, Category = "Bridge Helper")
	FVector2f GetGeometryLocalSize(const FGeometry& Geometry);

	UFUNCTION(BlueprintCallable, Category = "Bridge Helper")
	FVector2f GetGeometryDrawSize(const FGeometry& Geometry);
private:
	// 单例实例
	static UUeBridgeHelper* Instance;
};
