// Copyright Epic Games, Inc. All Rights Reserved.

#include "PackPanel/SPackPanelWidget.h"

#include "PackPanel/FPackProcessWorker.h"

// HotPatcher
#include "CreatePatch/FExportPatchSettings.h"
#include "FlibHotPatcherCoreHelper.h"
#include "FlibPatchParserHelper.h"
#include "HotPatcherSettings.h"
#include "Templates/HotPatcherTemplateHelper.hpp"

// Engine / Slate
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Windows API（检测 Live Coding Mutex）
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SPackPanelWidget"

void SPackPanelWidget::Construct(const FArguments& InArgs)
{
	// 默认值
	ChannelName = TEXT("Default");
	ResourceVersion = LexToString(FDateTime::UtcNow().ToUnixTimestamp());
	SelectedPlatform = TEXT("Windows");
	SelectedPackType = TEXT("Release");
	bFullInFirstPak = true;
	bFullPackage = true;

	PlatformOptions.Add(MakeShareable(new FString(TEXT("Windows"))));
	PlatformOptions.Add(MakeShareable(new FString(TEXT("Android"))));
	PlatformOptions.Add(MakeShareable(new FString(TEXT("IOS"))));

	PackTypeOptions.Add(MakeShareable(new FString(TEXT("Debug"))));
	PackTypeOptions.Add(MakeShareable(new FString(TEXT("Release"))));

	ChildSlot
	[
		SNew(SBorder)
		.Padding(FMargin(16.f))
		[
			SNew(SVerticalBox)

			// 标题
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 12.f)
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("QiongQi 打包面板")))
				.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 18))
				.ColorAndOpacity(FSlateColor(FLinearColor(0.15f, 0.68f, 1.f)))
			]

			// 参数表单
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SGridPanel)
				.FillColumn(1, 1.f)

				// 渠道名称
				+ SGridPanel::Slot(0, 0)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("渠道名称：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 0)
				.Padding(4.f, 4.f)
				[
					SAssignNew(ChannelNameBox, SEditableTextBox)
					.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
					.Text(FText::FromString(ChannelName))
				]

				// 资源版本（默认当前 Unix 时间戳）
				+ SGridPanel::Slot(0, 1)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("资源版本：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 1)
				.Padding(4.f, 4.f)
				[
					SAssignNew(ResourceVersionBox, SEditableTextBox)
					.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
					.Text(FText::FromString(ResourceVersion))
				]

				// 打包目标平台
				+ SGridPanel::Slot(0, 2)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("打包目标平台：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 2)
				.Padding(4.f, 4.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&PlatformOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						return SNew(STextBlock)
							.Text(FText::FromString(*InItem));
					})
					.OnSelectionChanged(this, &SPackPanelWidget::OnPlatformSelected)
					[
						SAssignNew(PlatformComboText, STextBlock)
						.Text(FText::FromString(SelectedPlatform))
					]
				]

				// 是否全量资源打入首包
				+ SGridPanel::Slot(0, 3)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("是否全量资源打入首包：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 3)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SAssignNew(FullInFirstPakCheckBox, SCheckBox)
					.IsChecked(ECheckBoxState::Checked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState InState)
					{
						bFullInFirstPak = (InState == ECheckBoxState::Checked);
					})
				]

				// 是否打整包
				+ SGridPanel::Slot(0, 4)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("是否打整包：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 4)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SAssignNew(FullPackageCheckBox, SCheckBox)
					.IsChecked(ECheckBoxState::Checked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState InState)
					{
						bFullPackage = (InState == ECheckBoxState::Checked);
						// 打整包时不需要选择历史版本，隐藏该行
						UpdateBaseVersionVisibility();
						UpdateStartButtonState(false);
					})
				]

				// 历史版本（不打整包时作为补丁主版本，默认选最新；打整包时整行隐藏）
				+ SGridPanel::Slot(0, 5)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SAssignNew(BaseVersionLabel, STextBlock)
					.Text(FText::FromString(TEXT("历史版本（主版本）：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 5)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SAssignNew(BaseVersionComboBox, SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&BaseVersionOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						return SNew(STextBlock)
							.Text(FText::FromString(*InItem));
					})
					.OnSelectionChanged(this, &SPackPanelWidget::OnBaseVersionSelected)
					[
						SAssignNew(BaseVersionComboText, STextBlock)
						.Text(FText::FromString(TEXT("无历史版本")))
					]
				]

				// 打包类型
				+ SGridPanel::Slot(0, 6)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("打包类型：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 6)
				.Padding(4.f, 4.f)
				[
					SNew(SComboBox<TSharedPtr<FString>>)
					.OptionsSource(&PackTypeOptions)
					.OnGenerateWidget_Lambda([](TSharedPtr<FString> InItem)
					{
						return SNew(STextBlock)
							.Text(FText::FromString(*InItem));
					})
					.OnSelectionChanged(this, &SPackPanelWidget::OnPackTypeSelected)
					[
						SAssignNew(PackTypeComboText, STextBlock)
						.Text(FText::FromString(SelectedPackType))
					]
				]
			]

			// 日志输出
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 8.f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.03f, 0.03f, 0.03f, 0.65f))
				.Padding(FMargin(8.f, 6.f))
				[
					SAssignNew(LogScrollBox, SScrollBox)
				]
			]

			// 操作按钮
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign(HAlign_Right)
			.Padding(0.f, 4.f, 0.f, 0.f)
			[
				SNew(SHorizontalBox)

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.f, 0.f)
				[
					SAssignNew(CopyLogButton, SButton)
					.Text(FText::FromString(TEXT("复制日志")))
					.OnClicked_Lambda([this]()
					{
						OnCopyLogClicked();
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.f, 0.f)
				[
					SAssignNew(CancelButton, SButton)
					.Text(FText::FromString(TEXT("取消")))
					.IsEnabled(false)
					.OnClicked_Lambda([this]()
					{
						OnCancelClicked();
						return FReply::Handled();
					})
				]

				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.f, 0.f)
				[
					SAssignNew(StartButton, SButton)
					.Text(FText::FromString(TEXT("开始打包")))
					.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton"))
					.OnClicked_Lambda([this]()
					{
						OnStartPackClicked();
						return FReply::Handled();
					})
				]
			]
		]
	];

	AppendLog(TEXT("欢迎使用 QiongQi 打包面板。"), FLinearColor(0.f, 1.f, 1.f));
	AppendLog(TEXT("提示：勾选\"是否打整包\"将先执行 UAT 构建完整包，再生成补丁。"), FLinearColor(0.6f, 0.6f, 0.6f));
	AppendLog(TEXT("提示：不勾选\"是否打整包\"时，需选择本地历史版本作为补丁主版本。"), FLinearColor(0.6f, 0.6f, 0.6f));

	// 扫描本地历史版本填充下拉框（默认选中最新）
	RefreshBaseVersionOptions();
}

