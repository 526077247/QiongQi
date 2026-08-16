// Copyright Epic Games, Inc. All Rights Reserved.

#include "QiongQiPackTools.h"

#include "PackPanel/SPackPanelWidget.h"
#include "TargetPlatformRegister.h"
#include "ToolMenus.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Widgets/SWindow.h"

#define LOCTEXT_NAMESPACE "FQiongQiPackToolsModule"

void FQiongQiPackToolsModule::StartupModule()
{
	// 确保 HotPatcher 的目标平台枚举（Windows/Android/IOS）已在当前进程注册
	GetDefault<UTargetPlatformRegister>();

	if (!IsRunningCommandlet())
	{
		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FQiongQiPackToolsModule::RegisterMenus));
	}
}

void FQiongQiPackToolsModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
}

void FQiongQiPackToolsModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* WindowMenu = UToolMenus::Get()->FindMenu("LevelEditor.MainMenu.Window");
	if (!WindowMenu)
	{
		return;
	}

	FToolMenuSection& Section = WindowMenu->FindOrAddSection("QiongQiTools",
		LOCTEXT("QiongQiToolsSection", "QiongQi 工具"));

	Section.AddMenuEntry(
		"QiongQiPackPanel",
		LOCTEXT("QiongQiPackPanelLabel", "QiongQi 打包面板"),
		LOCTEXT("QiongQiPackPanelToolTip", "打开 QiongQi 打包面板"),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateStatic(&FQiongQiPackToolsModule::OpenPackPanel)));
}

void FQiongQiPackToolsModule::OpenPackPanel()
{
	TSharedRef<SWindow> Window = SNew(SWindow)
		.Title(LOCTEXT("QiongQiPackPanelWindowTitle", "QiongQi 打包面板"))
		.ClientSize(FVector2D(880, 800))
		.SupportsMaximize(false)
		.SupportsMinimize(false);

	Window->SetContent(SNew(SPackPanelWidget));

	FSlateApplication::Get().AddWindow(Window);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FQiongQiPackToolsModule, QiongQiPackTools)
