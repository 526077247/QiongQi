#include "ResourceManager.h"
#include "Engine/AssetManager.h"

UResourceManager* UResourceManager::Instance = nullptr;

UResourceManager* UResourceManager::GetInstance()
{
    if (!Instance)
    {
        // 创建持久化实例
        Instance = NewObject<UResourceManager>(GetTransientPackage(), UResourceManager::StaticClass());
        Instance->AddToRoot(); // 防止垃圾回收
    }
    return Instance;
}

void UResourceManager::LoadResourceAsync(const FString& AssetPath)
{
    // 检查是否已存在资源记录
    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
    {
        return;
    }

    // 创建新资源记录
    FResourceInfo NewInfo;
    NewInfo.RefCount = 0;
    NewInfo.State = 1;
    // 开始异步加载
    NewInfo.LoadingHandle = StreamableManager.RequestAsyncLoad(
        AssetPath,
        FStreamableDelegate::CreateLambda([this, AssetPath]()
        {
            OnResourceLoaded(AssetPath);
        })
    );

    ResourceMap.Add(AssetPath, NewInfo);
}

UObject* UResourceManager::GetLoadedResource(const FString& AssetPath)
{
    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
    {
        if (ResourceInfo->ResourcePtr.IsValid())
        {
            ResourceInfo->RefCount++;
            return ResourceInfo->ResourcePtr.Get();
        }
    }
    return nullptr;
}

int32 UResourceManager::GetLoadingState(const FString& AssetPath)
{
    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
    {
        return ResourceInfo->State;
    }
    return 0;
}

void UResourceManager::ReleaseResource(UObject* Asset)
{
    if (FSoftObjectPath* AssetPath = ResourcePath.Find(Asset)) 
    {
        if (FResourceInfo* ResourceInfo = ResourceMap.Find(*AssetPath))
        {
            ResourceInfo->RefCount--;

            // 引用计数归零时释放资源
            if (ResourceInfo->RefCount <= 0)
            {
                // 释放加载句柄
                if (ResourceInfo->LoadingHandle.IsValid())
                {
                    ResourceInfo->LoadingHandle->ReleaseHandle();
                    ResourceInfo->LoadingHandle.Reset();
                }

                // 释放资源
                if (ResourceInfo->ResourcePtr.IsValid())
                {
                    ResourceInfo->ResourcePtr->ConditionalBeginDestroy();
                }
                ResourcePath.Remove(Asset);
                ResourceMap.Remove(*AssetPath);
            }
        }
    }
}

void UResourceManager::OnResourceLoaded(const FString& AssetPath)
{
    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
    {
        ResourceInfo->State = 3;
        // 获取加载的资源
        if (ResourceInfo->LoadingHandle.IsValid() && ResourceInfo->LoadingHandle->HasLoadCompleted())
        {
           
            ResourceInfo->ResourcePtr = Cast<UObject>(ResourceInfo->LoadingHandle->GetLoadedAsset());

            // 保持资源有效
            if (ResourceInfo->ResourcePtr.IsValid())
            {
                ResourceInfo->State = 2;
                ResourceInfo->ResourcePtr->AddToRoot();
                ResourcePath.Add(ResourceInfo->ResourcePtr.Get(), AssetPath);
            }
        }
        // 释放加载句柄（保留资源引用）
        ResourceInfo->LoadingHandle.Reset();
        if (ResourceInfo->State == 3) 
        {
            UE_LOG(LogTemp, Error, TEXT("Resource Loaded Error! %s"), *AssetPath);
            //加载失败
            ResourceMap.Remove(AssetPath);
        }
    }
}