SPackPanelWidget::~SPackPanelWidget()
{
	if (ProcessWorker.IsValid())
	{
		ProcessWorker->StopProcess();
		ProcessWorker.Reset();
	}
}

void SPackPanelWidget::Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (ProcessWorker.IsValid())
	{
		HandleProcessOutput();
	}
}

// ---------------------------------------------------------------------------------------------
// 回调
// ---------------------------------------------------------------------------------------------

void SPackPanelWidget::OnStartPackClicked()
{
	ChannelName = ChannelNameBox.IsValid() ? ChannelNameBox->GetText().ToString().TrimStartAndEnd() : ChannelName;
	ResourceVersion = ResourceVersionBox.IsValid() ? ResourceVersionBox->GetText().ToString().TrimStartAndEnd() : ResourceVersion;

	if (ChannelName.IsEmpty())
	{
		AppendLog(TEXT("[错误] 请填写渠道名称！"), FLinearColor::Red);
		return;
	}
	if (ResourceVersion.IsEmpty())
	{
		AppendLog(TEXT("[错误] 请填写资源版本！"), FLinearColor::Red);
		return;
	}
	if (SelectedPlatform.IsEmpty() || SelectedPackType.IsEmpty())
	{
		AppendLog(TEXT("[错误] 请选择打包目标平台与打包类型！"), FLinearColor::Red);
		return;
	}

	bFullInFirstPak = FullInFirstPakCheckBox.IsValid() ? FullInFirstPakCheckBox->IsChecked() : bFullInFirstPak;
	bFullPackage = FullPackageCheckBox.IsValid() ? FullPackageCheckBox->IsChecked() : bFullPackage;

	// 不打整包（纯补丁模式）：必须选择本地历史版本作为主版本
	if (!bFullPackage)
	{
		// 渠道/平台可能已变化，重新扫描本地历史版本（保留当前选中项）
		RefreshBaseVersionOptions();
		if (SelectedBaseVersion.IsEmpty())
		{
			AppendLog(TEXT("[错误] 未勾选\"是否打整包\"，且本地无历史版本可作为补丁主版本，无法打包！"), FLinearColor::Red);
			AppendLog(TEXT("[错误] 请先勾选\"是否打整包\"执行一次全量打包，或在历史版本下拉框中选择主版本。"), FLinearColor::Red);
			return;
		}
	}

	AppendLog(TEXT(""));
	AppendLog(TEXT("============== QiongQi 打包开始 =============="), FLinearColor::Green);
	AppendLog(FString::Printf(TEXT("[配置] 渠道: %s    版本: %s"), *ChannelName, *ResourceVersion));
	AppendLog(FString::Printf(TEXT("[配置] 平台: %s    类型: %s"), *SelectedPlatform, *SelectedPackType));
	AppendLog(FString::Printf(TEXT("[配置] 全量资源打入首包: %s    打整包: %s"),
		bFullInFirstPak ? TEXT("是") : TEXT("否"),
		bFullPackage ? TEXT("是") : TEXT("否")));
	if (bFullPackage)
	{
		AppendLog(FString::Printf(TEXT("[配置] 整包资源版本: %s"), *ResourceVersion));
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[配置] 补丁版本: %s    主版本(历史版本): %s"),
			*ResourceVersion, *SelectedBaseVersion));
	}
	AppendLog(FString::Printf(TEXT("[配置] HotPatcher 输出目录: %s"), *GetHotPatcherOutputRoot()));
	if (bFullPackage)
	{
		AppendLog(FString::Printf(TEXT("[配置] UAT 整包输出目录: %s"),
			*FPaths::ConvertRelativePathToFull(FPaths::Combine(GetOutputRootPath(), SelectedPlatform, TEXT("FullPackage")))));
	}
	AppendLog(FString::Printf(TEXT("[配置] Release 交付目录: %s"),
		*FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Release")))));

	// 打包前检测 Live Coding 会话：编辑器运行时 UBT 会因 Live Coding 活跃拒绝构建。
	// 运行中的编辑器无法彻底关闭 Live Coding（互斥量由引擎进程持有直到退出），
	// 从编辑器内打包时自动在 UAT 构建参数中附加 -NoHotReloadFromIDE，让 UBT 跳过该阻塞检查。
	AppendLog(TEXT("[信息] 从编辑器内启动打包，已自动为 UAT 构建附加 -NoHotReloadFromIDE，跳过 Live Coding 阻塞检查。"), FLinearColor::Yellow);

	SetProcessing(true);

	if (bFullPackage)
	{
		RunUATStage();
	}
	else
	{
		RunHotPatcherStage();
	}
}

void SPackPanelWidget::OnCancelClicked()
{
	if (ProcessWorker.IsValid())
	{
		AppendLog(TEXT("[信息] 正在终止打包进程..."), FLinearColor::Yellow);
		ProcessWorker->StopProcess();
		ProcessWorker.Reset();
	}

	CurrentStage = EPackStage::Idle;
	SetProcessing(false);
	AppendLog(TEXT("[打包] 已取消。"), FLinearColor::Yellow);
}

void SPackPanelWidget::OnPlatformSelected(TSharedPtr<FString> InValue, ESelectInfo::Type InSelectInfo)
{
	if (InValue.IsValid())
	{
		SelectedPlatform = *InValue;
		if (PlatformComboText.IsValid())
		{
			PlatformComboText->SetText(FText::FromString(SelectedPlatform));
		}
		// 平台变化后历史版本目录也随之变化，刷新下拉选项
		RefreshBaseVersionOptions();
	}
}

void SPackPanelWidget::OnPackTypeSelected(TSharedPtr<FString> InValue, ESelectInfo::Type InSelectInfo)
{
	if (InValue.IsValid())
	{
		SelectedPackType = *InValue;
		if (PackTypeComboText.IsValid())
		{
			PackTypeComboText->SetText(FText::FromString(SelectedPackType));
		}
	}
}

void SPackPanelWidget::OnBaseVersionSelected(TSharedPtr<FString> InValue, ESelectInfo::Type InSelectInfo)
{
	if (!InValue.IsValid())
	{
		return;
	}

	SelectedBaseVersion = *InValue;
	SelectedBaseVersionPath.Empty();
	for (int32 Idx = 0; Idx < BaseVersionOptions.Num(); ++Idx)
	{
		if (BaseVersionOptions[Idx].IsValid() && *BaseVersionOptions[Idx] == SelectedBaseVersion)
		{
			SelectedBaseVersionPath = BaseVersionPaths[Idx];
			break;
		}
	}

	if (BaseVersionComboText.IsValid())
	{
		BaseVersionComboText->SetText(FText::FromString(SelectedBaseVersion));
	}

	// 已选择主版本后允许开始打包（不打整包模式下）
	UpdateStartButtonState(false);
}

void SPackPanelWidget::RefreshBaseVersionOptions()
{
	// 保留用户当前选择：渠道/平台未变时保持选择，否则回退到最新版本
	const FString PreviousSelection = SelectedBaseVersion;

	BaseVersionOptions.Empty();
	BaseVersionPaths.Empty();
	SelectedBaseVersion.Empty();
	SelectedBaseVersionPath.Empty();

	const FString SaveAbsPath = GetHotPatcherOutputRoot();
	if (!FPaths::DirectoryExists(SaveAbsPath))
	{
		if (BaseVersionComboText.IsValid())
		{
			BaseVersionComboText->SetText(FText::FromString(TEXT("无历史版本")));
		}
		// 无历史版本时强制只能打整包，并同步历史版本行显示与按钮状态
		UpdateFullPackageConstraint();
		return;
	}

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFilesRecursive(FoundFiles, *SaveAbsPath, TEXT("*.json"), true, false);

	// 收集 {版本Id} -> 文件路径，按修改时间倒序（最新在前）
	struct FBaseVersionInfo
	{
		FString Name;
		FString Path;
		FDateTime Time;
	};
	TArray<FBaseVersionInfo> Versions;

	for (const FString& File : FoundFiles)
	{
		const FString FileName = FPaths::GetCleanFilename(File);

		// 跳过补丁配置
		if (FileName.Contains(TEXT("PatchConfig")))
		{
			continue;
		}

		// 版本文件命名规则: {版本Id}.json / {版本Id}_Release.json，位于 {版本Id}/ 目录下
		const FString ParentDirName = FPaths::GetCleanFilename(FPaths::GetPath(File));
		if (ParentDirName.IsEmpty() || ParentDirName.Contains(ResourceVersion))
		{
			continue;
		}
		if (!FileName.StartsWith(ParentDirName))
		{
			continue;
		}

		FBaseVersionInfo Info;
		Info.Name = ParentDirName;
		Info.Path = File;
		Info.Time = IFileManager::Get().GetTimeStamp(*File);
		Versions.Add(Info);
	}

	Versions.Sort([](const FBaseVersionInfo& A, const FBaseVersionInfo& B)
	{
		return A.Time > B.Time;
	});

	// 去重：同一版本目录下可能有 {版本Id}.json 与 {版本Id}_Release.json，取最新时间戳的
	TSet<FString> SeenNames;
	for (const FBaseVersionInfo& Info : Versions)
	{
		if (SeenNames.Contains(Info.Name))
		{
			continue;
		}
		SeenNames.Add(Info.Name);

		BaseVersionOptions.Add(MakeShareable(new FString(Info.Name)));
		BaseVersionPaths.Add(Info.Path);
	}

	// 默认选中：优先保持用户原选择，否则选最新（排序后的第一项）
	if (BaseVersionOptions.Num() > 0)
	{
		int32 DefaultIdx = 0;
		if (!PreviousSelection.IsEmpty())
		{
			for (int32 Idx = 0; Idx < BaseVersionOptions.Num(); ++Idx)
			{
				if (BaseVersionOptions[Idx].IsValid() && *BaseVersionOptions[Idx] == PreviousSelection)
				{
					DefaultIdx = Idx;
					break;
				}
			}
		}
		SelectedBaseVersion = *BaseVersionOptions[DefaultIdx];
		SelectedBaseVersionPath = BaseVersionPaths[DefaultIdx];
	}

	if (BaseVersionComboBox.IsValid())
	{
		BaseVersionComboBox->RefreshOptions();
	}
	if (BaseVersionComboText.IsValid())
	{
		BaseVersionComboText->SetText(BaseVersionOptions.Num() > 0
			? FText::FromString(SelectedBaseVersion)
			: FText::FromString(TEXT("无历史版本")));
	}

	// 无历史版本时强制只能打整包
	UpdateFullPackageConstraint();
}

void SPackPanelWidget::UpdateFullPackageConstraint()
{
	if (!FullPackageCheckBox.IsValid())
	{
		return;
	}

	const bool bHasBaseVersion = (BaseVersionOptions.Num() > 0);
	if (!bHasBaseVersion)
	{
		// 补丁模式需要本地历史版本作为主版本，无历史版本时锁定为打整包
		bFullPackage = true;
		FullPackageCheckBox->SetIsChecked(ECheckBoxState::Checked);
		FullPackageCheckBox->SetEnabled(false);
		if (!bFullPackageLocked)
		{
			bFullPackageLocked = true;
			AppendLog(TEXT("[提示] 本地无历史版本，已锁定\"是否打整包\"为勾选（补丁模式需历史版本作为主版本）。"), FLinearColor::Yellow);
		}
	}
	else
	{
		FullPackageCheckBox->SetEnabled(true);
		if (bFullPackageLocked)
		{
			bFullPackageLocked = false;
			AppendLog(TEXT("[提示] 检测到本地历史版本，已解锁\"是否打整包\"，可切换为补丁模式。"), FLinearColor::Yellow);
		}
	}

	// 同步历史版本选择行的显示与"开始打包"按钮状态
	UpdateBaseVersionVisibility();
	UpdateStartButtonState(false);
}

void SPackPanelWidget::UpdateBaseVersionVisibility()
{
	const EVisibility NewVisibility = bFullPackage ? EVisibility::Collapsed : EVisibility::Visible;
	if (BaseVersionLabel.IsValid())
	{
		BaseVersionLabel->SetVisibility(NewVisibility);
	}
	if (BaseVersionComboBox.IsValid())
	{
		BaseVersionComboBox->SetVisibility(NewVisibility);
	}
}

void SPackPanelWidget::UpdateStartButtonState(bool bProcessing)
{
	if (!StartButton.IsValid())
	{
		return;
	}

	// 不打整包（纯补丁模式）且未选择历史版本作为主版本时，禁止开始打包
	const bool bCanStart = !bProcessing && (bFullPackage || !SelectedBaseVersion.IsEmpty());
	StartButton->SetEnabled(bCanStart);
}

void SPackPanelWidget::OnCopyLogClicked()
{
	if (LogBuffer.IsEmpty())
	{
		return;
	}

	FPlatformApplicationMisc::ClipboardCopy(*LogBuffer);
	AppendLog(FString::Printf(TEXT("[信息] 已将日志复制到剪贴板（共 %d 字符）。"), LogBuffer.Len()), FLinearColor(0.f, 1.f, 1.f));
}

// ---------------------------------------------------------------------------------------------
// 打包流程
// ---------------------------------------------------------------------------------------------

void SPackPanelWidget::RunUATStage()
{
	CurrentStage = EPackStage::UATBuild;
	AppendLog(TEXT(""));
	AppendLog(TEXT("============== 整包构建 (UAT BuildCookRun) =============="), FLinearColor(0.f, 1.f, 1.f));

	const FString RunUATPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::EngineDir(), TEXT("Build/BatchFiles/RunUAT.bat")));
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	const FString ArchiveDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(GetOutputRootPath(), SelectedPlatform, TEXT("FullPackage")));

	FString UATArgs = FString::Printf(
		TEXT("BuildCookRun -project=\"%s\" -noP4 -platform=%s -clientconfig=%s -build -cook -stage -pak -archive -archivedirectory=\"%s\""),
		*ProjectPath,
		*GetUATPlatformName(),
		*GetClientConfigName(),
		*ArchiveDir);

	// 从编辑器内启动打包时，当前编辑器必然正在运行，必须让 UBT 跳过 Live Coding 阻塞检查，
	// 否则 UBT 检测到项目目标 exe 的 LiveCoding 互斥量后会直接报错退出。
	// 注意：参数值没有空格，不要加引号；cmd.exe 的嵌套引号会导致 UAT 收不到该参数。
	UATArgs += TEXT(" -ubtargs=-NoHotReloadFromIDE");

	const FString Params = FString::Printf(TEXT("/c \"\"%s\" %s\""), *RunUATPath, *UATArgs);
	AppendLog(FString::Printf(TEXT(">>> 执行: cmd.exe %s"), *Params));

	ProcessWorker = MakeShareable(new FPackProcessWorker(
		TEXT("cmd.exe"),
		Params,
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())));

	if (!ProcessWorker->Start())
	{
		AppendLog(TEXT("[错误] 无法启动 UAT 进程，请检查引擎路径！"), FLinearColor::Red);
		ProcessWorker.Reset();
		FinishPack(false);
	}
}

