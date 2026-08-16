// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class QiongQi : ModuleRules
{
	public QiongQi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" , "JsEnv","Puerts" ,
			// 添加以下模块支持资源管理
            "AssetRegistry",      // 支持资源注册和异步加载
            "Paper2D",            // 支持PaperSprite资源
            "Slate",              // 支持UMG和Slate UI
            "SlateCore",          // 支持UMG和Slate UI
            "UMG",                // 支持UMG控件
            "HTTP",               // 支持 HTTP 请求（UeHttpHelper）
            "Json",               // 支持 JSON 读写（UeDownloadHelper 版本清单）
			//"StreamableManager"   // 明确包含流式管理器支持
		});

		// 运行时插件依赖：RuntimeFilesDownloader（CDN 资源下载）+ HotPatcherRuntime（pak 挂载）
		PublicDependencyModuleNames.AddRange(new string[] {
			"RuntimeFilesDownloader",
			"HotPatcherRuntime"
		});


        PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
