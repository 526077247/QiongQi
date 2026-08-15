#include "UITemplateCodeGenerator.h"

#include "WidgetBlueprintEditor.h"
#include "WidgetBlueprint.h"
#include "WidgetReference.h"
#include "Blueprint/WidgetTree.h"

#include "Components/Widget.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Slider.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/MessageDialog.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "QiongQiEditor"

bool FUITemplateCodeGenerator::Export(FWidgetBlueprintEditor* BlueprintEditor)
{
    if (BlueprintEditor == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] Export 失败：编辑器实例为空"));
        return false;
    }

    UWidgetBlueprint* Blueprint = BlueprintEditor->GetWidgetBlueprintObj();
    if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("InvalidBlueprint", "当前 Widget Blueprint 无效，无法导出。"));
        return false;
    }

    const TSet<FWidgetReference>& SelectedWidgets = BlueprintEditor->GetSelectedWidgets();
    if (SelectedWidgets.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoSelection", "未选中任何节点。\n请在 Hierarchy 面板选中要导出的 UI 节点后再试。"));
        return false;
    }

    UWidget* Root = Blueprint->WidgetTree->RootWidget;

    // 资产路径 → ts 输出路径
    const FString AssetPath = Blueprint->GetPackage()->GetPathName();
    FString ImportPrefix;
    FString FileName;
    const FString OutputPath = BuildOutputPath(AssetPath, ImportPrefix, FileName);
    if (OutputPath.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NotUIResource", "该资产不在 Content/AssetsPackage/UI 目录下，不是标准 UI 资源。\n无法生成 TS 模板代码。"));
        return false;
    }

    const bool bIsView = FileName.EndsWith(TEXT("View")) || FileName.EndsWith(TEXT("Win"));

    FString PrefabPath;
    if (bIsView && Blueprint->GeneratedClass != nullptr)
    {
        PrefabPath = Blueprint->GeneratedClass->GetPathName();
    }

    // 收集选中节点信息
    TArray<FWidgetInfo> Infos;
    TSet<FString> UsedNames;
    for (const FWidgetReference& Ref : SelectedWidgets)
    {
        UWidget* Widget = Ref.GetTemplate();
        if (Widget == nullptr)
        {
            continue;
        }

        FWidgetInfo Info;
        Info.UITypeName = GetUITypeName(Widget);
        Info.RelativePath = BuildRelativePath(Root, Widget);

        // 字段名：去空格、首字母小写、重名加数字后缀
        FString FieldName = Widget->GetName().Replace(TEXT(" "), TEXT(""));
        if (FieldName.IsEmpty())
        {
            FieldName = TEXT("widget");
        }
        else
        {
            FieldName = FieldName.Left(1).ToLower() + FieldName.RightChop(1);
        }

        const FString BaseName = FieldName;
        int32 Index = 1;
        while (UsedNames.Contains(FieldName))
        {
            FieldName = FString::Printf(TEXT("%s%d"), *BaseName, Index);
            ++Index;
        }
        UsedNames.Add(FieldName);
        Info.FieldName = FieldName;

        Infos.Add(MoveTemp(Info));
    }

    if (Infos.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoWidget", "选中的节点均无法解析，请检查节点是否有效。"));
        return false;
    }

    // 生成模板文本
    const FString Template = BuildTemplate(FileName, bIsView, PrefabPath, ImportPrefix, Infos);

    // 写出（已存在不覆盖）
    const bool bFileExists = FPaths::FileExists(OutputPath);
    if (!bFileExists)
    {
        if (FFileHelper::SaveStringToFile(Template, *OutputPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        {
            UE_LOG(LogTemp, Log, TEXT("[QiongQiEditor] UI 模板代码已生成: %s"), *OutputPath);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("[QiongQiEditor] 写入文件失败: %s"), *OutputPath);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] 文件已存在，跳过写入: %s"), *OutputPath);
    }

    // 复制到剪贴板
    FPlatformApplicationMisc::ClipboardCopy(*Template);

    // 编辑器通知
    FNotificationInfo NotificationInfo(FText::Format(
        LOCTEXT("ExportSuccess", "已导出 UI 模板代码（已复制到剪贴板）：\n{0}"), FText::FromString(OutputPath)));
    NotificationInfo.ExpireDuration = 6.0f;
    FSlateNotificationManager::Get().AddNotification(NotificationInfo);

    return true;
}