void SPackPanelWidget::RunHotPatcherStage()
{
	CurrentStage = EPackStage::HotPatcher;
	AppendLog(TEXT(""));
	AppendLog(TEXT("============== 生成补丁 (HotPatcher) =============="), FLinearColor(0.f, 1.f, 1.f));

	FString ConfigPath;
	if (!GenerateHotPatcherConfig(ConfigPath) || ConfigPath.IsEmpty())
	{
		AppendLog(TEXT("[错误] 生成 HotPatcher 配置失败！"), FLinearColor::Red);
		FinishPack(false);
		return;
	}

	const FString UECmdBinary = UFlibHotPatcherCoreHelper::GetUECmdBinary();
	const FString ProjectPath = UFlibPatchParserHelper::GetProjectFilePath();

	const FString MissionCommand = FString::Printf(
		TEXT("\"%s\" -run=HotPatcher -config=\"%s\""),
		*ProjectPath,
		*ConfigPath);

	AppendLog(FString::Printf(TEXT(">>> 执行: %s %s"), *UECmdBinary, *MissionCommand));

	ProcessWorker = MakeShareable(new FPackProcessWorker(
		UECmdBinary,
		MissionCommand,
		FPaths::ConvertRelativePathToFull(FPaths::ProjectDir())));

	if (!ProcessWorker->Start())
	{
		AppendLog(TEXT("[错误] 无法启动 HotPatcher 进程！"), FLinearColor::Red);
		ProcessWorker.Reset();
		FinishPack(false);
	}
}

