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
	// ��ȡ����ʵ��
	UFUNCTION(BlueprintCallable, Category = "Bridge Helper")
	static UUeBridgeHelper* GetInstance();

	UFUNCTION(BlueprintCallable, Category = "Bridge Helper")
	FVector2f GetGeometryLocalSize(const FGeometry& Geometry);

	UFUNCTION(BlueprintCallable, Category = "Bridge Helper")
	FVector2f GetGeometryDrawSize(const FGeometry& Geometry);
private:
	// ����ʵ��
	static UUeBridgeHelper* Instance;
};
