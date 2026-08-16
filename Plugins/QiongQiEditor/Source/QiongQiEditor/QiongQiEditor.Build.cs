using UnrealBuildTool;

public class QiongQiEditor : ModuleRules
{
    public QiongQiEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] { "Core" });

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "InputCore",
            "UMG",
            "UMGEditor",
            "UnrealEd",
            "AssetTools",
            "Projects",
            "ToolMenus",
            "ApplicationCore",
            "Kismet",
            "BlueprintGraph",
            "ContentBrowser",
            "ContentBrowserData",
            "AssetRegistry",
            "Paper2D",
        });
    }
}