void SPackPanelWidget::HandleProcessOutput()
{
	if (!ProcessWorker.IsValid())
	{
		return;
	}

	// 先取尽所有待处理输出
	FString Line;
	while (ProcessWorker->OutputQueue.Dequeue(Line))
	{
		AppendLog(Line);
	}

	// 进程仍在运行
	if (ProcessWorker->IsRunning())
	{
		return;
	}

	const int32 ExitCode = ProcessWorker->GetExitCode();
	ProcessWorker->StopProcess();
	ProcessWorker.Reset();

	AppendLog(FString::Printf(TEXT(">>> 进程结束，退出码: %d"), ExitCode));

	const bool bSuccess = (ExitCode == 0);

	if (CurrentStage == EPackStage::UATBuild)
	{
		if (bSuccess)
		{
			AppendLog(TEXT("[打包] 整包构建完成，开始生成补丁..."), FLinearColor(0.f, 1.f, 1.f));
			RunHotPatcherStage();
		}
		else
		{
			FinishPack(false);
		}
	}
	else if (CurrentStage == EPackStage::HotPatcher)
	{
		FinishPack(bSuccess);
	}
	else
	{
		FinishPack(bSuccess);
	}
}

void SPackPanelWidget::FinishPack(bool bSuccess)
{
	if (bSuccess)
	{
		CurrentStage = EPackStage::Done;
		AppendLog(TEXT(""));
		AppendLog(TEXT("[打包] 全部流程完成！"), FLinearColor::Green);
		CopyResultsToRelease();
	}
	else
	{
		CurrentStage = EPackStage::Failed;
		AppendLog(TEXT(""));
		AppendLog(TEXT("[打包] 流程失败，请查看上方日志。"), FLinearColor::Red);
	}

	SetProcessing(false);
}

