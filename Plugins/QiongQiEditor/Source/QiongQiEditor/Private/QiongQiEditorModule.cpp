#include "QiongQiEditorModule.h"
#include "UITemplateCodeGenerator.h"

#include "WidgetBlueprintEditor.h"
#include "WidgetBlueprint.h"

#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"

#include "ToolMenus.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenuEntry.h"

#include "Framework/Commands/UIAction.h"
#include "Textures/SlateIcon.h"
#include "Misc/MessageDialog.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserDataSubsystem.h"
#include "ContentBrowserItem.h"
#include "IContentBrowserDataModule.h"

#include "HAL/FileManager.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "QiongQiEditor"

void FQiongQiEditorModule::StartupModule()
{
    FCoreDelegates::OnFEngineLoopInitComplete.AddRaw(this, &FQiongQiEditorModule::OnEngineInitComplete);
}

void FQiongQiEditorModule::ShutdownModule()
{
    FCoreDelegates::OnFEngineLoopInitComplete.RemoveAll(this);

    if (bSubscribed && GEditor)
    {
        if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            Subsystem->OnAssetEditorOpened().RemoveAll(this);
        }
        bSubscribed = false;
    }

    UToolMenus::UnregisterOwner(this);
}

void FQiongQiEditorModule::OnEngineInitComplete()
{
    FCoreDelegates::OnFEngineLoopInitComplete.RemoveAll(this);

    RegisterMenu();
    RegisterContentBrowserMenu();

    if (GEditor)
    {
        if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            Subsystem->OnAssetEditorOpened().AddRaw(this, &FQiongQiEditorModule::OnAssetEditorOpened);
            bSubscribed = true;
        }
    }
}

void FQiongQiEditorModule::OnAssetEditorOpened(UObject* Asset)
{
    UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(Asset);
    if (Blueprint == nullptr)
    {
        return;
    }

    if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
    {
        if (IAssetEditorInstance* EditorInstance = Subsystem->FindEditorForAsset(Blueprint, false))
        {
            if (EditorInstance->GetEditorName() == TEXT("WidgetBlueprintEditor"))
            {
                WeakWidgetBlueprint = Blueprint;
                RegisterMenu();
            }
        }
    }
}

void FQiongQiEditorModule::RegisterMenu()
{
    if (bMenuRegistered)
    {
        return;
    }

    UToolMenus* ToolMenus = UToolMenus::TryGet();
    if (ToolMenus == nullptr)
    {
        return;
    }

    static const FName AssetMenuName(TEXT("AssetEditor.WidgetBlueprintEditor.MainMenu.Asset"));
    UToolMenu* AssetMenu = ToolMenus->ExtendMenu(AssetMenuName);
    if (AssetMenu == nullptr)
    {
        return;
    }

    FToolMenuSection* Section = AssetMenu->FindSection(TEXT("QiongQi"));
    if (Section == nullptr)
    {
        Section = &AssetMenu->AddSection(TEXT("QiongQi"), LOCTEXT("QiongQiSectionLabel", "QiongQi"));
    }

    if (!Section->FindEntry(TEXT("ExportUITemplateCode")))
    {
        Section->AddMenuEntry(
            TEXT("ExportUITemplateCode"),
            LOCTEXT("ExportUITemplateCodeLabel", "根据选择节点生成UI代码"),
            LOCTEXT("ExportUITemplateCodeTooltip", "根据 Hierarchy 面板当前选中的节点生成 TS UI 模板代码并复制到剪贴板"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FQiongQiEditorModule::ExecuteExportUI))
        );
    }

    if (!Section->FindEntry(TEXT("CopyRelativePath")))
    {
        Section->AddMenuEntry(
            TEXT("CopyRelativePath"),
            LOCTEXT("CopyRelativePathLabel", "复制相对路径"),
            LOCTEXT("CopyRelativePathTooltip", "将 Hierarchy 面板当前选中节点相对根节点的路径复制到剪贴板（多选时按行分隔）"),
            FSlateIcon(),
            FUIAction(FExecuteAction::CreateRaw(this, &FQiongQiEditorModule::ExecuteCopyRelativePath))
        );
    }

    bMenuRegistered = true;
}

void FQiongQiEditorModule::ExecuteExportUI()
{
    FWidgetBlueprintEditor* WidgetEditor = GetActiveWidgetBlueprintEditor();
    if (WidgetEditor == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoEditorOpen", "未找到打开的 UI Widget Blueprint 编辑器。"));
        return;
    }

    FUITemplateCodeGenerator::Export(WidgetEditor);
}

