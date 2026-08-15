#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FWidgetBlueprintEditor;
class UWidgetBlueprint;
class UToolMenu;

/**
 * 编辑器扩展模块：为 UMG Widget Blueprint 编辑器提供"根据选择节点生成UI代码"与"复制相对路径"工具，
 * 并为内容浏览器 UI 文件夹提供"创建子目录"快捷菜单。
 */
class FQiongQiEditorModule : public IModuleInterface
{
public:
    //~ IModuleInterface
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    //~ End IModuleInterface

    /** 菜单点击入口：根据当前选中的 UI 节点生成 TS 模板代码 */
    void ExecuteExportUI();

    /** 菜单点击入口：复制选中节点相对根节点的路径 */
    void ExecuteCopyRelativePath();

private:
    /** 引擎完全初始化后（level editor 已启动）执行注册 */
    void OnEngineInitComplete();

    /** 订阅编辑器打开事件，记录当前 UMG 编辑器与蓝图 */
    void OnAssetEditorOpened(UObject* Asset);

    /** 在 UMG Widget Blueprint 编辑器 Asset 菜单注册导出菜单项（幂等） */
    void RegisterMenu();

    /** 在内容浏览器 UI 文件夹右键菜单注册"工具→创建子目录"（幂等） */
    void RegisterContentBrowserMenu();

    /** 动态填充内容浏览器文件夹右键菜单的 QiongQi 段（仅在 UI 文件夹右键时添加项） */
    void PopulateQiongQiFolderSection(UToolMenu* Menu);

    /** 在选中的 UI 文件夹下创建 Prefabs/Atlas/Animations/DiscreteImages 子目录 */
    void ExecuteCreateSubDirectories(const TArray<FString>& SelectedUIFolders);

    /** 定位最近一次打开的 UMG Widget Blueprint 编辑器实例 */
    FWidgetBlueprintEditor* GetActiveWidgetBlueprintEditor() const;

    /** 最近一次打开的 UI Widget Blueprint */
    TWeakObjectPtr<UWidgetBlueprint> WeakWidgetBlueprint;

    /** 菜单是否已注册 */
    bool bMenuRegistered = false;

    /** 内容浏览器右键菜单是否已注册 */
    bool bContentBrowserMenuRegistered = false;

    /** 是否已订阅编辑器事件 */
    bool bSubscribed = false;
};