void SPackPanelWidget::SetProcessing(bool bProcessing)
{
	if (ChannelNameBox.IsValid()) { ChannelNameBox->SetEnabled(!bProcessing); }
	if (ResourceVersionBox.IsValid()) { ResourceVersionBox->SetEnabled(!bProcessing); }
	if (PlatformComboBox.IsValid()) { PlatformComboBox->SetEnabled(!bProcessing); }
	if (PackTypeComboBox.IsValid()) { PackTypeComboBox->SetEnabled(!bProcessing); }
	if (FullInFirstPakCheckBox.IsValid()) { FullInFirstPakCheckBox->SetEnabled(!bProcessing); }
	if (FullPackageCheckBox.IsValid()) { FullPackageCheckBox->SetEnabled(!bProcessing); }
	UpdateStartButtonState(bProcessing);
	if (CancelButton.IsValid()) { CancelButton->SetEnabled(bProcessing); }

	// 打包结束恢复操作时，重新应用"无历史版本只能打整包"约束
	if (!bProcessing)
	{
		UpdateFullPackageConstraint();
	}
}

// ---------------------------------------------------------------------------------------------
// 辅助
// ---------------------------------------------------------------------------------------------

FString SPackPanelWidget::GetOutputRootPath() const
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Build/Pack/Output"), ChannelName));
}

