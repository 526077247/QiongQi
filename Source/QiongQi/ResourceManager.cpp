//#include "ResourceManager.h"
//#include "Engine/AssetManager.h"
//
//UResourceManager* UResourceManager::Instance = nullptr;
//
//UResourceManager* UResourceManager::GetInstance()
//{
//    if (!Instance)
//    {
//        // �����־û�ʵ��
//        Instance = NewObject<UResourceManager>(GetTransientPackage(), UResourceManager::StaticClass());
//        Instance->AddToRoot(); // ��ֹ��������
//        Instance->Initialize();
//    }
//    return Instance;
//}
//
//void UResourceManager::Initialize()
//{
//    // ��ʼ����Դ��
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
//    // ����Ƿ��Ѵ�����Դ��¼
//    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
//    {
//        // ��Դ�Ѽ��ػ����ڼ���
//        if (ResourceInfo->ResourcePtr.IsValid())
//        {
//            // �������ü���
//            ResourceInfo->RefCount++;
//
//            // ����ִ�лص�
//            OnLoaded.ExecuteIfBound();
//        }
//        else if (ResourceInfo->LoadingHandle.IsValid())
//        {
//            // ���ڼ����У�����»ص�
//            ResourceInfo->LoadingHandle->BindCompleteDelegate(FStreamableDelegate::CreateLambda([OnLoaded]() {
//                OnLoaded.ExecuteIfBound();
//                }));
//
//            ResourceInfo->RefCount++;
//        }
//        return;
//    }
//
//    // ��������Դ��¼
//    FResourceInfo NewInfo;
//    NewInfo.RefCount = 1;
//
//    // ��ʼ�첽����
//    NewInfo.LoadingHandle = StreamableManager.RequestAsyncLoad(
//        AssetPath,
//        FStreamableDelegate::CreateLambda([this, AssetPath, OnLoaded]()
//        {
//            OnResourceLoaded(AssetPath);
//            OnLoaded.ExecuteIfBound();
//        })
//    );
//
//    // ��Ӽ�����ɺ���û��ص�
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
//        // ���ü�������ʱ�ͷ���Դ
//        if (ResourceInfo->RefCount <= 0)
//        {
//            // �ͷż��ؾ��
//            if (ResourceInfo->LoadingHandle.IsValid())
//            {
//                ResourceInfo->LoadingHandle->ReleaseHandle();
//                ResourceInfo->LoadingHandle.Reset();
//            }
//
//            // �ͷ���Դ
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
//        // ��ȡ���ص���Դ
//        if (ResourceInfo->LoadingHandle.IsValid() && ResourceInfo->LoadingHandle->HasLoadCompleted())
//        {
//            ResourceInfo->ResourcePtr = Cast<UObject>(ResourceInfo->LoadingHandle->GetLoadedAsset());
//
//            // ������Դ��Ч
//            if (ResourceInfo->ResourcePtr.IsValid())
//            {
//                ResourceInfo->ResourcePtr->AddToRoot();
//            }
//        }
//
//        // �ͷż��ؾ����������Դ���ã�
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