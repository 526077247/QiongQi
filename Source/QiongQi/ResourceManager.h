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
//// ��Դ��Ϣ�ṹ
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
//    // ��ȡ����ʵ��
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    static UResourceManager* GetInstance();
//
//    // ��ʼ����Դ������
//    void Initialize();
//
//    // �첽������Դ
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    void LoadResourceAsync(FSoftObjectPath AssetPath, FResourceLoadedDelegate OnLoaded);
//
//    // ��ȡ�Ѽ�����Դ
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    UObject* GetLoadedResource(FSoftObjectPath AssetPath);
//
//    // �ͷ���Դ
//    UFUNCTION(BlueprintCallable, Category = "Resource Manager")
//    void ReleaseResource(FSoftObjectPath AssetPath);
//
//    // Ԥ������Դ��
//    void PreloadResourceGroup(FName GroupName);
//
//    // �ͷ���Դ��
//    void ReleaseResourceGroup(FName GroupName);
//
//private:
//    // ˽�й��캯��
//    UResourceManager();
//
//    // ��Դ������ɻص�
//    void OnResourceLoaded(FSoftObjectPath AssetPath);
//
//    // �ڲ���Դӳ���
//    TMap<FSoftObjectPath, FResourceInfo> ResourceMap;
//
//    // ��Դ��ӳ��
//    TMap<FName, TArray<FSoftObjectPath>> ResourceGroups;
//
//    // ��ʽ��Դ������
//    FStreamableManager StreamableManager;
//
//    // ����ʵ��
//    static UResourceManager* Instance;
//};