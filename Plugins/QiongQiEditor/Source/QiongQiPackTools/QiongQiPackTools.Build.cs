// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class QiongQiPackTools : ModuleRules
{
	public QiongQiPackTools(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"Projects",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"DesktopPlatform",
			"EditorStyle",
			"WorkspaceMenuStructure",
			// FPlatformApplicationMisc::ClipboardCopy 等平台剪贴板接口
			"ApplicationCore",
			// HotPatcher 序列化需要
			"Json",
			"JsonUtilities",
			// HotPatcher 相关
			"HotPatcherRuntime",
			"HotPatcherCore",
		});
	}
}