void FQiongQiEditorModule::ExecuteCopyRelativePath()
{
    FWidgetBlueprintEditor* WidgetEditor = GetActiveWidgetBlueprintEditor();
    if (WidgetEditor == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoEditorOpen", "未找到打开的 UI Widget Blueprint 编辑器。"));
        return;
    }

    FUITemplateCodeGenerator::CopyRelativePaths(WidgetEditor);
}

void FQiongQiEditorModule::RegisterContentBrowserMenu()
{
    if (bContentBrowserMenuRegistered)
    {
        return;
    }

    UToolMenus* ToolMenus = UToolMenus::TryGet();
    if (ToolMenus == nullptr)
    {
        return;
    }

    // 内容浏览器 PathView 与 AssetView 的文件夹右键共用此菜单名
    static const FName FolderMenuName(TEXT("ContentBrowser.FolderContextMenu"));
    UToolMenu* FolderMenu = ToolMenus->ExtendMenu(FolderMenuName);
    if (FolderMenu == nullptr)
    {
        return;
    }

    // 动态段：仅在右键 UI 文件夹时注入"工具→创建子目录"，避免普通文件夹出现无关菜单
    FolderMenu->AddDynamicSection(
        TEXT("QiongQiTools"),
        FNewToolMenuDelegate::CreateRaw(this, &FQiongQiEditorModule::PopulateQiongQiFolderSection)
    );

    bContentBrowserMenuRegistered = true;
}

void FQiongQiEditorModule::PopulateQiongQiFolderSection(UToolMenu* Menu)
{
    if (Menu == nullptr)
    {
        return;
    }

    const UContentBrowserFolderContext* FolderContext = Menu->FindContext<UContentBrowserFolderContext>();
    if (FolderContext == nullptr)
    {
        return;
    }

    // 仅收集 UI 目录（/Game/AssetsPackage/UI 及其子目录）
    const TArray<FString>& SelectedPackagePaths = FolderContext->GetSelectedPackagePaths();
    TArray<FString> SelectedUIFolders;
    for (const FString& PackagePath : SelectedPackagePaths)
    {
        FString NormalizedPath = PackagePath;
        while (NormalizedPath.EndsWith(TEXT("/")))
        {
            NormalizedPath.LeftChopInline(1);
        }

        if (NormalizedPath.Equals(TEXT("/Game/AssetsPackage/UI"), ESearchCase::IgnoreCase)
            || NormalizedPath.StartsWith(TEXT("/Game/AssetsPackage/UI/"), ESearchCase::IgnoreCase))
        {
            SelectedUIFolders.Add(NormalizedPath);
        }
    }

    if (SelectedUIFolders.Num() == 0)
    {
        return;
    }

    FToolMenuSection& Section = Menu->AddSection(TEXT("QiongQiTools"));

    // 工具子菜单（捕获路径副本，避免引用悬垂）
    Section.AddSubMenu(
        TEXT("QiongQiToolsSubMenu"),
        LOCTEXT("QiongQiToolsLabel", "工具"),
        LOCTEXT("QiongQiToolsTooltip", "QiongQi 编辑器工具"),
        FNewToolMenuChoice(FNewToolMenuDelegate::CreateLambda(
            [this, SelectedUIFolders](UToolMenu* SubMenu)
            {
                FToolMenuSection& SubSection = SubMenu->AddSection(TEXT("CreateSubDirectoriesSection"), FText::GetEmpty());
                SubSection.AddMenuEntry(
                    TEXT("CreateSubDirectories"),
                    LOCTEXT("CreateSubDirsLabel", "创建子目录"),
                    LOCTEXT("CreateSubDirsTooltip", "在当前选中的 UI 文件夹下创建 Prefabs/Atlas/Animations/DiscreteImages 子目录"),
                    FSlateIcon(),
                    FUIAction(FExecuteAction::CreateLambda(
                        [this, SelectedUIFolders]() { ExecuteCreateSubDirectories(SelectedUIFolders); }))
                );
            }
        ))
    );
}