FString SPackPanelWidget::GetHotPatcherOutputRoot() const
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("HotPatcherRes"),
			FString::Printf(TEXT("%s_%s"), *ChannelName, *GetHotPatcherPlatformName())));
}

void SPackPanelWidget::CopyResultsToRelease()
{
	const FString ReleaseRoot = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Release")));
	const FString ChannelPlatform = FString::Printf(TEXT("%s_%s"), *ChannelName, *GetHotPatcherPlatformName());
	const FString CdnDestDir = FPaths::Combine(ReleaseRoot, ChannelPlatform);
	const FString SaveRoot = GetHotPatcherOutputRoot();

	AppendLog(TEXT(""));
	AppendLog(TEXT("============== 复制到 Release 目录 =============="), FLinearColor(0.f, 1.f, 1.f));

	// 1. 复制本次 HotPatcher 产生的 CDN 资源：{版本}/ 目录 + {版本}_Release.json
	const FString VersionDir = FPaths::Combine(SaveRoot, ResourceVersion);
	if (FPaths::DirectoryExists(VersionDir))
	{
		IFileManager::Get().MakeDirectory(*CdnDestDir, true);
		if (FPlatformFileManager::Get().GetPlatformFile().CopyDirectoryTree(*CdnDestDir, *VersionDir, true))
		{
			AppendLog(FString::Printf(TEXT("[复制] CDN 资源已复制: %s -> %s"), *VersionDir, *CdnDestDir), FLinearColor(0.f, 1.f, 0.f));
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[错误] 复制 CDN 资源失败: %s -> %s"), *VersionDir, *CdnDestDir), FLinearColor::Red);
		}
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[警告] 未找到本次版本资源目录: %s"), *VersionDir), FLinearColor::Yellow);
	}

	// 版本描述 json（若存在）
	const FString VersionJson = FPaths::Combine(SaveRoot, FString::Printf(TEXT("%s_Release.json"), *ResourceVersion));
	if (FPaths::FileExists(VersionJson))
	{
		const FString DestJson = FPaths::Combine(CdnDestDir, FString::Printf(TEXT("%s_Release.json"), *ResourceVersion));
		if (IFileManager::Get().Copy(*DestJson, *VersionJson, true))
		{
			AppendLog(FString::Printf(TEXT("[复制] 版本描述已复制: %s"), *DestJson));
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[错误] 复制版本描述失败: %s"), *VersionJson), FLinearColor::Red);
		}
	}

	// 2. 复制整包 -> Release/Build/{平台}（仅勾选了打整包时）
	if (bFullPackage)
	{
		const FString FullPackageDir = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(GetOutputRootPath(), SelectedPlatform, TEXT("FullPackage")));
		const FString BuildDestDir = FPaths::Combine(ReleaseRoot, TEXT("Build"), SelectedPlatform);
		if (FPaths::DirectoryExists(FullPackageDir))
		{
			IFileManager::Get().MakeDirectory(*BuildDestDir, true);
			if (FPlatformFileManager::Get().GetPlatformFile().CopyDirectoryTree(*BuildDestDir, *FullPackageDir, true))
			{
				AppendLog(FString::Printf(TEXT("[复制] 整包已复制: %s -> %s"), *FullPackageDir, *BuildDestDir), FLinearColor(0.f, 1.f, 0.f));
			}
			else
			{
				AppendLog(FString::Printf(TEXT("[错误] 复制整包失败: %s -> %s"), *FullPackageDir, *BuildDestDir), FLinearColor::Red);
			}
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[警告] 未找到整包输出目录: %s"), *FullPackageDir), FLinearColor::Yellow);
		}
	}

	AppendLog(TEXT("============== 复制完成 =============="), FLinearColor(0.f, 1.f, 1.f));
}

