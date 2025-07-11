//#include "ResourceManager.h"
//#include "Engine/AssetManager.h"
//
//UResourceManager* UResourceManager::Instance = nullptr;
//
//UResourceManager* UResourceManager::GetInstance()
//{
//    if (!Instance)
//    {
//        // 创建持久化实例
//        Instance = NewObject<UResourceManager>(GetTransientPackage(), UResourceManager::StaticClass());
//        Instance->AddToRoot(); // 防止垃圾回收
//        Instance->Initialize();
//    }
//    return Instance;
//}
//
//void UResourceManager::Initialize()
//{
//    // 初始化资源组
//    /*ResourceGroups.Add(TEXT("UI"), {
//        FSoftObjectPath(TEXT("/Game/UI/Sprites/MainIcon.MainIcon")),
//        FSoftObjectPath(TEXT("/Game/UI/Sprites/HealthBar.HealthBar"))
//        });
//
//    ResourceGroups.Add(TEXT("Characters"), {
//        FSoftObjectPath(TEXT("/Game/Characters/Player_Sprite.Player_Sprite")),
//        FSoftObjectPath(TEXT("/Game/Characters/Enemy_Sprite.Enemy_Sprite"))
//        });*/
//}
//
//void UResourceManager::LoadResourceAsync(FSoftObjectPath AssetPath, FResourceLoadedDelegate OnLoaded)
//{
//    // 检查是否已存在资源记录
//    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
//    {
//        // 资源已加载或正在加载
//        if (ResourceInfo->ResourcePtr.IsValid())
//        {
//            // 增加引用计数
//            ResourceInfo->RefCount++;
//
//            // 立即执行回调
//            OnLoaded.ExecuteIfBound();
//        }
//        else if (ResourceInfo->LoadingHandle.IsValid())
//        {
//            // 正在加载中，添加新回调
//            ResourceInfo->LoadingHandle->BindCompleteDelegate(FStreamableDelegate::CreateLambda([OnLoaded]() {
//                OnLoaded.ExecuteIfBound();
//                }));
//
//            ResourceInfo->RefCount++;
//        }
//        return;
//    }
//
//    // 创建新资源记录
//    FResourceInfo NewInfo;
//    NewInfo.RefCount = 1;
//
//    // 开始异步加载
//    NewInfo.LoadingHandle = StreamableManager.RequestAsyncLoad(
//        AssetPath,
//        FStreamableDelegate::CreateLambda([this, AssetPath, OnLoaded]()
//        {
//            OnResourceLoaded(AssetPath);
//            OnLoaded.ExecuteIfBound();
//        })
//    );
//
//    // 添加加载完成后的用户回调
//    NewInfo.LoadingHandle->BindCompleteDelegate(FStreamableDelegate::CreateLambda([OnLoaded]() {
//        OnLoaded.ExecuteIfBound();
//        }));
//
//    ResourceMap.Add(AssetPath, NewInfo);
//}
//
//UObject* UResourceManager::GetLoadedResource(FSoftObjectPath AssetPath)
//{
//    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
//    {
//        if (ResourceInfo->ResourcePtr.IsValid())
//        {
//            return ResourceInfo->ResourcePtr.Get();
//        }
//    }
//    return nullptr;
//}
//
//void UResourceManager::ReleaseResource(FSoftObjectPath AssetPath)
//{
//    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
//    {
//        ResourceInfo->RefCount--;
//
//        // 引用计数归零时释放资源
//        if (ResourceInfo->RefCount <= 0)
//        {
//            // 释放加载句柄
//            if (ResourceInfo->LoadingHandle.IsValid())
//            {
//                ResourceInfo->LoadingHandle->ReleaseHandle();
//                ResourceInfo->LoadingHandle.Reset();
//            }
//
//            // 释放资源
//            if (ResourceInfo->ResourcePtr.IsValid())
//            {
//                ResourceInfo->ResourcePtr->ConditionalBeginDestroy();
//            }
//
//            ResourceMap.Remove(AssetPath);
//        }
//    }
//}
//
//void UResourceManager::OnResourceLoaded(FSoftObjectPath AssetPath)
//{
//    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
//    {
//        // 获取加载的资源
//        if (ResourceInfo->LoadingHandle.IsValid() && ResourceInfo->LoadingHandle->HasLoadCompleted())
//        {
//            ResourceInfo->ResourcePtr = Cast<UObject>(ResourceInfo->LoadingHandle->GetLoadedAsset());
//
//            // 保持资源有效
//            if (ResourceInfo->ResourcePtr.IsValid())
//            {
//                ResourceInfo->ResourcePtr->AddToRoot();
//            }
//        }
//
//        // 释放加载句柄（保留资源引用）
//        ResourceInfo->LoadingHandle.Reset();
//    }
//}
//
//void UResourceManager::PreloadResourceGroup(FName GroupName)
//{
//    if (const TArray<FSoftObjectPath>* Resources = ResourceGroups.Find(GroupName))
//    {
//        for (const FSoftObjectPath& Path : *Resources)
//        {
//            LoadResourceAsync(Path, FStreamableDelegate());
//        }
//    }
//}
//
//void UResourceManager::ReleaseResourceGroup(FName GroupName)
//{
//    if (const TArray<FSoftObjectPath>* Resources = ResourceGroups.Find(GroupName))
//    {
//        for (const FSoftObjectPath& Path : *Resources)
//        {
//            ReleaseResource(Path);
//        }
//    }
//}