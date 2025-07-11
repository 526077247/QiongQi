//#pragma once
//
//#include "CoreMinimal.h"
//#include "Engine/StreamableManager.h"
//#include "UObject/SoftObjectPtr.h"
//#include "ResourceManager.generated.h"
//
//
//DECLARE_DELEGATE(FResourceLoadedDelegate);
//
//// 资源信息结构
//struct FResourceInfo
//{
//    TSoftObjectPtr<UObject> ResourcePtr;
//    int32 RefCount = 0;
//    TSharedPtr<FStreamableHandle> LoadingHandle;
//};
//
//UCLASS()
//class QIONGQI_API UResourceManager : public UObject
//{
//    GENERATED_BODY()
//
//public:
//    // 获取单例实例
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    static UResourceManager* GetInstance();
//
//    // 初始化资源管理器
//    void Initialize();
//
//    // 异步加载资源
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    void LoadResourceAsync(FSoftObjectPath AssetPath, FResourceLoadedDelegate OnLoaded);
//
//    // 获取已加载资源
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    UObject* GetLoadedResource(FSoftObjectPath AssetPath);
//
//    // 释放资源
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    void ReleaseResource(FSoftObjectPath AssetPath);
//
//    // 预加载资源组
//    void PreloadResourceGroup(FName GroupName);
//
//    // 释放资源组
//    void ReleaseResourceGroup(FName GroupName);
//
//private:
//    // 私有构造函数
//    UResourceManager();
//
//    // 资源加载完成回调
//    void OnResourceLoaded(FSoftObjectPath AssetPath);
//
//    // 内部资源映射表
//    TMap<FSoftObjectPath, FResourceInfo> ResourceMap;
//
//    // 资源组映射
//    TMap<FName, TArray<FSoftObjectPath>> ResourceGroups;
//
//    // 流式资源管理器
//    FStreamableManager StreamableManager;
//
//    // 单例实例
//    static UResourceManager* Instance;
//};