// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class QiongQi : ModuleRules
{
	public QiongQi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" , "JsEnv","Puerts" ,
			// �������ģ��֧����Դ����
            "AssetRegistry",      // ֧����Դע����첽����
            "Paper2D",            // ֧��PaperSprite��Դ
            "Slate",              // ֧��UMG��Slate UI
            "SlateCore",          // ֧��UMG��Slate UI
            "UMG",                // ֧��UMG�ؼ�
			//"StreamableManager"   // ��ȷ������ʽ������֧��
		});


        PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
