#pragma once

#include "CoreMinimal.h"

class UWidget;
class UWidgetBlueprint;
class FWidgetBlueprintEditor;

/**
 * UI 模板代码生成器
 * 将 UMG Widget Blueprint 编辑器中选中的节点按 cocos 版 code-generate.ts 的逻辑
 * 导出为 TS UI 模板代码（字段 / onCreate / onEnable / 特殊组件函数骨架）。
 */
class FUITemplateCodeGenerator
{
public:
    /** 读取选中节点并生成/写出 TS 模板；成功返回 true */
    static bool Export(FWidgetBlueprintEditor* BlueprintEditor);

    /** 将选中节点相对根节点的路径（/ 分隔）复制到剪贴板，多选按行拼接；成功返回 true */
    static bool CopyRelativePaths(FWidgetBlueprintEditor* BlueprintEditor);

private:
    /** 单个选中节点的生成信息 */
    struct FWidgetInfo
    {
        FString FieldName;     // 字段名（首字母小写，重名加数字）
        FString UITypeName;    // 对应 UI 组件类型，如 UIButton / UIImage
        FString RelativePath;  // 相对根节点的路径（/ 分隔），根节点为空
    };

    /** 计算相对根节点路径（用 GetName 拼接 "/"），root 即 target 时返回空串 */
    static FString BuildRelativePath(UWidget* Root, UWidget* Target);

    /** 类型映射：UButton→UIButton、UScrollBox→UILoopListView2 等 */
    static FString GetUITypeName(UWidget* Widget);

    /** 段名规范化：UI 前缀保持 UI 并大写后续首字母，否则首字母大写 */
    static FString NormalizeSegment(const FString& Segment);

    /** 资产路径 → ts 输出绝对路径；OutImportPrefix 返回 import 相对前缀，OutFileName 返回类名/文件名 */
    static FString BuildOutputPath(const FString& AssetPath, FString& OutImportPrefix, FString& OutFileName);

    /** 组装完整 TS 模板文本 */
    static FString BuildTemplate(const FString& FileName, bool bIsView, const FString& PrefabPath,
        const FString& ImportPrefix, const TArray<FWidgetInfo>& Infos);
};