FString SPackPanelWidget::GetHotPatcherPlatformName() const
{
	if (SelectedPlatform == TEXT("Android"))
	{
		return TEXT("Android");
	}
	if (SelectedPlatform == TEXT("IOS"))
	{
		return TEXT("IOS");
	}
	return TEXT("Windows");
}

FString SPackPanelWidget::GetUATPlatformName() const
{
	if (SelectedPlatform == TEXT("Android"))
	{
		return TEXT("Android");
	}
	if (SelectedPlatform == TEXT("IOS"))
	{
		return TEXT("IOS");
	}
	return TEXT("Win64");
}

FString SPackPanelWidget::GetClientConfigName() const
{
	// Debug -> Development, Release -> Shipping
	return SelectedPackType == TEXT("Debug") ? TEXT("Development") : TEXT("Shipping");
}

bool SPackPanelWidget::IsLiveCodingActive()
{
#if PLATFORM_WINDOWS
	// 命名规则与 LiveCodingModule.cpp / UnrealBuildTool::HotReload 保持一致：
	// Global\LiveCoding_{可执行文件全路径，其中 '/', '\\', ':' 替换为 '+'}
	FString MutexName = TEXT("Global\\LiveCoding_");
	const FString ExecutablePath = FPaths::ConvertRelativePathToFull(FPlatformProcess::ExecutablePath());
	MutexName.Reserve(MutexName.Len() + ExecutablePath.Len());
	for (int32 Idx = 0; Idx < ExecutablePath.Len(); ++Idx)
	{
		const TCHAR Ch = ExecutablePath[Idx];
		MutexName += (Ch == TEXT('/') || Ch == TEXT('\\') || Ch == TEXT(':')) ? TEXT('+') : Ch;
	}

	HANDLE Mutex = ::OpenMutexW(SYNCHRONIZE, false, *MutexName);
	if (Mutex != nullptr)
	{
		::CloseHandle(Mutex);
		return true;
	}

	// 对象存在但当前会话无权限访问时，同样视为 Live Coding 活跃
	return ::GetLastError() == ERROR_ACCESS_DENIED;
#else
	return false;
#endif
}

