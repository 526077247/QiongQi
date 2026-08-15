// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * QiongQi 打包工具模块：
 * 提供"QiongQi 打包面板"（渠道/版本/平台/全量/整包/打包类型）等编辑器工具。
 */
class FQiongQiPackToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** 打开打包面板 */
	static void OpenPackPanel();

private:
	void RegisterMenus();
};