bool FUITemplateCodeGenerator::CopyRelativePaths(FWidgetBlueprintEditor* BlueprintEditor)
{
    if (BlueprintEditor == nullptr)
    {
        UE_LOG(LogTemp, Warning, TEXT("[QiongQiEditor] CopyRelativePaths 失败：编辑器实例为空"));
        return false;
    }

    UWidgetBlueprint* Blueprint = BlueprintEditor->GetWidgetBlueprintObj();
    if (Blueprint == nullptr || Blueprint->WidgetTree == nullptr)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("InvalidBlueprintCopy", "当前 Widget Blueprint 无效，无法复制相对路径。"));
        return false;
    }

    const TSet<FWidgetReference>& SelectedWidgets = BlueprintEditor->GetSelectedWidgets();
    if (SelectedWidgets.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoSelectionCopy", "未选中任何节点。\n请在 Hierarchy 面板选中要复制相对路径的节点后再试。"));
        return false;
    }

    UWidget* Root = Blueprint->WidgetTree->RootWidget;

    // 逐节点计算相对根节点路径（根节点自身为空串），多选按行拼接
    TArray<FString> Paths;
    for (const FWidgetReference& Ref : SelectedWidgets)
    {
        UWidget* Widget = Ref.GetTemplate();
        if (Widget == nullptr)
        {
            continue;
        }

        const FString RelativePath = BuildRelativePath(Root, Widget);
        if (RelativePath.IsEmpty() && Widget != Root)
        {
            // 非根节点解析失败（不在根节点层级下），跳过
            continue;
        }
        Paths.Add(RelativePath);
    }

    if (Paths.Num() == 0)
    {
        FMessageDialog::Open(EAppMsgType::Ok,
            LOCTEXT("NoPathCopy", "选中的节点均无法解析相对路径，请检查节点是否有效。"));
        return false;
    }

    const FString ClipboardText = FString::Join(Paths, TEXT("\n"));
    FPlatformApplicationMisc::ClipboardCopy(*ClipboardText);

    FNotificationInfo NotificationInfo(FText::Format(
        LOCTEXT("CopySuccess", "已复制 {0} 个节点的相对路径到剪贴板。"),
        FText::AsNumber(Paths.Num())));
    NotificationInfo.ExpireDuration = 4.0f;
    FSlateNotificationManager::Get().AddNotification(NotificationInfo);

    UE_LOG(LogTemp, Log, TEXT("[QiongQiEditor] 已复制相对路径：\n%s"), *ClipboardText);
    return true;
}

FString FUITemplateCodeGenerator::BuildRelativePath(UWidget* Root, UWidget* Target)
{
    if (Root == nullptr || Target == nullptr || Root == Target)
    {
        return TEXT("");
    }

    TArray<FString> Names;
    UWidget* Current = Target;
    while (Current != nullptr && Current != Root)
    {
        Names.Insert(Current->GetName(), 0);
        Current = Current->GetParent();
    }

    if (Current != Root)
    {
        return TEXT("");
    }

    return FString::Join(Names, TEXT("/"));
}

FString FUITemplateCodeGenerator::GetUITypeName(UWidget* Widget)
{
    if (Widget == nullptr)
    {
        return TEXT("UIEmptyView");
    }

    if (Widget->IsA<UButton>())
    {
        return TEXT("UIButton");
    }
    if (Widget->IsA<UImage>())
    {
        return TEXT("UIImage");
    }
    if (Widget->IsA<UTextBlock>())
    {
        return TEXT("UIText");
    }
    if (Widget->IsA<USlider>())
    {
        return TEXT("UISlider");
    }
    if (Widget->IsA<UProgressBar>())
    {
        return TEXT("UIProgressBar");
    }
    if (Widget->IsA<UScrollBox>())
    {
        // 项目循环列表容器基于 ScrollBox；需要网格请手动改为 UILoopGridView
        return TEXT("UILoopListView2");
    }

    return TEXT("UIEmptyView");
}

FString FUITemplateCodeGenerator::NormalizeSegment(const FString& Segment)
{
    if (Segment.IsEmpty())
    {
        return Segment;
    }

    if (Segment.StartsWith(TEXT("UI"), ESearchCase::CaseSensitive))
    {
        if (Segment.Len() > 2)
        {
            return TEXT("UI") + Segment.Mid(2, 1).ToUpper() + Segment.RightChop(3);
        }
        return Segment;
    }

    return Segment.Left(1).ToUpper() + Segment.RightChop(1);
}

