#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "UObject/NoExportTypes.h"
#include "ResourceManager.generated.h"


// ��Դ��Ϣ�ṹ
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

    // ��ȡ����ʵ��
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    static UResourceManager* GetInstance();

    // �첽������Դ
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    void LoadResourceAsync(const FString& AssetPath);

    // ��ȡ����״̬
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    int32 GetLoadingState(const FString& AssetPath);

    // ��ȡ�Ѽ�����Դ
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    UObject* GetLoadedResource(const FString& AssetPath);

    // �ͷ���Դ
    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
    void ReleaseResource(UObject* Asset);

private:

    // ��Դ������ɻص�
    void OnResourceLoaded(const FString& AssetPath);

    // �ڲ���Դӳ���
    TMap<FSoftObjectPath, FResourceInfo> ResourceMap;

    // ��Դ·��
    TMap<UObject*, FSoftObjectPath> ResourcePath;

    // ��ʽ��Դ������
    FStreamableManager StreamableManager;

    // ����ʵ��
    static UResourceManager* Instance;
};