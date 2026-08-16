#include "AtlasGroupHelper.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PaperSprite.h"
#include "PaperSpriteAtlas.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "AtlasGroupHelper"

void FAtlasGroupHelper::RunAll()
{
    const TArray<FString> AtlasFolders = FindAllAtlasFolders();

    int32 AtlasCount = 0;
    int32 SpriteTotal = 0;

    for (const FString& AtlasFolder : AtlasFolders)
    {
        // AtlasFolder 形如 /Game/AssetsPackage/UI/UICommon/Atlas，取父目录作为图集所在目录
        const FString ParentPath = FPaths::GetPath(AtlasFolder);
        UPaperSpriteAtlas* Atlas = EnsureAtlasAsset(ParentPath);
        if (Atlas == nullptr)
        {
            UE_LOG(LogTemp, Warning, TEXT("[AtlasGroupHelper] 无法创建/加载图集: %s"), *ParentPath);
            continue;
        }

        ++AtlasCount;
        SpriteTotal += SetAtlasGroupForFolder(AtlasFolder, Atlas);
        // 图集重建（PostEditChangeProperty 触发）会修改图集包（BuiltWidth/AtlasSlots/纹理），需保存
        SavePackage(Atlas->GetOutermost());
    }

    const FString Msg = FString::Printf(
        TEXT("一键设置 AtlasGroup 完成：\n共处理 %d 个 Atlas 目录，设置 %d 个 Sprite。"),
        AtlasCount, SpriteTotal);
    FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(Msg));
    UE_LOG(LogTemp, Log, TEXT("[AtlasGroupHelper] %s"), *Msg);
}

UPaperSpriteAtlas* FAtlasGroupHelper::EnsureAtlasAsset(const FString& InParentContentPath)
{
    FString ParentPath = InParentContentPath;
    while (ParentPath.EndsWith(TEXT("/")))
    {
        ParentPath.LeftChopInline(1);
    }

    if (ParentPath.IsEmpty())
    {
        return nullptr;
    }

    // 已存在：直接加载返回
    const FString AtlasObjectPath = ParentPath / TEXT("Atlas");
    if (UObject* Existing = LoadObject<UObject>(nullptr, *AtlasObjectPath))
    {
        if (UPaperSpriteAtlas* ExistingAtlas = Cast<UPaperSpriteAtlas>(Existing))
        {
            return ExistingAtlas;
        }
        UE_LOG(LogTemp, Warning, TEXT("[AtlasGroupHelper] %s 已存在但不是 UPaperSpriteAtlas，跳过。"), *AtlasObjectPath);
        return nullptr;
    }

    // 不存在：创建新的 UPaperSpriteAtlas
    FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
    UPaperSpriteAtlas* Atlas = Cast<UPaperSpriteAtlas>(
        AssetToolsModule.Get().CreateAsset(TEXT("Atlas"), *ParentPath, UPaperSpriteAtlas::StaticClass(), nullptr));
    if (Atlas != nullptr)
    {
        Atlas->MarkPackageDirty();
        SavePackage(Atlas->GetOutermost());
        UE_LOG(LogTemp, Log, TEXT("[AtlasGroupHelper] 已创建图集: %s"), *AtlasObjectPath);
    }
    return Atlas;
}

TArray<FString> FAtlasGroupHelper::FindAllAtlasFolders()
{
    TArray<FString> Result;
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    TArray<FString> SubPaths;
    Registry.GetSubPaths(TEXT("/Game/AssetsPackage"), SubPaths, /*bInRecurse=*/true);
    for (const FString& Path : SubPaths)
    {
        if (Path.EndsWith(TEXT("/Atlas")))
        {
            Result.Add(Path);
        }
    }
    return Result;
}

int32 FAtlasGroupHelper::SetAtlasGroupForFolder(const FString& InAtlasFolderPath, UPaperSpriteAtlas* InAtlas)
{
    if (InAtlas == nullptr)
    {
        return 0;
    }

    int32 Count = 0;
    IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

    TArray<FAssetData> Assets;
    Registry.GetAssetsByPath(FName(*InAtlasFolderPath), Assets, /*bRecursive=*/true);

    const FTopLevelAssetPath PaperSpriteClassPath = UPaperSprite::StaticClass()->GetClassPathName();

    for (const FAssetData& Asset : Assets)
    {
        if (Asset.AssetClassPath != PaperSpriteClassPath)
        {
            continue;
        }

        UPaperSprite* Sprite = Cast<UPaperSprite>(Asset.GetAsset());
        if (Sprite == nullptr)
        {
            continue;
        }

        // AtlasGroup 为 protected 成员，通过反射设置
        FObjectProperty* AtlasGroupProp = FindFieldChecked<FObjectProperty>(UPaperSprite::StaticClass(), TEXT("AtlasGroup"));
        if (AtlasGroupProp->GetObjectPropertyValue_InContainer(Sprite) == InAtlas)
        {
            continue;
        }

        Sprite->Modify();
        AtlasGroupProp->SetObjectPropertyValue_InContainer(Sprite, InAtlas);

        // 触发引擎图集重建（Paper2DEditor 的 OnObjectPropertyChanged 回调）
        FPropertyChangedEvent ChangedEvent(AtlasGroupProp, EPropertyChangeType::ValueSet);
        Sprite->PostEditChangeProperty(ChangedEvent);

        Sprite->MarkPackageDirty();
        SavePackage(Sprite->GetOutermost());
        ++Count;
    }

    return Count;
}

void FAtlasGroupHelper::SavePackage(UPackage* InPackage)
{
    if (InPackage == nullptr)
    {
        return;
    }

    const FString FileName = FPackageName::LongPackageNameToFilename(
        InPackage->GetName(), FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;

    UPackage::SavePackage(InPackage, nullptr, *FileName, SaveArgs);
}

#undef LOCTEXT_NAMESPACE
