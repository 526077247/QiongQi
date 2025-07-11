#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "UObject/NoExportTypes.h"
#include "ResourceManager.generated.h"


// 资源信息结构
struct FResourceInfo
{
    TSoftObjectPtr<UObject> ResourcePtr;
    int32 RefCount = 0;
    int32 State = 0;//1 loading 2suc 3fail
    TSharedPtr<FStreamableHandle> LoadingHandle;
};

UCLASS()
class QIONGQI_API UResourceManager : public UObject
{
    GENERATED_BODY()

public:

    // 获取单例实例
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    static UResourceManager* GetInstance();

    // 异步加载资源
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    void LoadResourceAsync(const FString& AssetPath);

    // 获取加载状态
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    int32 GetLoadingState(const FString& AssetPath);

    // 获取已加载资源
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    UObject* GetLoadedResource(const FString& AssetPath);

    // 释放资源
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    void ReleaseResource(UObject* Asset);

private:

    // 资源加载完成回调
    void OnResourceLoaded(const FString& AssetPath);

    // 内部资源映射表
    TMap<FSoftObjectPath, FResourceInfo> ResourceMap;

    // 资源路径
    TMap<UObject*, FSoftObjectPath> ResourcePath;

    // 流式资源管理器
    FStreamableManager StreamableManager;

    // 单例实例
    static UResourceManager* Instance;
};