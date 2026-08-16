#pragma once

#include "CoreMinimal.h"

class UPaperSpriteAtlas;

/**
 * Atlas 目录 Sprite 图集分组一键工具。
 *
 * 功能：
 * 1. 遍历 /Game/AssetsPackage 下所有名为 Atlas 的内容目录；
 * 2. 确保每个 Atlas 目录的同级存在 Atlas.uasset（UPaperSpriteAtlas），缺失则自动创建；
 * 3. 将 Atlas 目录下所有 UPaperSprite 的 AtlasGroup 指向该图集资产并保存。
 *
 * 设置后通过 Sprite->PostEditChangeProperty 触发引擎图集重建
 * （Paper2DEditor 注册的 FCoreUObjectDelegates::OnObjectPropertyChanged 回调）。
 */
class FAtlasGroupHelper
{
public:
    /** 一键执行：遍历所有 Atlas 目录，设置 Sprite 的 AtlasGroup 到同级 Atlas.uasset */
    static void RunAll();

    /**
     * 确保指定内容目录（如 /Game/AssetsPackage/UI/UICommon）下存在 Atlas（UPaperSpriteAtlas），
     * 不存在则创建并保存，返回图集资产（失败返回 nullptr）。
     */
    static UPaperSpriteAtlas* EnsureAtlasAsset(const FString& InParentContentPath);

private:
    /** 收集所有名为 Atlas 的内容目录（递归 /Game/AssetsPackage） */
    static TArray<FString> FindAllAtlasFolders();

    /** 将 Atlas 目录下所有 UPaperSprite 的 AtlasGroup 指向 InAtlas，返回设置个数 */
    static int32 SetAtlasGroupForFolder(const FString& InAtlasFolderPath, UPaperSpriteAtlas* InAtlas);

    /** 保存指定包 */
    static void SavePackage(UPackage* InPackage);
};