bool SPackPanelWidget::GenerateHotPatcherConfig(FString& OutConfigPath)
{
	// 复用用户在 HotPatcher 设置中保存的补丁模板配置
	UHotPatcherSettings* HotPatcherSettings = GetMutableDefault<UHotPatcherSettings>();
	HotPatcherSettings->ReloadConfig();

	FExportPatchSettings PatchSettings = HotPatcherSettings->TempPatchSetting;

	PatchSettings.VersionId = ResourceVersion;

	// 输出目录（统一输出到 {项目}/HotPatcherRes/{渠道}_{平台}）
	const FString OutputRoot = GetHotPatcherOutputRoot();
	PatchSettings.SavePath.Path = FPaths::ConvertRelativePathToFull(OutputRoot);

	// 目标平台
	ETargetPlatform PlatformEnum;
	TArray<ETargetPlatform> Platforms;
	if (THotPatcherTemplateHelper::GetEnumValueByName<ETargetPlatform>(GetHotPatcherPlatformName(), PlatformEnum))
	{
		Platforms.Add(PlatformEnum);
		AppendLog(FString::Printf(TEXT("[信息] 目标平台: %s"), *GetHotPatcherPlatformName()));
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[警告] 无法解析平台枚举: %s，将使用空平台列表！"), *GetHotPatcherPlatformName()), FLinearColor::Yellow);
	}
	PatchSettings.PakTargetPlatforms = Platforms;

	// 资源扫描范围：用户已配置则保留，否则默认扫描 /Game/ 全部内容
	FAssetScanConfig& ScanConfig = PatchSettings.GetAssetScanConfigRef();
	if (ScanConfig.AssetIncludeFilters.Num() == 0 && ScanConfig.IncludeSpecifyAssets.Num() == 0)
	{
		FDirectoryPath GameDir;
		GameDir.Path = TEXT("/Game/");
		ScanConfig.AssetIncludeFilters.Add(GameDir);
		AppendLog(TEXT("[信息] 未配置资源扫描范围，默认全量扫描 /Game/ 内容。"));
	}

	// 主版本（基础版本）策略
	if (bFullPackage)
	{
		// 打整包：资源版本 = 面板资源版本（VersionId 已设置），主版本仍按"是否全量资源打入首包"决定
		if (bFullInFirstPak)
		{
			PatchSettings.bByBaseVersion = false;
			PatchSettings.BaseVersion.FilePath.Empty();
			AppendLog(TEXT("[信息] 全量模式：所有资源打入首包。"));
		}
		else
		{
			const FString BaseJson = FindLatestBaseVersionJson(PatchSettings.GetSaveAbsPath());
			if (BaseJson.IsEmpty())
			{
				AppendLog(TEXT("[警告] 未检测到基础版本，自动切换为全量模式！"), FLinearColor::Yellow);
				PatchSettings.bByBaseVersion = false;
			}
			else
			{
				PatchSettings.bByBaseVersion = true;
				PatchSettings.BaseVersion.FilePath = BaseJson;
				AppendLog(FString::Printf(TEXT("[信息] 增量模式，基础版本: %s"), *BaseJson));
			}
		}
	}
	else
	{
		// 不打整包（纯补丁）：patch 版本 = 面板资源版本，主版本 = 下拉选中的历史版本
		if (SelectedBaseVersionPath.IsEmpty())
		{
			AppendLog(TEXT("[错误] 未选择历史版本作为主版本，无法生成补丁！"), FLinearColor::Red);
			return false;
		}

		PatchSettings.BaseVersion.FilePath = SelectedBaseVersionPath;
		if (bFullInFirstPak)
		{
			PatchSettings.bByBaseVersion = false;
			AppendLog(FString::Printf(TEXT("[信息] 全量补丁模式，主版本: %s (%s)"),
				*SelectedBaseVersion, *SelectedBaseVersionPath));
		}
		else
		{
			PatchSettings.bByBaseVersion = true;
			AppendLog(FString::Printf(TEXT("[信息] 增量补丁模式，主版本: %s (%s)"),
				*SelectedBaseVersion, *SelectedBaseVersionPath));
		}
		AppendLog(FString::Printf(TEXT("[信息] 补丁版本(patch): %s"), *ResourceVersion));
	}

	// 序列化并写出配置
	FString ConfigJson;
	if (!THotPatcherTemplateHelper::TSerializeStructAsJsonString(PatchSettings, ConfigJson))
	{
		AppendLog(TEXT("[错误] 序列化 HotPatcher 配置失败！"), FLinearColor::Red);
		return false;
	}

	const FString ConfigDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("HotPatcher")));
	IFileManager::Get().MakeDirectory(*ConfigDir, true);

	OutConfigPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(ConfigDir,
			FString::Printf(TEXT("%s_%s_PatchConfig.json"), *ResourceVersion, *GetHotPatcherPlatformName())));

	if (!FFileHelper::SaveStringToFile(ConfigJson, *OutConfigPath))
	{
		AppendLog(FString::Printf(TEXT("[错误] 写入配置文件失败: %s"), *OutConfigPath), FLinearColor::Red);
		return false;
	}

	AppendLog(FString::Printf(TEXT("[信息] HotPatcher 配置已生成: %s"), *OutConfigPath));
	return true;
}

FString SPackPanelWidget::FindLatestBaseVersionJson(const FString& SaveAbsPath) const
{
	if (!FPaths::DirectoryExists(SaveAbsPath))
	{
		return FString();
	}

	TArray<FString> FoundFiles;
	IFileManager::Get().FindFilesRecursive(FoundFiles, *SaveAbsPath, TEXT("*.json"), true, false);

	FString LatestFile;
	FDateTime LatestTime = FDateTime::MinValue();

	for (const FString& File : FoundFiles)
	{
		const FString FileName = FPaths::GetCleanFilename(File);

		// 跳过补丁配置
		if (FileName.Contains(TEXT("PatchConfig")))
		{
			continue;
		}

		// 版本文件命名规则: {版本Id}.json / {版本Id}_Release.json，位于 {版本Id}/ 目录下
		const FString ParentDirName = FPaths::GetCleanFilename(FPaths::GetPath(File));
		if (ParentDirName.IsEmpty() || ParentDirName.Contains(ResourceVersion))
		{
			continue;
		}
		if (!FileName.StartsWith(ParentDirName))
		{
			continue;
		}

		const FDateTime ModifyTime = IFileManager::Get().GetTimeStamp(*File);
		if (ModifyTime > LatestTime)
		{
			LatestTime = ModifyTime;
			LatestFile = File;
		}
	}

	return LatestFile;
}

void SPackPanelWidget::AppendLog(const FString& Text, const FLinearColor& Color)
{
	// 累积日志缓存，供"复制日志"按钮使用
	LogBuffer += Text;
	LogBuffer += LINE_TERMINATOR;

	if (!LogScrollBox.IsValid())
	{
		return;
	}

	LogScrollBox->AddSlot()
	[
		SNew(STextBlock)
		.Text(FText::FromString(Text))
		.ColorAndOpacity(FSlateColor(Color))
		.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 9))
		.AutoWrapText(true)
	];

	LogScrollBox->ScrollToEnd();
}

#undef LOCTEXT_NAMESPACE
