// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Binding.hpp"
#include "QiongQiConfigLoader.generated.h"

/**
 * 配置表 JSON 加载辅助类：枚举并读取 Content/AssetsPackage/Config 目录下的配置 JSON。
 * 走 UE 虚拟文件系统（VFS），编辑器直接读 Content，打包后穿透 pak 读取，行为一致。
 */
UCLASS()
class QIONGQI_API UQiongQiConfigLoader : public UObject
{
	GENERATED_BODY()
public:
	// 返回 Config 目录下所有 .json 文件的名称（不含扩展名，如 ServerConfigCategory），用 "|" 分隔
	// 注意：不能直接返回 TArray<FString>，puer-ts 1.0.5 反射模式下静态函数返回容器会转成 {}，
	// 因此这里改为返回 FString，由 JS 侧 split。
	UFUNCTION(BlueprintCallable, Category = "ConfigLoader")
	static FString GetConfigJsonFileNames();

	// 按文件名读取 JSON 内容（FileName 不含扩展名，文件不存在返回空串）
	UFUNCTION(BlueprintCallable, Category = "ConfigLoader")
	static FString LoadConfigJson(const FString& FileName);
};
