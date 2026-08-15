// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class FPackProcessWorker;
class SButton;
class SCheckBox;
class SEditableTextBox;
class SScrollBox;
class STextBlock;

template <typename OptionType>
class SComboBox;

/**
 * QiongQi 打包面板
 *
 * 手动填写：
 *  - 渠道名称（输入框）
 *  - 资源版本（打开时默认填入当前 Unix 时间戳）
 *  - 打包目标平台（下拉：Windows / Android / IOS）
 *  - 是否全量资源打入首包（勾选）
 *  - 是否打整包（勾选，默认勾选；勾选时隐藏历史版本选择）
 *  - 历史版本（下拉：不打整包时作为补丁主版本，默认选本地最新）
 *  - 打包类型（下拉：Debug / Release）
 *
 * 流程：
 *  1. 勾选"是否打整包" -> 先执行 UAT BuildCookRun 产出完整可运行包，资源版本 = 面板版本；
 *  2. 生成 HotPatcher 配置 -> 执行 UnrealEditor-Cmd -run=HotPatcher 产出补丁 Pak。
 *     - 勾选"是否打整包"：主版本按"是否全量资源打入首包"决定（全量 / 自动检测最新基础版本）。
 *     - 未勾选"是否打整包"：必须选择本地历史版本作为主版本，patch 版本 = 面板资源版本。
 */
class SPackPanelWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPackPanelWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SPackPanelWidget() override;

	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

private:
	enum class EPackStage
	{
		Idle,
		UATBuild,   // 整包：UAT BuildCookRun
		HotPatcher, // 补丁：HotPatcher 全量 / 增量
		Done,
		Failed,
	};

	// ---- UI 控件 ----
	TSharedPtr<SEditableTextBox> ChannelNameBox;
	TSharedPtr<SEditableTextBox> ResourceVersionBox;

	TArray<TSharedPtr<FString>> PlatformOptions;
	TArray<TSharedPtr<FString>> PackTypeOptions;
	/** 本地可选历史版本（不打整包时作为主版本），与 BaseVersionPaths 一一对应 */
	TArray<TSharedPtr<FString>> BaseVersionOptions;
	TArray<FString> BaseVersionPaths;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> PlatformComboBox;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> PackTypeComboBox;
	TSharedPtr<SComboBox<TSharedPtr<FString>>> BaseVersionComboBox;
	TSharedPtr<STextBlock> PlatformComboText;
	TSharedPtr<STextBlock> PackTypeComboText;
	TSharedPtr<STextBlock> BaseVersionComboText;
	/** 历史版本（主版本）行的标签，勾选打整包时随下拉框一起隐藏 */
	TSharedPtr<STextBlock> BaseVersionLabel;

	TSharedPtr<SCheckBox> FullInFirstPakCheckBox;
	TSharedPtr<SCheckBox> FullPackageCheckBox;

	TSharedPtr<SScrollBox> LogScrollBox;
	TSharedPtr<SButton> StartButton;
	TSharedPtr<SButton> CancelButton;
	TSharedPtr<SButton> CopyLogButton;

	/** 日志缓存（用于"复制日志"按钮） */
	FString LogBuffer;

	// ---- 面板状态 ----
	FString ChannelName;
	FString ResourceVersion;
	FString SelectedPlatform; // Windows / Android / IOS
	FString SelectedPackType; // Debug / Release
	/** 当前选中的历史版本（版本名，不打整包时作为补丁主版本） */
	FString SelectedBaseVersion;
	/** 当前选中的历史版本完整文件路径 */
	FString SelectedBaseVersionPath;
	bool bFullInFirstPak = true;
	bool bFullPackage = false;
	/** 是否因本地无历史版本而锁定"是否打整包"选项 */
	bool bFullPackageLocked = false;

	EPackStage CurrentStage = EPackStage::Idle;
	TSharedPtr<FPackProcessWorker> ProcessWorker;

	// ---- 回调 ----
	void OnStartPackClicked();
	void OnCancelClicked();
	void OnCopyLogClicked();
	void OnPlatformSelected(TSharedPtr<FString> InValue, ESelectInfo::Type InSelectInfo);
	void OnPackTypeSelected(TSharedPtr<FString> InValue, ESelectInfo::Type InSelectInfo);
	void OnBaseVersionSelected(TSharedPtr<FString> InValue, ESelectInfo::Type InSelectInfo);

	// ---- 流程 ----
	void RunUATStage();
	void RunHotPatcherStage();
	void HandleProcessOutput();
	void FinishPack(bool bSuccess);
	void SetProcessing(bool bProcessing);

	/** 打包成功后把本次 CDN 资源与整包复制到 Release 目录 */
	void CopyResultsToRelease();

	/** UAT 中间输出根目录（Build/Pack/Output/{渠道}） */
	FString GetOutputRootPath() const;
	/** HotPatcher 资源输出根目录（{项目}/HotPatcherRes/{渠道}_{平台}） */
	FString GetHotPatcherOutputRoot() const;
	FString GetHotPatcherPlatformName() const;
	FString GetUATPlatformName() const;
	FString GetClientConfigName() const;

	/** 检测当前编辑器进程是否处于 Live Coding 会话活跃状态（命名 Mutex，与 UBT 判定一致） */
	static bool IsLiveCodingActive();
	bool GenerateHotPatcherConfig(FString& OutConfigPath);
	FString FindLatestBaseVersionJson(const FString& SaveAbsPath) const;
	/** 扫描本地 HotPatcher 输出目录中的历史版本，刷新下拉选项并默认选中最新 */
	void RefreshBaseVersionOptions();
	/** 无历史版本时强制只能打整包（补丁模式缺少主版本无法执行），恢复可用后解锁 */
	void UpdateFullPackageConstraint();
	/** 按当前"是否打整包"状态显示/隐藏历史版本（主版本）选择行 */
	void UpdateBaseVersionVisibility();
	/** 按当前状态更新"开始打包"按钮可用性（不打整包且未选主版本时置灰） */
	void UpdateStartButtonState(bool bProcessing);

	// ---- UI 辅助 ----
	void AppendLog(const FString& Text, const FLinearColor& Color = FLinearColor::White);
};