FString FUITemplateCodeGenerator::BuildOutputPath(const FString& AssetPath, FString& OutImportPrefix, FString& OutFileName)
{
    // AssetPath 形如：/Game/AssetsPackage/UI/UIMain/Prefabs/UIMainView
    FString Relative = AssetPath;
    Relative.RemoveFromStart(TEXT("/Game/"), ESearchCase::CaseSensitive);

    TArray<FString> Segments;
    Relative.ParseIntoArray(Segments, TEXT("/"), true);
    if (Segments.Num() < 3)
    {
        return TEXT("");
    }

    // 必须位于 AssetsPackage/UI 下
    if (!Segments[0].Equals(TEXT("AssetsPackage"), ESearchCase::IgnoreCase))
    {
        return TEXT("");
    }
    if (!Segments[1].StartsWith(TEXT("UI"), ESearchCase::CaseSensitive))
    {
        return TEXT("");
    }

    FString RelativeOutDir = TEXT("TypeScript/Code/Game/UI");
    int32 UICount = 0;
    FString FileName;

    for (int32 i = 2; i < Segments.Num(); ++i)
    {
        const FString& Segment = Segments[i];
        if (Segment.Equals(TEXT("Prefabs"), ESearchCase::IgnoreCase))
        {
            continue;
        }

        if (i == Segments.Num() - 1)
        {
            FileName = NormalizeSegment(Segment);
            break;
        }

        RelativeOutDir /= NormalizeSegment(Segment);
        ++UICount;
    }

    if (FileName.IsEmpty())
    {
        return TEXT("");
    }

    // import 前缀：../../ + ../ * UI目录段数（对齐 cocos 版 points 计算）
    OutImportPrefix = TEXT("../../");
    for (int32 i = 0; i < UICount; ++i)
    {
        OutImportPrefix += TEXT("../");
    }

    OutFileName = FileName;
    return FPaths::ProjectDir() / RelativeOutDir / (FileName + TEXT(".ts"));
}