void FQiongQiEditorModule::ExecuteCreateSubDirectories(const TArray<FString>& SelectedUIFolders)
{
    static const TArray<FString> SubDirNames = {
        TEXT("Prefabs"), TEXT("Atlas"), TEXT("Animations"), TEXT("DiscreteImages") };

    UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
    if (ContentBrowserData == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoContentBrowserData", "无法访问内容浏览器数据子系统，创建子目录失败。"));
        return;
    }

    int32 CreatedCount = 0;
    int32 DiskFallbackCount = 0;
    TArray<FString> FailedPaths;
    FScopedSuppressContentBrowserDataTick TickSuppression(ContentBrowserData);

    for (const FString& Folder : SelectedUIFolders)
    {
        for (const FString& SubDirName : SubDirNames)
        {
            const FString FullPath = Folder / SubDirName;

            FText ErrorMsg;
            const FContentBrowserItemTemporaryContext PendingItem = ContentBrowserData->CreateFolder(FName(*FullPath));
            if (PendingItem.IsValid())
            {
                // 与引擎 SPathView 新建文件夹流程一致：ValidateItem/FinalizeItem 均传目录名
                if (!PendingItem.ValidateItem(SubDirName, &ErrorMsg))
                {
                    FailedPaths.Add(FullPath);
                    UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] 创建子目录失败（校验未通过）: %s，%s"), *FullPath, *ErrorMsg.ToString());
                    continue;
                }

                const FContentBrowserItem NewItem = PendingItem.FinalizeItem(SubDirName, &ErrorMsg);
                if (NewItem.IsValid())
                {
                    ++CreatedCount;
                    continue;
                }

                // FinalizeItem 失败：可能是目录已存在等，回退磁盘创建
                FailedPaths.Add(FullPath);
                UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] 创建子目录失败（FinalizeItem）: %s，%s"), *FullPath, *ErrorMsg.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] 创建子目录失败（CreateFolder 未产生有效项）: %s"), *FullPath);
            }

            // —— 兜底方案：直接在磁盘创建目录，DirectoryWatcher 会自动让内容浏览器感知 ——
            FString RelativePath = FullPath;
            static const FString GamePrefix = TEXT("/Game/");
            if (RelativePath.StartsWith(GamePrefix))
            {
                RelativePath.RightChopInline(GamePrefix.Len());
            }
            const FString DiskPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / RelativePath);
            if (IFileManager::Get().MakeDirectory(*DiskPath, /*bTree=*/true))
            {
                ++CreatedCount;
                ++DiskFallbackCount;
            }
            else
            {
                FailedPaths.AddUnique(FullPath);
                UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] 磁盘创建目录失败: %s"), *DiskPath);
            }
        }
    }

    FText ResultText;
    if (FailedPaths.Num() == 0)
    {
        if (DiskFallbackCount > 0)
        {
            ResultText = FText::Format(
                LOCTEXT("CreateSubDirsSuccessDisk", "已创建 {0} 个子目录（其中 {1} 个通过磁盘直建，内容浏览器将自动刷新）。"),
                FText::AsNumber(CreatedCount), FText::AsNumber(DiskFallbackCount));
        }
        else
        {
            ResultText = FText::Format(
                LOCTEXT("CreateSubDirsSuccess", "已创建 {0} 个子目录。"),
                FText::AsNumber(CreatedCount));
        }
    }
    else
    {
        ResultText = FText::Format(
            LOCTEXT("CreateSubDirsPartial", "已创建 {0} 个子目录，{1} 个失败（已存在或路径无效）。"),
            FText::AsNumber(CreatedCount), FText::AsNumber(FailedPaths.Num()));
    }

    FNotificationInfo NotificationInfo(ResultText);
    NotificationInfo.ExpireDuration = 5.0f;
    FSlateNotificationManager::Get().AddNotification(NotificationInfo);

    UE_LOG(LogTemp, Log, TEXT("[QiongQiEditor] 创建子目录完成：成功 %d 个，失败 %d 个"), CreatedCount, FailedPaths.Num());
    for (const FString& FailedPath : FailedPaths)
    {
        UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] 创建子目录失败: %s"), *FailedPath);
    }
}

FWidgetBlueprintEditor* FQiongQiEditorModule::GetActiveWidgetBlueprintEditor() const
{
    if (WeakWidgetBlueprint.IsValid())
    {
        if (UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
        {
            if (IAssetEditorInstance* EditorInstance = Subsystem->FindEditorForAsset(WeakWidgetBlueprint.Get(), false))
            {
                if (EditorInstance->GetEditorName() == TEXT("WidgetBlueprintEditor"))
                {
                    return static_cast<FWidgetBlueprintEditor*>(EditorInstance);
                }
            }
        }
    }
    return nullptr;
}

IMPLEMENT_MODULE(FQiongQiEditorModule, QiongQiEditor)

#undef LOCTEXT_NAMESPACE
