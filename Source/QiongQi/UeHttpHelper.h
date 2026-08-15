// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "UeHttpHelper.generated.h"

class UTexture2D;

// 动态委托：HTTP 请求结果回调
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnHttpResult, bool, bSuccess, int32, StatusCode, FString, ResponseText);

// 动态委托：HTTP 图片下载结果回调（返回解码后的纹理，失败时 Texture 为 nullptr）
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FOnHttpImageResult, bool, bSuccess, int32, StatusCode, UTexture2D*, Texture);

/**
 * HTTP 请求辅助类：基于 UE FHttpModule 封装异步 GET/POST 请求，
 * 供 puerts 的 TypeScript 侧调用（替代浏览器 XMLHttpRequest）。
 */
UCLASS()
class QIONGQI_API UUeHttpHelper : public UObject
{
	GENERATED_BODY()
public:
	// 获取单例实例
	UFUNCTION(BlueprintCallable, Category = "Http Helper")
	static UUeHttpHelper* GetInstance();

	// 发送 GET 请求
	UFUNCTION(BlueprintCallable, Category = "Http Helper")
	void HttpGet(const FString& Url, const TMap<FString, FString>& Headers, float TimeoutSeconds, FOnHttpResult OnResult);

	// 发送 POST 请求
	UFUNCTION(BlueprintCallable, Category = "Http Helper")
	void HttpPost(const FString& Url, const TMap<FString, FString>& Headers, const FString& Body, float TimeoutSeconds, FOnHttpResult OnResult);

	// 下载网络图片并解码为 Texture2D（异步）。CachePath 非空时，下载成功后将原始字节写入本地缓存文件
	UFUNCTION(BlueprintCallable, Category = "Http Helper")
	void HttpGetImage(const FString& Url, const TMap<FString, FString>& Headers, float TimeoutSeconds, const FString& CachePath, FOnHttpImageResult OnResult);

	// 从本地文件读取图片并解码为 Texture2D（同步）
	UFUNCTION(BlueprintCallable, Category = "Http Helper")
	void LoadImageFromLocalFile(const FString& FilePath, FOnHttpImageResult OnResult);

	// 获取图片本地缓存根目录（不存在会自动创建）
	UFUNCTION(BlueprintCallable, Category = "Http Helper")
	FString GetImageCacheDir();

private:
	// 内部通用请求发送逻辑
	void SendRequest(const FString& Verb, const FString& Url, const TMap<FString, FString>& Headers, const FString& Body, float TimeoutSeconds, FOnHttpResult OnResult);

	// 单例实例
	static UUeHttpHelper* Instance;
};
