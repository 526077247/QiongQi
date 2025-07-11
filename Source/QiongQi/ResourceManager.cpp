#include "ResourceManager.h"
#include "Engine/AssetManager.h"

UResourceManager* UResourceManager::Instance = nullptr;

UResourceManager* UResourceManager::GetInstance()
{
    if (!Instance)
    {
        // �����־û�ʵ��
        Instance = NewObject<UResourceManager>(GetTransientPackage(), UResourceManager::StaticClass());
        Instance->AddToRoot(); // ��ֹ��������
    }
    return Instance;
}

void UResourceManager::LoadResourceAsync(const FString& AssetPath)
{
    // ����Ƿ��Ѵ�����Դ��¼
    if (FResourceInfo* ResourceInfo = ResourceMap.Find(AssetPath))
    {
        return;
    }

    // ��������Դ��¼
    FResourceInfo NewInfo;
    NewInfo.RefCount = 0;
    NewInfo.State = 1;
    // ��ʼ�첽����
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

            // ���ü�������ʱ�ͷ���Դ
            if (ResourceInfo->RefCount <= 0)
            {
                // �ͷż��ؾ��
                if (ResourceInfo->LoadingHandle.IsValid())
                {
                    ResourceInfo->LoadingHandle->ReleaseHandle();
                    ResourceInfo->LoadingHandle.Reset();
                }

                // �ͷ���Դ
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
        // ��ȡ���ص���Դ
        if (ResourceInfo->LoadingHandle.IsValid() && ResourceInfo->LoadingHandle->HasLoadCompleted())
        {
           
            ResourceInfo->ResourcePtr = Cast<UObject>(ResourceInfo->LoadingHandle->GetLoadedAsset());

            // ������Դ��Ч
            if (ResourceInfo->ResourcePtr.IsValid())
            {
                ResourceInfo->State = 2;
                ResourceInfo->ResourcePtr->AddToRoot();
                ResourcePath.Add(ResourceInfo->ResourcePtr.Get(), AssetPath);
            }
        }
        // �ͷż��ؾ����������Դ���ã�
        ResourceInfo->LoadingHandle.Reset();
        if (ResourceInfo->State == 3) 
        {
            UE_LOG(LogTemp, Error, TEXT("Resource Loaded Error! %s"), *AssetPath);
            //����ʧ��
            ResourceMap.Remove(AssetPath);
        }
    }
}