FString FUITemplateCodeGenerator::BuildTemplate(const FString& FileName, bool bIsView, const FString& PrefabPath,
    const FString& ImportPrefix, const TArray<FWidgetInfo>& Infos)
{
    TArray<FString> Lines;

    // ---- imports ----
    Lines.Add(FString::Printf(TEXT("import { IOnCreate } from \"%sModule/UI/IOnCreate\";"), *ImportPrefix));
    Lines.Add(FString::Printf(TEXT("import { IOnEnable } from \"%sModule/UI/IOnEnable\";"), *ImportPrefix));
    if (bIsView)
    {
        Lines.Add(FString::Printf(TEXT("import { UIBaseView, uiView } from \"%sModule/UI/UIBaseView\";"), *ImportPrefix));
    }
    else
    {
        Lines.Add(FString::Printf(TEXT("import { UIBaseContainer } from \"%sModule/UI/UIBaseContainer\";"), *ImportPrefix));
    }

    TArray<FString> UITypes;
    bool bHasLoopList = false;
    bool bHasLoopGrid = false;
    for (const FWidgetInfo& Info : Infos)
    {
        if (!UITypes.Contains(Info.UITypeName))
        {
            UITypes.Add(Info.UITypeName);
        }
        if (Info.UITypeName == TEXT("UILoopListView2"))
        {
            bHasLoopList = true;
        }
        else if (Info.UITypeName == TEXT("UILoopGridView"))
        {
            bHasLoopGrid = true;
        }
    }

    for (const FString& UIType : UITypes)
    {
        Lines.Add(FString::Printf(TEXT("import { %s } from \"%sModule/UIComponent/%s\";"), *UIType, *ImportPrefix, *UIType));
    }
    if (bHasLoopList)
    {
        Lines.Add(FString::Printf(TEXT("import { LoopListView2 } from \"%s../ThirdParty/SuperScrollView/ListView/LoopListView2\";"), *ImportPrefix));
        Lines.Add(FString::Printf(TEXT("import { LoopListViewItem2 } from \"%s../ThirdParty/SuperScrollView/ListView/LoopListViewItem2\";"), *ImportPrefix));
    }
    if (bHasLoopGrid)
    {
        Lines.Add(FString::Printf(TEXT("import { LoopGridView } from \"%s../ThirdParty/SuperScrollView/GridView/LoopGridView\";"), *ImportPrefix));
        Lines.Add(FString::Printf(TEXT("import { LoopGridViewItem } from \"%s../ThirdParty/SuperScrollView/GridView/LoopGridViewItem\";"), *ImportPrefix));
    }
    Lines.Add(TEXT(""));

    // ---- 类声明 ----
    if (bIsView)
    {
        Lines.Add(FString::Printf(TEXT("@uiView(\"%s\")"), *FileName));
    }
    Lines.Add(FString::Printf(TEXT("export class %s extends %s implements IOnCreate, IOnEnable {"),
        *FileName, bIsView ? TEXT("UIBaseView") : TEXT("UIBaseContainer")));
    Lines.Add(TEXT(""));

    if (bIsView)
    {
        Lines.Add(FString::Printf(TEXT("    public static readonly PrefabPath:string = \"%s\";"), *PrefabPath));
        Lines.Add(TEXT(""));
    }

    Lines.Add(TEXT("    public getConstructor()"));
    Lines.Add(TEXT("    {"));
    Lines.Add(FString::Printf(TEXT("        return %s;"), *FileName));
    Lines.Add(TEXT("    }"));
    Lines.Add(TEXT(""));

    // ---- 字段 ----
    for (const FWidgetInfo& Info : Infos)
    {
        Lines.Add(FString::Printf(TEXT("    public %s: %s;"), *Info.FieldName, *Info.UITypeName));
    }
    Lines.Add(TEXT(""));

    // ---- onCreate ----
    Lines.Add(TEXT("    public onCreate()"));
    Lines.Add(TEXT("    {"));
    for (const FWidgetInfo& Info : Infos)
    {
        const FString UpperName = Info.FieldName.Left(1).ToUpper() + Info.FieldName.RightChop(1);

        if (Info.RelativePath.IsEmpty())
        {
            Lines.Add(FString::Printf(TEXT("        this.%s = this.addComponent<%s>(%s);"),
                *Info.FieldName, *Info.UITypeName, *Info.UITypeName));
        }
        else
        {
            Lines.Add(FString::Printf(TEXT("        this.%s = this.addComponent<%s>(%s,\"%s\");"),
                *Info.FieldName, *Info.UITypeName, *Info.UITypeName, *Info.RelativePath));
        }

        if (Info.UITypeName == TEXT("UILoopGridView"))
        {
            Lines.Add(FString::Printf(TEXT("        this.%s.initGridView(0, this.onGet%sItemByIndex.bind(this));"),
                *Info.FieldName, *UpperName));
        }
        else if (Info.UITypeName == TEXT("UILoopListView2"))
        {
            Lines.Add(FString::Printf(TEXT("        this.%s.initListView(this.onGet%sItemByIndex.bind(this));"),
                *Info.FieldName, *UpperName));
        }
    }
    Lines.Add(TEXT("    }"));
    Lines.Add(TEXT(""));

    // ---- onEnable ----
    Lines.Add(TEXT("    public onEnable()"));
    Lines.Add(TEXT("    {"));
    for (const FWidgetInfo& Info : Infos)
    {
        if (Info.UITypeName == TEXT("UIButton"))
        {
            const FString UpperName = Info.FieldName.Left(1).ToUpper() + Info.FieldName.RightChop(1);
            Lines.Add(FString::Printf(TEXT("        this.%s.setOnClick(this.onClick%s.bind(this));"),
                *Info.FieldName, *UpperName));
        }
    }
    Lines.Add(TEXT("    }"));
    Lines.Add(TEXT(""));

    // ---- 函数骨架 ----
    for (const FWidgetInfo& Info : Infos)
    {
        const FString UpperName = Info.FieldName.Left(1).ToUpper() + Info.FieldName.RightChop(1);

        if (Info.UITypeName == TEXT("UIButton"))
        {
            Lines.Add(FString::Printf(TEXT("    private onClick%s()"), *UpperName));
            Lines.Add(TEXT("    {"));
            Lines.Add(TEXT(""));
            Lines.Add(TEXT("    }"));
            Lines.Add(TEXT(""));
        }
        else if (Info.UITypeName == TEXT("UILoopGridView"))
        {
            Lines.Add(FString::Printf(
                TEXT("    private onGet%sItemByIndex(gridView: LoopGridView, index: number, row: number, column: number): LoopGridViewItem"),
                *UpperName));
            Lines.Add(TEXT("    {"));
            Lines.Add(TEXT("        return null;"));
            Lines.Add(TEXT("    }"));
            Lines.Add(TEXT(""));
        }
        else if (Info.UITypeName == TEXT("UILoopListView2"))
        {
            Lines.Add(FString::Printf(
                TEXT("    private onGet%sItemByIndex(listView: LoopListView2, index: number): LoopListViewItem2"),
                *UpperName));
            Lines.Add(TEXT("    {"));
            Lines.Add(TEXT("        return null;"));
            Lines.Add(TEXT("    }"));
            Lines.Add(TEXT(""));
        }
    }

    Lines.Add(TEXT("}"));

    return FString::Join(Lines, TEXT("\n")) + TEXT("\n");
}

#undef LOCTEXT_NAMESPACE
