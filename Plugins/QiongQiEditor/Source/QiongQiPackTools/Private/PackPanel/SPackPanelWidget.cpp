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
#include "DesktopPlatformModule.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/App.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"

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
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SPackPanelWidget"

namespace
{
	// 整包主版本标记文件名后缀（{版本Id}_Release.json，仅打整包成功时生成）
	const TCHAR* MainVersionMarkerFileSuffix = TEXT("_Release.json");
	// 历史版本下拉框中整包主版本的显示后缀（如 1786886960（主））
	const TCHAR* MainVersionDisplaySuffix = TEXT("（主）");

	/**
	 * 从 ini 文件解析 [WhiteList] 节下的 Paths 列表。
	 * 独立 ini（非配置层级文件）经引擎加载时 +Paths= 前缀不会被剥离（FConfigFile::Read
	 * 对非层级配置 bHandleSymbolCommands=false），因此手写解析并同时兼容新版 Paths=
	 * 与旧版 +Paths= 两种写法，避免 GConfig->GetArray 永远读不到数据的问题。
	 */
	bool ParseWhiteListFromIni(const FString& IniPath, TArray<FString>& OutDirs)
	{
		OutDirs.Reset();
		if (!FPaths::FileExists(IniPath))
		{
			return false;
		}

		TArray<FString> Lines;
		if (!FFileHelper::LoadFileToStringArray(Lines, *IniPath))
		{
			return false;
		}

		bool bInSection = false;
		for (const FString& RawLine : Lines)
		{
			const FString Line = RawLine.TrimStartAndEnd();
			if (Line.IsEmpty())
			{
				continue;
			}
			// 跳过注释行
			if (Line.StartsWith(TEXT(";")) || Line.StartsWith(TEXT("#")))
			{
				continue;
			}
			if (Line.StartsWith(TEXT("[")) && Line.EndsWith(TEXT("]")))
			{
				bInSection = Line.Equals(TEXT("[WhiteList]"));
				continue;
			}
			if (!bInSection)
			{
				continue;
			}

			FString Key, Value;
			if (!Line.Split(TEXT("="), &Key, &Value))
			{
				continue;
			}
			Key.TrimStartAndEndInline();
			if (Key != TEXT("Paths") && Key != TEXT("+Paths"))
			{
				continue;
			}
			Value.TrimStartAndEndInline();
			if (!Value.IsEmpty())
			{
				OutDirs.Add(Value);
			}
		}
		return OutDirs.Num() > 0;
	}
}

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
					// 资源版本仅允许数字（拦截非法字符，实时过滤）
					.OnTextChanged_Lambda([this](const FText& InText)
					{
						FString Filtered;
						for (const TCHAR Ch : InText.ToString())
						{
							if (FChar::IsDigit(Ch))
							{
								Filtered.AppendChar(Ch);
							}
						}
						if (!Filtered.Equals(InText.ToString()))
						{
							ResourceVersionBox->SetText(FText::FromString(Filtered));
						}
					})
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

				// 清理 Release 交付目录（打包开始前清空 CDN 累积目录与整包留档）
				+ SGridPanel::Slot(0, 7)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("清理打包目录：")))
					.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 12))
				]
				+ SGridPanel::Slot(1, 7)
				.VAlign(VAlign_Center)
				.Padding(4.f, 4.f)
				[
					SAssignNew(CleanReleaseCheckBox, SCheckBox)
					.ToolTipText(FText::FromString(TEXT("勾选后，打包开始前自动清空构建产物目录（UAT/HotPatcher 输出），保留 Release 交付目录中的 CDN 历史资源与版本清单")))
					.IsChecked(ECheckBoxState::Unchecked)
					.OnCheckStateChanged_Lambda([this](ECheckBoxState InState)
					{
						bCleanRelease = (InState == ECheckBoxState::Checked);
					})
				]
			]

			// 随包目录白名单（Git 提交共享，配置存 Config/CDNInFirstPak.ini）
			// 仅在"全量资源打入首包"取消勾选（CDN 模式）时显示，全量模式隐藏
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SBox)
				.Visibility_Lambda([this]()
				{
					return bFullInFirstPak ? EVisibility::Collapsed : EVisibility::Visible;
				})
				[
					SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.05f, 0.12f, 0.14f, 0.9f))
				.Padding(FMargin(10.f, 8.f))
				[
					SNew(SVerticalBox)

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("随包目录白名单")))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 12))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.1f, 0.82f, 0.77f)))
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(STextBlock)
						.Text(FText::FromString(TEXT("列入白名单的资源目录随安装包发布；未列入的目录资源将走 CDN 动态下发。配置保存于 Config/CDNInFirstPak.ini。")))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Regular"), 10))
						.ColorAndOpacity(FSlateColor(FLinearColor(0.6f, 0.6f, 0.6f)))
						.AutoWrapText(true)
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SBox)
						.HeightOverride(86.f)
						[
							SAssignNew(WhiteListView, SListView<TSharedPtr<FString>>)
							.ListItemsSource(&WhiteListPaths)
							.SelectionMode(ESelectionMode::None)
							.OnGenerateRow_Lambda([this](TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
							{
								return SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
								.Padding(FMargin(2.f, 1.f))
								[
									SNew(SHorizontalBox)
									+ SHorizontalBox::Slot()
									.FillWidth(1.f)
									.VAlign(VAlign_Center)
									[
										SNew(STextBlock)
										.Text(FText::FromString(*InItem))
										.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Mono"), 10))
									]
									+ SHorizontalBox::Slot()
									.AutoWidth()
									.VAlign(VAlign_Center)
									[
										SNew(SButton)
										.Text(FText::FromString(TEXT("移除")))
										.ButtonStyle(&FAppStyle::Get().GetWidgetStyle<FButtonStyle>("NoBorder"))
										.ContentPadding(FMargin(6.f, 0.f))
										.OnClicked_Lambda([this, InItem]()
										{
											OnRemoveWhiteList(InItem);
											return FReply::Handled();
										})
									]
								];
							})
						]
					]

					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)

						+ SHorizontalBox::Slot()
						.FillWidth(1.f)
						.Padding(0.f, 2.f, 6.f, 2.f)
						[
							SAssignNew(WhiteListInputBox, SEditableTextBox)
							.Style(&FCoreStyle::Get().GetWidgetStyle<FEditableTextBoxStyle>("NormalEditableTextBox"))
							.HintText(FText::FromString(TEXT("/Game/资源目录，如 /Game/UI/Characters")))
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2.f, 0.f)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("添加")))
							.OnClicked_Lambda([this]()
							{
								OnAddWhiteListClicked();
								return FReply::Handled();
							})
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2.f, 0.f)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("导入")))
							.OnClicked_Lambda([this]()
							{
								OnImportWhiteListClicked();
								return FReply::Handled();
							})
						]

						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(2.f, 0.f)
						[
							SNew(SButton)
							.Text(FText::FromString(TEXT("导出")))
							.OnClicked_Lambda([this]()
							{
								OnExportWhiteListClicked();
								return FReply::Handled();
							})
						]
					]
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

	// 加载随包目录白名单（Git 提交共享，配置存 Config/CDNInFirstPak.ini）
	LoadWhiteListConfig();

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
	if (!ResourceVersion.IsNumeric())
	{
		AppendLog(TEXT("[错误] 资源版本仅允许填数字（如 Unix 时间戳）！"), FLinearColor::Red);
		return;
	}
	if (SelectedPlatform.IsEmpty() || SelectedPackType.IsEmpty())
	{
		AppendLog(TEXT("[错误] 请选择打包目标平台与打包类型！"), FLinearColor::Red);
		return;
	}

	bFullInFirstPak = FullInFirstPakCheckBox.IsValid() ? FullInFirstPakCheckBox->IsChecked() : bFullInFirstPak;
	bFullPackage = FullPackageCheckBox.IsValid() ? FullPackageCheckBox->IsChecked() : bFullPackage;
	bCleanRelease = CleanReleaseCheckBox.IsValid() ? CleanReleaseCheckBox->IsChecked() : bCleanRelease;

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

	// 白名单随本次打包配置一并落盘（Config/CDNInFirstPak.ini）
	SaveWhiteListConfig();

	AppendLog(TEXT(""));
	AppendLog(TEXT("============== QiongQi 打包开始 =============="), FLinearColor::Green);
	AppendLog(FString::Printf(TEXT("[配置] 渠道: %s    版本: %s"), *ChannelName, *ResourceVersion));
	AppendLog(FString::Printf(TEXT("[配置] 平台: %s    类型: %s"), *SelectedPlatform, *SelectedPackType));
	AppendLog(FString::Printf(TEXT("[配置] 随包目录白名单(%d): %s"),
		WhiteListRaw.Num(), WhiteListRaw.Num() > 0 ? *FString::Join(WhiteListRaw, TEXT(", ")) : TEXT("(空，全部资源走 CDN)")));
	AppendLog(FString::Printf(TEXT("[配置] 全量资源打入首包: %s    打整包: %s"),
		bFullInFirstPak ? TEXT("是") : TEXT("否"),
		bFullPackage ? TEXT("是") : TEXT("否")));
	AppendLog(FString::Printf(TEXT("[配置] 清理 Release 交付目录: %s"), bCleanRelease ? TEXT("是") : TEXT("否")));
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

	// 勾选"清理打包目录"时，在打包正式开始前清空 Release 交付目录
	if (bCleanRelease)
	{
		CleanReleaseDirectory();
	}

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

	// 下拉项可能带"（主）"后缀（整包主版本），剥离后仅存纯版本号，避免污染补丁 base 路径
	SelectedBaseVersion = *InValue;
	SelectedBaseVersion.RemoveFromEnd(MainVersionDisplaySuffix);
	SelectedBaseVersionPath.Empty();
	for (int32 Idx = 0; Idx < BaseVersionOptions.Num(); ++Idx)
	{
		if (BaseVersionOptions[Idx].IsValid())
		{
			// 同样剥离"（主）"后缀后按纯版本号精确匹配路径，避免 StartsWith 式误匹配（如 1786864530 与 17868645301）
			FString OptionVersion = *BaseVersionOptions[Idx];
			OptionVersion.RemoveFromEnd(MainVersionDisplaySuffix);
			if (OptionVersion == SelectedBaseVersion)
			{
				SelectedBaseVersionPath = BaseVersionPaths[Idx];
				break;
			}
		}
	}

	if (BaseVersionComboText.IsValid())
	{
		// 下拉框显示原始选中文本（含"（主）"后缀）
		BaseVersionComboText->SetText(FText::FromString(*InValue));
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

	// 收集 {版本Id} -> 版本信息：整包/补丁版本均列出，整包主版本（存在 {版本Id}_Release.json）加"（主）"后缀
	struct FBaseVersionInfo
	{
		FString Name;          // 纯版本号（目录名）
		FString Path;          // 该目录下最新版本文件的完整路径
		FDateTime Time;        // 最新修改时间
		bool bRelease = false; // 是否存在 {版本Id}_Release.json（整包主版本）
	};
	TMap<FString, FBaseVersionInfo> VersionMap;

	for (const FString& File : FoundFiles)
	{
		const FString FileName = FPaths::GetCleanFilename(File);

		// 跳过补丁配置
		if (FileName.Contains(TEXT("PatchConfig")))
		{
			continue;
		}

		// 版本目录内仅两类版本文件参与识别，均位于 {版本Id}/ 目录下：
		//   {版本Id}_PakResults.json —— 整包/补丁每次打包均生成，用于发现版本目录
		//   {版本Id}_Release.json —— 仅整包主版本生成，用于标记主版本
		const FString ParentDirName = FPaths::GetCleanFilename(FPaths::GetPath(File));
		if (ParentDirName.IsEmpty() || ParentDirName.Contains(ResourceVersion))
		{
			continue;
		}
		const FString ReleaseName = ParentDirName + MainVersionMarkerFileSuffix;
		const FString ResultsName = ParentDirName + TEXT("_PakResults.json");
		if (FileName != ReleaseName && FileName != ResultsName)
		{
			continue;
		}

		FBaseVersionInfo& Info = VersionMap.FindOrAdd(ParentDirName);
		Info.Name = ParentDirName;
		// 同一版本目录任一路径为 _Release.json 即视为整包主版本
		Info.bRelease = Info.bRelease || (FileName == ReleaseName);
		const FDateTime FileTime = IFileManager::Get().GetTimeStamp(*File);
		if (FileTime > Info.Time)
		{
			Info.Time = FileTime;
			Info.Path = File;
		}
	}

	TArray<FBaseVersionInfo> Versions;
	Versions.Reserve(VersionMap.Num());
	for (const auto& Pair : VersionMap)
	{
		Versions.Add(Pair.Value);
	}
	Versions.Sort([](const FBaseVersionInfo& A, const FBaseVersionInfo& B)
	{
		return A.Time > B.Time;
	});

	// 生成下拉选项：整包主版本显示为"版本号（主）"，纯补丁版本显示纯版本号
	for (const FBaseVersionInfo& Info : Versions)
	{
		const FString DisplayName = Info.bRelease
			? Info.Name + MainVersionDisplaySuffix
			: Info.Name;
		BaseVersionOptions.Add(MakeShareable(new FString(DisplayName)));
		BaseVersionPaths.Add(Info.Path);
	}

	// 默认选中：优先保持用户原选择（纯版本号），否则选最新（排序后的第一项）
	int32 DefaultIdx = 0;
	if (BaseVersionOptions.Num() > 0)
	{
		if (!PreviousSelection.IsEmpty())
		{
			for (int32 Idx = 0; Idx < BaseVersionOptions.Num(); ++Idx)
			{
				// 下拉项可能带"（主）"后缀，剥离后与纯版本号比较
				FString OptionVersion = BaseVersionOptions[Idx].IsValid() ? *BaseVersionOptions[Idx] : FString();
				OptionVersion.RemoveFromEnd(MainVersionDisplaySuffix);
				if (OptionVersion == PreviousSelection)
				{
					DefaultIdx = Idx;
					break;
				}
			}
		}
		// 显示与存储分离：下拉框显示"版本号（主）"，内部仅存纯版本号
		SelectedBaseVersion = *BaseVersionOptions[DefaultIdx];
		SelectedBaseVersion.RemoveFromEnd(MainVersionDisplaySuffix);
		SelectedBaseVersionPath = BaseVersionPaths[DefaultIdx];
	}

	if (BaseVersionComboBox.IsValid())
	{
		BaseVersionComboBox->RefreshOptions();
	}
	if (BaseVersionComboText.IsValid())
	{
		BaseVersionComboText->SetText(BaseVersionOptions.Num() > 0
			? FText::FromString(*BaseVersionOptions[DefaultIdx])
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
	LastStageOutput.Reset();
	AppendLog(TEXT(""));
	AppendLog(TEXT("============== 整包构建 (UAT BuildCookRun) =============="), FLinearColor(0.f, 1.f, 1.f));

	const FString RunUATPath = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::EngineDir(), TEXT("Build/BatchFiles/RunUAT.bat")));
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	const FString ArchiveDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(GetOutputRootPath(), SelectedPlatform, TEXT("FullPackage")));

	// CDN 模式：把 Chunk 分配规则（FirstPakAsset/CdnAsset）同步到 DefaultGame.ini，并让 UAT 产出 chunk 级容器
	SyncChunkRulesToGameIni(!bFullInFirstPak);

	// 面板渠道写入 DefaultGame.ini，随包固化（客户端运行时经 UeDownloadHelper::GetChannel 读取）
	SyncChannelToGameIni();

	FString UATArgs = FString::Printf(
		TEXT("BuildCookRun -project=\"%s\" -noP4 -platform=%s -clientconfig=%s -build -cook -stage -pak -archive -archivedirectory=\"%s\""),
		*ProjectPath,
		*GetUATPlatformName(),
		*GetClientConfigName(),
		*ArchiveDir);

	// 非全量进首包（CDN 模式）：追加 -manifests 产出 chunk 清单与容器（pakchunk0/pakchunk100），
	// 打包后剔除 CDN chunk（pakchunk100*）容器实现"白名单进首包、其余走 CDN"。
	if (!bFullInFirstPak)
	{
		UATArgs += TEXT(" -manifests");
		AppendLog(TEXT("[信息] CDN 模式：UAT 已附加 -manifests，将按 Chunk 规则拆分首包与 CDN 容器。"), FLinearColor(0.1f, 0.82f, 0.77f));
	}

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
	LastStageOutput.Reset();
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
		LastStageOutput.Add(Line);
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
			// 失败时对输出做特征匹配，输出可操作的解决指引（如编辑器 DLL 被占用导致 LNK1104）
			DiagnoseUATFailure();
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

void SPackPanelWidget::DiagnoseUATFailure()
{
	// 特征匹配：编辑器插件 DLL 被当前编辑器进程占用，导致 UBT 链接失败（LNK1104）
	bool bDllLocked = false;
	for (const FString& Line : LastStageOutput)
	{
		if (Line.Contains(TEXT("LNK1104"))
			|| (Line.Contains(TEXT("being used by another process")) && Line.Contains(TEXT(".dll")))
			|| (Line.Contains(TEXT("The process cannot access the file")) && Line.Contains(TEXT(".dll"))))
		{
			bDllLocked = true;
			break;
		}
	}

	if (!bDllLocked)
	{
		return;
	}

	const FLinearColor WarnColor = FLinearColor(1.f, 0.7f, 0.f);
	const FLinearColor InfoColor = FLinearColor(0.f, 1.f, 1.f);
	AppendLog(TEXT(""));
	AppendLog(TEXT("============== 失败诊断 =============="), WarnColor);
	AppendLog(TEXT("[诊断] 链接失败：编辑器插件 DLL（UnrealEditor-*.dll）正被当前编辑器进程占用，无法覆盖写入。"), WarnColor);
	AppendLog(TEXT("[诊断] 原因：编辑器插件源码（QiongQiEditor/Source）有改动，UAT 构建 QiongQiEditor 目标时需要重新链接 DLL，"), WarnColor);
	AppendLog(TEXT("       但 Windows 不允许写入正被运行中的编辑器加载的文件。"), WarnColor);
	AppendLog(TEXT("[解决] 1. 关闭 UE 编辑器（释放 DLL 占用）；"), InfoColor);
	AppendLog(TEXT("[解决] 2. 在 CMD 中运行以下预编译命令，把改动编译进 DLL："), InfoColor);

	const FString EngineBuildBat = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::EngineDir(), TEXT("Build/BatchFiles/Build.bat")));
	const FString ProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
	AppendLog(FString::Printf(TEXT("       \"%s\" QiongQiEditor Win64 Development -project=\"%s\" -WaitMutex"),
		*EngineBuildBat, *ProjectPath));

	AppendLog(TEXT("[解决] 3. 编译成功后重新打开编辑器，再点击打包即可；此后 DLL 已是最新，UAT 不会再尝试重链。"), InfoColor);
	AppendLog(TEXT("====================================="), WarnColor);
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

// ---------------------------------------------------------------------------------------------
// 随包目录白名单（Git 提交共享）
// ---------------------------------------------------------------------------------------------

FString SPackPanelWidget::GetWhiteListConfigPath()
{
	return FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Config/CDNInFirstPak.ini")));
}

void SPackPanelWidget::LoadWhiteListConfig()
{
	WhiteListPaths.Empty();
	WhiteListRaw.Empty();

	const FString ConfigPath = GetWhiteListConfigPath();
	if (FPaths::FileExists(ConfigPath))
	{
		// 独立 ini 文件经引擎加载时 +Paths= 前缀不会被剥离（FConfigFile::Read 对非层级
		// 配置不处理符号命令），GConfig->GetArray 会永远读不到数据，因此改用手写解析，
		// 同时兼容旧版 +Paths= 与新版 Paths= 两种格式。
		ParseWhiteListFromIni(ConfigPath, WhiteListRaw);
		for (const FString& Dir : WhiteListRaw)
		{
			WhiteListPaths.Add(MakeShareable(new FString(Dir)));
		}
	}

	// 默认白名单：AssetsPackage 目录保持随包方式不变
	if (WhiteListRaw.Num() == 0)
	{
		WhiteListRaw.Add(TEXT("/Game/AssetsPackage/"));
		WhiteListPaths.Add(MakeShareable(new FString(TEXT("/Game/AssetsPackage/"))));
		SaveWhiteListConfig();
		AppendLog(TEXT("[信息] 未找到白名单配置，已写入默认白名单: /Game/AssetsPackage/"), FLinearColor::Yellow);
	}

	RefreshWhiteListView();
	AppendLog(FString::Printf(TEXT("[信息] 随包目录白名单已加载（%d 个目录）: %s"),
		WhiteListRaw.Num(), WhiteListRaw.Num() > 0 ? *FString::Join(WhiteListRaw, TEXT(", ")) : TEXT("(空)")), FLinearColor(0.1f, 0.82f, 0.77f));
}

void SPackPanelWidget::SaveWhiteListConfig()
{
	// 手写 ini 文本，与 ParseWhiteListFromIni 读取格式对称。
	// 注意：独立 ini 解析时不剥离 + 前缀，必须写无前缀的 Paths=，否则重新加载时会读不到。
	FString IniStr = TEXT("[WhiteList]\r\n");
	for (const FString& Dir : WhiteListRaw)
	{
		IniStr += FString::Printf(TEXT("Paths=%s\r\n"), *Dir);
	}

	const FString ConfigPath = GetWhiteListConfigPath();
	if (FFileHelper::SaveStringToFile(IniStr, *ConfigPath))
	{
		AppendLog(FString::Printf(TEXT("[信息] 白名单配置已保存: %s"), *ConfigPath), FLinearColor(0.1f, 0.82f, 0.77f));
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[错误] 保存白名单配置失败: %s"), *ConfigPath), FLinearColor::Red);
	}
}

void SPackPanelWidget::OnAddWhiteListClicked()
{
	if (!WhiteListInputBox.IsValid())
	{
		return;
	}

	const FString Dir = WhiteListInputBox->GetText().ToString().TrimStartAndEnd();
	if (Dir.IsEmpty())
	{
		AppendLog(TEXT("[错误] 请输入要加入白名单的目录，如 /Game/AssetsPackage/！"), FLinearColor::Red);
		return;
	}
	if (!Dir.StartsWith(TEXT("/Game/")))
	{
		AppendLog(TEXT("[错误] 白名单目录必须位于 /Game/ 下！"), FLinearColor::Red);
		return;
	}
	if (WhiteListRaw.Contains(Dir))
	{
		AppendLog(FString::Printf(TEXT("[提示] 目录已在白名单中: %s"), *Dir), FLinearColor::Yellow);
		return;
	}

	WhiteListRaw.Add(Dir);
	WhiteListPaths.Add(MakeShareable(new FString(Dir)));
	WhiteListInputBox->SetText(FText::GetEmpty());
	RefreshWhiteListView();
	SaveWhiteListConfig();
}

void SPackPanelWidget::OnRemoveWhiteList(TSharedPtr<FString> InItem)
{
	if (!InItem.IsValid())
	{
		return;
	}

	const FString Dir = *InItem;
	WhiteListRaw.Remove(Dir);
	WhiteListPaths.Remove(InItem);
	RefreshWhiteListView();
	SaveWhiteListConfig();
	AppendLog(FString::Printf(TEXT("[信息] 已从白名单移除: %s"), *Dir));
}

void SPackPanelWidget::OnImportWhiteListClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> OutFiles;
	if (!DesktopPlatform->OpenFileDialog(
		nullptr,
		TEXT("导入白名单配置"),
		*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),
		TEXT(""),
		TEXT("INI 文件 (*.ini)|*.ini"),
		EFileDialogFlags::None,
		OutFiles) || OutFiles.Num() == 0)
	{
		return;
	}

	// 手写解析所选 ini 的 [WhiteList] 节 Paths 列表（兼容 +Paths= 旧格式）
	TArray<FString> ImportedRaw;
	ParseWhiteListFromIni(OutFiles[0], ImportedRaw);
	if (ImportedRaw.Num() == 0)
	{
		AppendLog(FString::Printf(TEXT("[错误] 未从 ini 中解析到 [WhiteList] 下 Paths 列表: %s"), *OutFiles[0]), FLinearColor::Red);
		return;
	}

	WhiteListRaw.Empty();
	WhiteListPaths.Empty();
	for (const FString& Dir : ImportedRaw)
	{
		const FString CleanDir = Dir.TrimStartAndEnd();
		if (!CleanDir.IsEmpty())
		{
			WhiteListRaw.Add(CleanDir);
			WhiteListPaths.Add(MakeShareable(new FString(CleanDir)));
		}
	}

	RefreshWhiteListView();
	SaveWhiteListConfig();
	AppendLog(FString::Printf(TEXT("[信息] 已从文件导入白名单（%d 个目录）: %s"),
		WhiteListRaw.Num(), *OutFiles[0]), FLinearColor(0.1f, 0.82f, 0.77f));
}

void SPackPanelWidget::OnExportWhiteListClicked()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return;
	}

	TArray<FString> OutFiles;
	if (!DesktopPlatform->SaveFileDialog(
		nullptr,
		TEXT("导出白名单配置"),
		*FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()),
		TEXT("CDNInFirstPak.ini"),
		TEXT("INI 文件 (*.ini)|*.ini"),
		EFileDialogFlags::None,
		OutFiles) || OutFiles.Num() == 0)
	{
		return;
	}

	// 导出格式与 SaveWhiteListConfig 一致（[WhiteList] 节 Paths= 多行）
	FString IniStr = TEXT("[WhiteList]\r\n");
	for (const FString& Dir : WhiteListRaw)
	{
		IniStr += FString::Printf(TEXT("Paths=%s\r\n"), *Dir);
	}

	if (FFileHelper::SaveStringToFile(IniStr, *OutFiles[0]))
	{
		AppendLog(FString::Printf(TEXT("[信息] 白名单配置已导出: %s"), *OutFiles[0]), FLinearColor(0.1f, 0.82f, 0.77f));
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[错误] 导出白名单配置失败: %s"), *OutFiles[0]), FLinearColor::Red);
	}
}

void SPackPanelWidget::RefreshWhiteListView()
{
	if (WhiteListView.IsValid())
	{
		WhiteListView->RequestListRefresh();
	}
}

// ---------------------------------------------------------------------------------------------
// CDN Chunk 隔离
// ---------------------------------------------------------------------------------------------

/**
 * 直接文本方式同步 ini section：保留已有且不被 IsManagedLine 匹配的行，
 * 移除所有被 IsManagedLine 匹配的旧行，并追加 NewManagedLines。
 * 绕过 GConfig 缓存/层级合并，确保落盘到项目 DefaultGame.ini，供 UAT 子进程读取。
 */
template<typename PredicateT>
static void SyncIniSectionByText(const FString& IniPath, const FString& SectionName, const FString& KeyPrefix, const TArray<FString>& NewManagedLines, PredicateT IsManagedLine)
{
	FString Content;
	if (!FFileHelper::LoadFileToString(Content, *IniPath))
	{
		Content.Empty();
	}

	// 统一换行符
	Content.ReplaceInline(TEXT("\r\n"), TEXT("\n"));
	Content.ReplaceInline(TEXT("\r"), TEXT("\n"));

	const FString SectionHeader = FString::Printf(TEXT("[%s]"), *SectionName);
	const FString FullKeyPrefix = KeyPrefix.EndsWith(TEXT("=")) ? KeyPrefix : KeyPrefix + TEXT("=");

	TArray<FString> PreservedLines;
	int32 SectionStart = Content.Find(SectionHeader, ESearchCase::CaseSensitive);
	int32 SectionEnd = INDEX_NONE;

	if (SectionStart != INDEX_NONE)
	{
		// 下一个 section 开始或文件末尾
		SectionEnd = Content.Find(TEXT("["), ESearchCase::CaseSensitive, ESearchDir::FromStart, SectionStart + SectionHeader.Len());
		if (SectionEnd == INDEX_NONE)
		{
			SectionEnd = Content.Len();
		}

		const FString SectionBody = Content.Mid(SectionStart + SectionHeader.Len(), SectionEnd - SectionStart - SectionHeader.Len());
		TArray<FString> Lines;
		SectionBody.ParseIntoArray(Lines, TEXT("\n"), true);

		for (const FString& RawLine : Lines)
		{
			const FString Line = RawLine.TrimStartAndEnd();
			if (Line.IsEmpty())
			{
				continue;
			}

			// 仅当是 KeyPrefix 开头且被管理时移除；其余（注释、其他 key）保留
			if (Line.StartsWith(FullKeyPrefix) && IsManagedLine(Line))
			{
				continue;
			}
			PreservedLines.Add(Line);
		}
	}

	// 组装新的 section 内容
	FString NewSection;
	NewSection.Append(SectionHeader).Append(TEXT("\n"));
	for (const FString& Line : PreservedLines)
	{
		NewSection.Append(Line).Append(TEXT("\n"));
	}
	for (const FString& Line : NewManagedLines)
	{
		NewSection.Append(Line).Append(TEXT("\n"));
	}

	// 组装新文件内容
	FString NewContent;
	if (SectionStart != INDEX_NONE)
	{
		NewContent = Content.Left(SectionStart);
		// 确保 section 前至少一个换行
		if (NewContent.Len() > 0 && !NewContent.EndsWith(TEXT("\n")))
		{
			NewContent.Append(TEXT("\n"));
		}
		NewContent.Append(NewSection);
		if (SectionEnd < Content.Len())
		{
			NewContent.Append(Content.Mid(SectionEnd));
		}
	}
	else
	{
		NewContent = Content;
		if (NewContent.Len() > 0 && !NewContent.EndsWith(TEXT("\n")))
		{
			NewContent.Append(TEXT("\n"));
		}
		NewContent.Append(TEXT("\n")).Append(NewSection);
	}

	FFileHelper::SaveStringToFile(NewContent, *IniPath, FFileHelper::EEncodingOptions::ForceUTF8);
}

void SPackPanelWidget::SyncChunkRulesToGameIni(bool bCdnMode)
{
	const FString IniPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));
	const FString Section = TEXT("/Script/Engine.AssetManagerSettings");
	const FString KeyPrefix = TEXT("+PrimaryAssetTypesToScan");

	TArray<FString> NewManagedLines;
	TArray<FString> MapCustomRuleLines;
	if (bCdnMode)
	{
		// 构造白名单目录列表（去尾斜杠）
		TArray<FString> TrimmedDirs;
		for (const FString& Dir : WhiteListRaw)
		{
			FString Trimmed = Dir.TrimStartAndEnd();
			Trimmed.RemoveFromEnd(TEXT("/"));
			if (Trimmed.IsEmpty())
			{
				continue;
			}
			TrimmedDirs.Add(Trimmed);
		}

		// 序列化为 FDirectoryPath
		TArray<FString> DirParts;
		for (const FString& Trimmed : TrimmedDirs)
		{
			DirParts.Add(FString::Printf(TEXT("(Path=\"%s\")"), *Trimmed));
		}
		if (DirParts.Num() == 0)
		{
			AppendLog(TEXT("[警告] 白名单为空，首包将不包含任何随包目录资源（全部走 CDN）！"), FLinearColor::Yellow);
		}

		// FirstPakAsset: ChunkId=0 + 高优先级(100)，覆盖 CdnAsset 对白名单资源的 chunk100 分配
		// 注意：bHasBlueprintClasses 必须为 False，让扫描器按 AssetBaseClass(Object) 全量扫描
		//（包含蓝图与普通资源）；若为 True 只会搜 UBlueprintCore，非蓝图资源将无法被分配到 chunk。
		const FString FirstPakEntry = FString::Printf(
			TEXT("+PrimaryAssetTypesToScan=(PrimaryAssetType=\"FirstPakAsset\",AssetBaseClass=/Script/CoreUObject.Object,bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=(%s),SpecificAssets=(),Rules=(Priority=100,ChunkId=0,CookRule=Unknown,bApplyRecursively=True))"),
			*FString::Join(DirParts, TEXT(",")));
		NewManagedLines.Add(FirstPakEntry);

		// CdnAsset: /Game 全量扫描 + ChunkId=100 + 低优先级(10)，未进白名单的资源全部归 CDN chunk
		NewManagedLines.Add(TEXT("+PrimaryAssetTypesToScan=(PrimaryAssetType=\"CdnAsset\",AssetBaseClass=/Script/CoreUObject.Object,bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path=\"/Game\")),SpecificAssets=(),Rules=(Priority=10,ChunkId=100,CookRule=Unknown,bApplyRecursively=True))"));

		// Map 类型全量规则：引擎对 .umap 资产硬编码注册为原生 Map 类型（自定义 FirstPakAsset/CdnAsset 扫描
		// 地图时必然冲突被忽略，日志 "Ignoring PrimaryAssetType ... - Conflicts with Map"），且引擎默认 Map
		// 只扫描 /Game/Maps（本项目地图在 /Game/AssetsPackage/Scenes），从未被 chunk 规则覆盖 → 全部落默认
		// chunk0（首包）。追加 Map 扫描 /Game 全量并设全局 ChunkId=100（Priority=10，低于白名单覆盖的 100），
		// 使非白名单地图默认归入 CDN chunk100；bIsEditorOnly=False 避免被编辑器专用过滤。
		NewManagedLines.Add(TEXT("+PrimaryAssetTypesToScan=(PrimaryAssetType=\"Map\",AssetBaseClass=/Script/Engine.World,bHasBlueprintClasses=False,bIsEditorOnly=False,Directories=((Path=\"/Game\")),SpecificAssets=(),Rules=(Priority=10,ChunkId=100,CookRule=Unknown,bApplyRecursively=True))"));

		// 白名单目录内地图覆盖回 chunk0：引擎在 ScanPrimaryAssetRulesFromConfig（PostInitialAssetScan，
		// 晚于类型扫描）按资产路径 Contains(FilterDirectory.Path) 命中并写入每资产覆盖，
		// GetPrimaryAssetRules 中每资产覆盖优先于类型全局规则 → 白名单地图回落首包。
		for (const FString& Trimmed : TrimmedDirs)
		{
			MapCustomRuleLines.Add(FString::Printf(
				TEXT("+CustomPrimaryAssetRules=(PrimaryAssetType=\"Map\",FilterDirectory=(Path=\"%s\"),Rules=(Priority=100,ChunkId=0,CookRule=Unknown,bApplyRecursively=True))"),
				*Trimmed));
		}

		AppendLog(FString::Printf(TEXT("[信息] CDN Chunk 隔离规则已写入 DefaultGame.ini：FirstPakAsset(ChunkId=0, %d 目录) + CdnAsset(ChunkId=100) + Map(全量 ChunkId=100, %d 白名单目录覆盖 ChunkId=0)。"),
			DirParts.Num(), MapCustomRuleLines.Num()), FLinearColor(0.1f, 0.82f, 0.77f));
	}
	else
	{
		AppendLog(TEXT("[信息] 全量进首包：已移除 CDN Chunk 隔离规则，全部资源随包发布（与现状一致）。"));
	}

	// 第一次同步：PrimaryAssetTypesToScan（管理 FirstPakAsset / CdnAsset / Map 三条条目）
	SyncIniSectionByText(IniPath, Section, KeyPrefix, NewManagedLines, [](const FString& Line)
	{
		return Line.Contains(TEXT("FirstPakAsset")) || Line.Contains(TEXT("CdnAsset")) || Line.Contains(TEXT("PrimaryAssetType=\"Map\""));
	});

	// 第二次同步：CustomPrimaryAssetRules（CDN 写入白名单 Map 覆盖，全量模式传空数组清理）
	SyncIniSectionByText(IniPath, Section, TEXT("+CustomPrimaryAssetRules"), MapCustomRuleLines, [](const FString& Line)
	{
		return Line.Contains(TEXT("CustomPrimaryAssetRules"));
	});

	// bShouldManagerDetermineTypeAndName：必须开启。它让 UObject::GetPrimaryAssetId 走
	// UAssetManager::DeterminePrimaryAssetIdForObject（经 AssetPathMap 命中注册 ID）。
	// 蓝图资产（GeneratedClass，Outer 为 package）因此能返回与扫描注册一致的 PrimaryAssetId，
	// 避免 cook 保存时 OnObjectPreSave 报 "Registered PrimaryAssetId ... does not match object's real id"。
	TArray<FString> FlagLines;
	if (bCdnMode)
	{
		FlagLines.Add(TEXT("bShouldManagerDetermineTypeAndName=True"));
	}
	SyncIniSectionByText(IniPath, Section, TEXT("bShouldManagerDetermineTypeAndName"), FlagLines, [](const FString& Line)
	{
		return Line.StartsWith(TEXT("bShouldManagerDetermineTypeAndName="));
	});
}

void SPackPanelWidget::SyncChannelToGameIni()
{
	const FString IniPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultGame.ini"));
	const FString Section = TEXT("QiongQi");

	// ChannelName：客户端运行时 UeDownloadHelper::GetChannel 读取
	TArray<FString> ChannelLines;
	ChannelLines.Add(FString::Printf(TEXT("ChannelName=%s"), *ChannelName));
	SyncIniSectionByText(IniPath, Section, TEXT("ChannelName"), ChannelLines, [](const FString& Line)
	{
		return Line.StartsWith(TEXT("ChannelName="));
	});

	// ResourceVersion：包内版本号，随包固化。客户端 C++ 初始化时读取并与本地版本对比取较大值写回
	//（UeDownloadHelper::SyncLocalVersionFromPackage），保证本地版本记录不低于包内版本。
	TArray<FString> VersionLines;
	VersionLines.Add(FString::Printf(TEXT("ResourceVersion=%s"), *ResourceVersion));
	SyncIniSectionByText(IniPath, Section, TEXT("ResourceVersion"), VersionLines, [](const FString& Line)
	{
		return Line.StartsWith(TEXT("ResourceVersion="));
	});

	// FullInFirstPak：全量资源打入首包标志，随包固化。客户端 C++ 初始化时读取
	//（UeDownloadHelper::IsFullInFirstPak），用于全量进首包模式本地版本对齐后的更新短路判断。
	TArray<FString> FullInFirstPakLines;
	FullInFirstPakLines.Add(FString::Printf(TEXT("FullInFirstPak=%d"), bFullInFirstPak ? 1 : 0));
	SyncIniSectionByText(IniPath, Section, TEXT("FullInFirstPak"), FullInFirstPakLines, [](const FString& Line)
	{
		return Line.StartsWith(TEXT("FullInFirstPak="));
	});

	// IsDebugPackage：打包类型（Debug/Release）固化为调试标志，随包固化。客户端 C++ 初始化时读取
	//（QiongQiGameInstance::IsDebugPackage），打包版 Define.Debug 由此决定：
	// Debug 包=true（切换/记忆服务器等调试逻辑生效），Release 包=false（正式逻辑）。
	TArray<FString> IsDebugLines;
	IsDebugLines.Add(FString::Printf(TEXT("IsDebugPackage=%d"), SelectedPackType == TEXT("Debug") ? 1 : 0));
	SyncIniSectionByText(IniPath, Section, TEXT("IsDebugPackage"), IsDebugLines, [](const FString& Line)
	{
		return Line.StartsWith(TEXT("IsDebugPackage="));
	});

	AppendLog(FString::Printf(TEXT("[信息] 渠道/包内版本/全量进首包/调试标志已写入 DefaultGame.ini：[%s] ChannelName=%s, ResourceVersion=%s, FullInFirstPak=%d, IsDebugPackage=%d"),
		*Section, *ChannelName, *ResourceVersion, bFullInFirstPak ? 1 : 0, SelectedPackType == TEXT("Debug") ? 1 : 0), FLinearColor(0.1f, 0.82f, 0.77f));
}

// ---------------------------------------------------------------------------------------------
// CDN 版本清单
// ---------------------------------------------------------------------------------------------

FString SPackPanelWidget::CalcFileMd5(const FString& FilePath)
{
	IFileHandle* Handle = FPlatformFileManager::Get().GetPlatformFile().OpenRead(*FilePath);
	if (!Handle)
	{
		return FString();
	}

	FMD5 Md5;
	const int64 ChunkSize = 1 << 20; // 1MB 分块，避免大文件整块读入内存
	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(ChunkSize);

	int64 Remaining = Handle->Size();
	while (Remaining > 0)
	{
		const int64 ToRead = FMath::Min<int64>(ChunkSize, Remaining);
		if (!Handle->Read(Buffer.GetData(), ToRead))
		{
			delete Handle;
			return FString();
		}
		Md5.Update(Buffer.GetData(), ToRead);
		Remaining -= ToRead;
	}
	delete Handle;

	uint8 Digest[16];
	Md5.Final(Digest);
	return BytesToHex(Digest, 16);
}

bool SPackPanelWidget::GenerateVersionManifest(const FString& CdnDestDir)
{
	// 扫描 CDN 根目录下的 CDN 资源（资源已展开，无版本/平台子目录）。
	// IoStore 模式下补丁为 .pak/.utoc/.ucas 三件套，全部纳入清单（缺一客户端挂载即失败）。
	const FString PakDir = CdnDestDir;
	TArray<FString> CdnFiles;
	{
		TArray<FString> Found;
		static const TCHAR* CdnExts[] = { TEXT("*.pak"), TEXT("*.utoc"), TEXT("*.ucas") };
		for (const TCHAR* Ext : CdnExts)
		{
			Found.Reset();
			IFileManager::Get().FindFiles(Found, *FPaths::Combine(PakDir, Ext), true, false);
			CdnFiles.Append(Found);
		}
	}

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetStringField(TEXT("channel"), ChannelName);
	// platform 为 CDN 对外平台名（Windows→pc、Android→android、IOS→ios），
	// 与客户端运行时 GetPlatformName() / CDN 目录 {渠道}_{平台} 保持一致。
	Root->SetStringField(TEXT("platform"), GetCDNPlatformName());
	// 资源版本为纯数字，JSON 中以数字形式写入（与本地 SaveLocalVersion / 运行时 GetLocalVersion 保持一致）
	Root->SetNumberField(TEXT("version"), static_cast<double>(FCString::Atoi64(*ResourceVersion)));

	// 1. 当前磁盘实际扫描到的 CDN 资源：name -> (md5, size)，以磁盘最新内容为准
	TMap<FString, TPair<FString, int64>> DiskFiles;
	for (const FString& CdnName : CdnFiles)
	{
		const FString CdnFull = FPaths::Combine(PakDir, CdnName);
		const FString Md5 = CalcFileMd5(CdnFull);
		const int64 Size = IFileManager::Get().FileSize(*CdnFull);
		if (Md5.IsEmpty() || Size <= 0)
		{
			AppendLog(FString::Printf(TEXT("[警告] 跳过无效 CDN 资源: %s"), *CdnFull), FLinearColor::Yellow);
			continue;
		}
		DiskFiles.Add(CdnName, TPair<FString, int64>(Md5, Size));
	}

	// 2. 合并历史版本清单 {版本号}.json（排除当前版本）：同名以历史最高版本记录兜底。
	//    保证最新版本清单恒为全量，首包玩家只拉最新清单也能拿到全部 CDN 资源。
	TMap<FString, TPair<FString, int64>> HistoryFiles;
	{
		TArray<FString> ManifestFiles;
		IFileManager::Get().FindFiles(ManifestFiles, *FPaths::Combine(PakDir, TEXT("*.json")), true, false);

		// 只保留纯数字版本号的历史清单（跳过当前版本），并按版本号升序排序，
		// 保证后续 Add 覆盖已存在键时，最终保留的是历史最高版本的记录。
		ManifestFiles.RemoveAll([this](const FString& ManifestName)
		{
			const FString VerStr = FPaths::GetBaseFilename(ManifestName);
			return !VerStr.IsNumeric() || VerStr == ResourceVersion;
		});
		ManifestFiles.Sort([](const FString& A, const FString& B)
		{
			return FCString::Atoi64(*FPaths::GetBaseFilename(A)) < FCString::Atoi64(*FPaths::GetBaseFilename(B));
		});

		for (const FString& ManifestName : ManifestFiles)
		{
			const FString ManifestPath = FPaths::Combine(PakDir, ManifestName);
			FString JsonStr;
			if (!FFileHelper::LoadFileToString(JsonStr, *ManifestPath))
			{
				AppendLog(FString::Printf(TEXT("[警告] 读取历史清单失败，跳过: %s"), *ManifestPath), FLinearColor::Yellow);
				continue;
			}

			TSharedPtr<FJsonObject> HistoryRoot;
			const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
			if (!FJsonSerializer::Deserialize(Reader, HistoryRoot) || !HistoryRoot.IsValid())
			{
				AppendLog(FString::Printf(TEXT("[警告] 解析历史清单失败，跳过: %s"), *ManifestPath), FLinearColor::Yellow);
				continue;
			}

			const TArray<TSharedPtr<FJsonValue>>* HistoryFilesArr = nullptr;
			if (!HistoryRoot->TryGetArrayField(TEXT("files"), HistoryFilesArr))
			{
				continue;
			}

			for (const TSharedPtr<FJsonValue>& FileValue : *HistoryFilesArr)
			{
				const TSharedPtr<FJsonObject> FileObj = FileValue->AsObject();
				if (!FileObj.IsValid())
				{
					continue;
				}
				FString Name;
				if (!FileObj->TryGetStringField(TEXT("name"), Name) || Name.IsEmpty())
				{
					continue;
				}
				FString Md5;
				FileObj->TryGetStringField(TEXT("md5"), Md5);
				double SizeNum = 0.0;
				FileObj->TryGetNumberField(TEXT("size"), SizeNum);
				// 按版本升序遍历，Add 覆盖已存在键，最终保留历史最高版本记录
				HistoryFiles.Add(Name, TPair<FString, int64>(Md5, static_cast<int64>(SizeNum)));
			}
		}
	}

	// 3. 生成并集 files：先输出磁盘实际扫描到的资源，再补入历史兜底记录
	TArray<TSharedPtr<FJsonValue>> Files;
	for (const auto& Pair : DiskFiles)
	{
		// 资源已展开到 CDN 根目录，name 直接使用资源原始文件名（不再带 Windows/ 平台前缀）
		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("name"), Pair.Key);
		FileObj->SetStringField(TEXT("md5"), Pair.Value.Key);
		FileObj->SetNumberField(TEXT("size"), static_cast<double>(Pair.Value.Value));
		Files.Add(MakeShareable(new FJsonValueObject(FileObj)));
		AppendLog(FString::Printf(TEXT("[清单] CDN 资源: %s  md5=%s  size=%lld"),
			*Pair.Key, *Pair.Value.Key, Pair.Value.Value), FLinearColor(0.6f, 0.6f, 0.6f));
	}

	// 历史有记录但磁盘未扫到的资源：若物理存在则重新计算 md5 补入（防御 FindFiles 遗漏），
	// 若物理不存在则保留历史记录并红色告警，避免清单静默残缺。
	for (const auto& Pair : HistoryFiles)
	{
		if (DiskFiles.Contains(Pair.Key))
		{
			continue;
		}

		const FString CdnFull = FPaths::Combine(PakDir, Pair.Key);
		FString Md5 = Pair.Value.Key;
		int64 Size = Pair.Value.Value;
		if (FPaths::FileExists(CdnFull))
		{
			const FString ActualMd5 = CalcFileMd5(CdnFull);
			if (!ActualMd5.IsEmpty())
			{
				Md5 = ActualMd5;
				Size = IFileManager::Get().FileSize(*CdnFull);
			}
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[错误] CDN 根目录缺少历史清单中的文件: %s（历史记录已保留，请确认该资源已上传，否则玩家下载后将无法加载）"), *CdnFull), FLinearColor::Red);
		}

		TSharedPtr<FJsonObject> FileObj = MakeShareable(new FJsonObject);
		FileObj->SetStringField(TEXT("name"), Pair.Key);
		FileObj->SetStringField(TEXT("md5"), Md5);
		FileObj->SetNumberField(TEXT("size"), static_cast<double>(Size));
		Files.Add(MakeShareable(new FJsonValueObject(FileObj)));
		AppendLog(FString::Printf(TEXT("[清单] 历史保留资源: %s  md5=%s  size=%lld"),
			*Pair.Key, *Md5, Size), FLinearColor(0.6f, 0.6f, 0.6f));
	}
	Root->SetArrayField(TEXT("files"), Files);

	if (Files.Num() == 0)
	{
		AppendLog(FString::Printf(TEXT("[警告] 版本目录内未找到 CDN 资源（.pak/.utoc/.ucas）: %s"), *PakDir), FLinearColor::Yellow);
	}

	FString JsonStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		AppendLog(TEXT("[错误] 序列化版本清单失败！"), FLinearColor::Red);
		return false;
	}

	// 清单文件名用版本号命名：{版本号}.json
	const FString ManifestPath = FPaths::Combine(CdnDestDir, FString::Printf(TEXT("%s.json"), *ResourceVersion));
	if (FFileHelper::SaveStringToFile(JsonStr, *ManifestPath))
	{
		AppendLog(FString::Printf(TEXT("[清单] 版本清单已生成: %s"), *ManifestPath), FLinearColor(0.f, 1.f, 0.f));
		return true;
	}

	AppendLog(FString::Printf(TEXT("[错误] 写入版本清单失败: %s"), *ManifestPath), FLinearColor::Red);
	return false;
}

void SPackPanelWidget::CleanReleaseDirectory()
{
	const FString ProjectRoot = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());

	// 仅清理 Release 交付目录（CDN 累积目录 + 整包留档）。
	// 注意：清空后历史 CDN pak 与版本清单将丢失，本次打包产出的清单将作为全新基线。
	TArray<FString> ReleaseDirs;
	ReleaseDirs.Add(FPaths::Combine(ProjectRoot, TEXT("Release")));

	for (const FString& ReleaseDir : ReleaseDirs)
	{
		const FString FullDir = FPaths::ConvertRelativePathToFull(ReleaseDir);

		// 防御性校验：确保目标确实是项目目录下的 Release 目录，避免误删其他目录
		if (!FullDir.StartsWith(ProjectRoot) ||
			!FPaths::IsUnderDirectory(FullDir, ProjectRoot))
		{
			AppendLog(FString::Printf(TEXT("[错误] 清理路径异常，已跳过: %s"), *FullDir), FLinearColor::Red);
			continue;
		}

		if (!FPaths::DirectoryExists(FullDir))
		{
			AppendLog(FString::Printf(TEXT("[清理] Release 目录不存在，无需清理: %s"), *FullDir), FLinearColor::Yellow);
			continue;
		}

		AppendLog(FString::Printf(TEXT("[清理] 正在清空 Release 目录: %s"), *FullDir), FLinearColor::Yellow);

		// 递归删除目录下所有文件与子目录，随后重建空目录，保证后续复制可直接写入
		if (IFileManager::Get().DeleteDirectory(*FullDir, false, true))
		{
			IFileManager::Get().MakeDirectory(*FullDir, true);
			AppendLog(FString::Printf(TEXT("[清理] Release 目录已清空: %s"), *FullDir), FLinearColor::Green);
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[警告] Release 目录清理未完全成功（可能有文件被占用），请检查后重试: %s"), *FullDir), FLinearColor::Yellow);
		}
	}
}

void SPackPanelWidget::CopyResultsToRelease()
{
	const FString ReleaseRoot = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Release")));
	// CDN 交付目录 {渠道}_{CDN平台名}：对外平台名（pc/android/ios），与客户端拼接的 CDN URL 一致
	const FString ChannelPlatform = FString::Printf(TEXT("%s_%s"), *ChannelName, *GetCDNPlatformName());
	const FString CdnDestDir = FPaths::Combine(ReleaseRoot, ChannelPlatform);
	const FString SaveRoot = GetHotPatcherOutputRoot();

	AppendLog(TEXT(""));
	AppendLog(TEXT("============== 复制到 Release 目录 =============="), FLinearColor(0.f, 1.f, 1.f));

	// 1. 复制本次 HotPatcher 产生的 CDN 资源（{版本}/{HotPatcher内部平台名}/ 内容）直接展开到 CDN 根目录：
	//    复制 IoStore 三件套 .pak/.utoc/.ucas（保留原始文件名），三者缺一客户端挂载即失败；
	//    Metadatas 目录、*.PakCommands.txt 等描述文件一律不复制。
	//    注意：此处取 HotPatcher 输出目录用的是内部平台名（Windows/Android/IOS），与 CDN 对外名无关。
	//    全量进首包·首次打包：本次产物为全量资源且已随包进首包，跳过 CDN 复制，
	//    仅生成空版本清单（files 为空），客户端本地版本对齐包内版本后短路跳过下载。
	const FString VersionDir = FPaths::Combine(SaveRoot, ResourceVersion);
	const FString WindowsDir = FPaths::Combine(VersionDir, GetHotPatcherPlatformName());
	const bool bSkipCdnCopy = bFullInFirstPak && bFirstFullInFirstPak;
	if (bSkipCdnCopy)
	{
		AppendLog(TEXT("[信息] 全量进首包·首次打包：全量产物随包进首包，不复制 CDN，仅生成空版本清单。"), FLinearColor(0.f, 1.f, 0.f));
	}
	else if (FPaths::DirectoryExists(WindowsDir))
	{
		IFileManager::Get().MakeDirectory(*CdnDestDir, true);

		// 收集 IoStore 三件套（.pak/.utoc/.ucas），首包为 IoStore 容器时补丁必须完整下发三件套
		TArray<FString> CdnFiles;
		{
			TArray<FString> Found;
			static const TCHAR* CdnExts[] = { TEXT("*.pak"), TEXT("*.utoc"), TEXT("*.ucas") };
			for (const TCHAR* Ext : CdnExts)
			{
				Found.Reset();
				IFileManager::Get().FindFiles(Found, *FPaths::Combine(WindowsDir, Ext), true, false);
				CdnFiles.Append(Found);
			}
		}

		int32 CopiedCount = 0;
		for (const FString& CdnName : CdnFiles)
		{
			const FString CdnFullPath = FPaths::Combine(WindowsDir, CdnName);
			const FString DestFullPath = FPaths::Combine(CdnDestDir, CdnName);
			// IFileManager::Copy 返回 ECopyResult（uint32）：COPY_OK(0) 成功，非 0 为失败，
			// 不能直接转 bool（既触发 C4800 又弄反语义），须显式与 COPY_OK 比较。
			const uint32 CopyResult = IFileManager::Get().Copy(*DestFullPath, *CdnFullPath, true);
			bool bOk = (CopyResult == COPY_OK);
			if (!bOk)
			{
				// 回退校验：Copy 失败（常见于目标文件被编辑器/杀毒占用），
				// 若目标已存在且 md5 与源一致，则视为复制成功，避免误报错误。
				const FString SrcMd5 = CalcFileMd5(CdnFullPath);
				const FString DstMd5 = CalcFileMd5(DestFullPath);
				if (!SrcMd5.IsEmpty() && SrcMd5 == DstMd5)
				{
					AppendLog(FString::Printf(TEXT("[复制] %s（Copy 被占用，md5 校验一致，视为已存在）"), *CdnName), FLinearColor(0.f, 1.f, 0.f));
					bOk = true;
				}
				else
				{
					AppendLog(FString::Printf(TEXT("[错误] 复制 CDN 资源失败: %s"), *CdnFullPath), FLinearColor::Red);
				}
			}

			if (bOk)
			{
				++CopiedCount;
			}
		}

		if (CopiedCount > 0)
		{
			AppendLog(FString::Printf(TEXT("[复制] CDN 资源已复制 %d 个文件（.pak/.utoc/.ucas，保留原始文件名）: %s -> %s"), CopiedCount, *WindowsDir, *CdnDestDir), FLinearColor(0.f, 1.f, 0.f));
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[警告] 未找到本次版本的 CDN 资源（.pak/.utoc/.ucas）: %s"), *WindowsDir), FLinearColor::Yellow);
		}
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[警告] 未找到本次版本资源目录: %s"), *WindowsDir), FLinearColor::Yellow);
	}

	// 2. 生成 CDN 版本清单 {版本号}.json（CDN 根目录，文件名用版本号命名）
	//    全量进首包·首次打包时 CDN 目录可能尚不存在，先创建再生成空清单
	if (bSkipCdnCopy || FPaths::DirectoryExists(CdnDestDir))
	{
		IFileManager::Get().MakeDirectory(*CdnDestDir, true);
		GenerateVersionManifest(CdnDestDir);
	}

	// 3. 复制整包 -> Release/{平台}（去掉 Build 中间目录，整包直接放外层，仅勾选了打整包时）
	if (bFullPackage)
	{
		const FString FullPackageDir = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(GetOutputRootPath(), SelectedPlatform, TEXT("FullPackage")));
		const FString BuildDestDir = FPaths::Combine(ReleaseRoot, SelectedPlatform);
		if (FPaths::DirectoryExists(FullPackageDir))
		{
			IFileManager::Get().MakeDirectory(*BuildDestDir, true);
			if (FPlatformFileManager::Get().GetPlatformFile().CopyDirectoryTree(*BuildDestDir, *FullPackageDir, true))
			{
				AppendLog(FString::Printf(TEXT("[复制] 整包已复制: %s -> %s"), *FullPackageDir, *BuildDestDir), FLinearColor(0.f, 1.f, 0.f));

				// CDN 模式：剔除 CDN chunk 容器（pakchunk100*，含 .pak/.utoc/.ucas/.sig 等），
				// 白名单资源已在 chunk0 首包内，CDN 资源由 HotPatcher 补丁 pak 经 CDN 下发。
				if (!bFullInFirstPak)
				{
					TArray<FString> Removed;
					IFileManager::Get().FindFilesRecursive(
						Removed, *BuildDestDir, TEXT("pakchunk100*"), true, false);
					for (const FString& ChunkFile : Removed)
					{
						if (IFileManager::Get().Delete(*ChunkFile, false, true))
						{
							AppendLog(FString::Printf(TEXT("[剔除] CDN chunk 容器: %s"), *ChunkFile), FLinearColor(1.f, 0.65f, 0.f));
						}
						else
						{
							AppendLog(FString::Printf(TEXT("[错误] 剔除 CDN chunk 容器失败: %s"), *ChunkFile), FLinearColor::Red);
						}
					}
					AppendLog(FString::Printf(TEXT("[信息] CDN chunk（pakchunk100*）剔除完成，共 %d 个文件。"), Removed.Num()), FLinearColor(0.f, 1.f, 0.f));
				}
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

	// 4. 打整包且本次 HotPatcher 产物已生成时，写入主版本标记文件 {版本Id}_Release.json，
	//    供历史版本下拉框识别整包主版本（纯补丁不写；整包流程失败不会走到本函数）
	if (bFullPackage && FPaths::DirectoryExists(VersionDir))
	{
		WriteMainVersionMarker(VersionDir);
	}

	AppendLog(TEXT("============== 复制完成 =============="), FLinearColor(0.f, 1.f, 1.f));
}

void SPackPanelWidget::WriteMainVersionMarker(const FString& VersionDir)
{
	const FString MarkerPath = FPaths::Combine(VersionDir, ResourceVersion + MainVersionMarkerFileSuffix);

	TSharedPtr<FJsonObject> Root = MakeShareable(new FJsonObject);
	Root->SetStringField(TEXT("versionId"), ResourceVersion);
	Root->SetStringField(TEXT("channel"), ChannelName);
	Root->SetStringField(TEXT("platform"), GetHotPatcherPlatformName());
	Root->SetStringField(TEXT("time"), FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")));

	FString JsonStr;
	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonStr);
	if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer))
	{
		AppendLog(FString::Printf(TEXT("[错误] 序列化主版本标记失败: %s"), *MarkerPath), FLinearColor::Red);
		return;
	}

	if (FFileHelper::SaveStringToFile(JsonStr, *MarkerPath))
	{
		AppendLog(FString::Printf(TEXT("[信息] 已写入主版本标记（整包）: %s"), *MarkerPath), FLinearColor(0.f, 1.f, 0.f));
	}
	else
	{
		AppendLog(FString::Printf(TEXT("[错误] 写入主版本标记失败: %s"), *MarkerPath), FLinearColor::Red);
	}
}

// HotPatcher 内部平台名：仅用于 HotPatcher 产物目录、TargetPlatform 枚举与补丁配置文件名，
// 不参与 CDN 对外路径/清单（对外平台名见 GetCDNPlatformName，两者已分离）。
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

// CDN 对外平台名：Windows→pc、Android→android、IOS→ios。
// 用于 Release/CDN 目录 {渠道}_{平台} 与版本清单 platform 字段，
// 必须与客户端运行时 UeDownloadHelper::GetPlatformName() 的映射保持一致。
FString SPackPanelWidget::GetCDNPlatformName() const
{
	if (SelectedPlatform == TEXT("Android"))
	{
		return TEXT("android");
	}
	if (SelectedPlatform == TEXT("IOS"))
	{
		return TEXT("ios");
	}
	return TEXT("pc");
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
	// 首次标志仅在本函数内按"是否有历史基础版本"重新判定，先重置避免上次打包残留影响 CopyResultsToRelease
	bFirstFullInFirstPak = false;

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

	// IoStore 补丁支持：项目打包配置开启 IoStore（bUseIoStore=True）时，首包为 .utoc/.ucas 容器，
	// 补丁必须同步生成 .pak/.utoc/.ucas 三件套；否则 UE5.5 挂载补丁 .pak 时因缺少同名 .utoc 而整体失败。
	// BasePackageStagedRootDir 指向整包输出目录（内含 {项目}/Content/Paks/global.utoc），IoStore 生成器
	// 从其中定位首包 global 容器作为基座。补丁容器名（{版本}_{平台}_001_P）与首包容器名（pakchunk0-*）不一致，
	// 无法做 IoStore diff，故 bGenerateDiffPatch 固定为 false（全量生成当前补丁资源的容器）。
	bool bUseIoStore = false;
	GConfig->GetBool(TEXT("/Script/UnrealEd.ProjectPackagingSettings"), TEXT("bUseIoStore"), bUseIoStore, GGameIni);
	if (bUseIoStore)
	{
		const FString FullPackageDir = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(GetOutputRootPath(), SelectedPlatform, TEXT("FullPackage")));
		const FString GlobalUtocPath = FPaths::Combine(
			FullPackageDir, FApp::GetProjectName(), TEXT("Content/Paks/global.utoc"));

		if (FPaths::FileExists(GlobalUtocPath))
		{
			FIoStorePlatformContainers IoStoreContainers;
			IoStoreContainers.BasePackageStagedRootDir.Path = FullPackageDir;
			IoStoreContainers.bGenerateDiffPatch = false;

			PatchSettings.IoStoreSettings.bIoStore = true;
			PatchSettings.IoStoreSettings.PlatformContainers.Add(PlatformEnum, IoStoreContainers);
			AppendLog(FString::Printf(TEXT("[信息] 已开启 IoStore 补丁：BasePackageStagedRootDir=%s"), *FullPackageDir), FLinearColor(0.1f, 0.82f, 0.77f));
		}
		else
		{
			AppendLog(FString::Printf(TEXT("[警告] 项目已开启 IoStore 但未找到首包 global.utoc（%s），补丁将退回普通 .pak，客户端挂载会失败！请先勾选\"打整包\"生成 IoStore 首包。"), *GlobalUtocPath), FLinearColor::Yellow);
		}
	}

	// 资源扫描范围
	FAssetScanConfig& ScanConfig = PatchSettings.GetAssetScanConfigRef();
	if (bFullInFirstPak)
	{
		// 全量进首包：保持模板配置，未配置则默认扫描 /Game/ 全部内容（与现有行为一致）
		if (ScanConfig.AssetIncludeFilters.Num() == 0 && ScanConfig.IncludeSpecifyAssets.Num() == 0)
		{
			FDirectoryPath GameDir;
			GameDir.Path = TEXT("/Game/");
			ScanConfig.AssetIncludeFilters.Add(GameDir);
			AppendLog(TEXT("[信息] 未配置资源扫描范围，默认全量扫描 /Game/ 内容。"));
		}
	}
	else
	{
		// 非全量（CDN 模式）：扫描范围 = /Game/ 全量 - 随包目录白名单。
		// 未列入白名单的目录资源全部进入 CDN 补丁 pak，随包资源（白名单）从 CDN 产物中剔除。
		ScanConfig.AssetIncludeFilters.Reset();
		ScanConfig.IncludeSpecifyAssets.Reset();
		ScanConfig.AssetIgnoreFilters.Reset();

		FDirectoryPath GameDir;
		GameDir.Path = TEXT("/Game/");
		ScanConfig.AssetIncludeFilters.Add(GameDir);

		for (const FString& Dir : WhiteListRaw)
		{
			FDirectoryPath IgnoreDir;
			IgnoreDir.Path = Dir;
			ScanConfig.AssetIgnoreFilters.Add(IgnoreDir);
		}

		AppendLog(FString::Printf(TEXT("[信息] CDN 模式：扫描范围 /Game/ 全量，排除随包白名单 %d 个目录，其余资源全部进入 CDN pak。"),
			WhiteListRaw.Num()), FLinearColor(0.1f, 0.82f, 0.77f));
	}

	// 主版本（基础版本）策略
	if (bFullPackage)
	{
		// 打整包：资源版本 = 面板资源版本（VersionId 已设置），主版本仍按"是否全量资源打入首包"决定
		if (bFullInFirstPak)
		{
			// 全量进首包：有历史基础版本则相对其做增量补丁（首包已含基线资源，补丁仅含增量）；
			// 无历史基础版本则为首次打包：全量打入首包，CDN 不产出增量（CopyResultsToRelease 据此生成空清单）。
			const FString BaseJson = FindLatestBaseVersionJson(PatchSettings.GetSaveAbsPath());
			if (BaseJson.IsEmpty())
			{
				bFirstFullInFirstPak = true;
				PatchSettings.bByBaseVersion = false;
				PatchSettings.BaseVersion.FilePath.Empty();
				AppendLog(TEXT("[信息] 全量进首包·首次打包：所有资源打入首包，CDN 不产出增量。"), FLinearColor(0.f, 1.f, 0.f));
			}
			else
			{
				bFirstFullInFirstPak = false;
				PatchSettings.bByBaseVersion = true;
				PatchSettings.BaseVersion.FilePath = BaseJson;
				AppendLog(FString::Printf(TEXT("[信息] 全量进首包·增量补丁：基础版本 %s，补丁仅含相对首包的增量。"), *BaseJson), FLinearColor(0.1f, 0.82f, 0.77f));
			}
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
			// 全量进首包·纯补丁：首包已含基线资源，仅需相对所选主版本的增量补丁（无需全量重打）
			PatchSettings.bByBaseVersion = true;
			bFirstFullInFirstPak = false; // 纯补丁必有主版本（历史基线），非首次
			AppendLog(FString::Printf(TEXT("[信息] 全量进首包·增量补丁，主版本: %s (%s)"),
				*SelectedBaseVersion, *SelectedBaseVersionPath), FLinearColor(0.1f, 0.82f, 0.77f));
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

		// 仅识别整包主版本标记文件 {版本Id}_Release.json（位于 {版本Id}/ 目录下）。
		// 纯补丁版本目录不生成该标记，不作为整包自动选择的基础版本，避免误选补丁版本作 base。
		const FString ParentDirName = FPaths::GetCleanFilename(FPaths::GetPath(File));
		if (ParentDirName.IsEmpty() || ParentDirName.Contains(ResourceVersion))
		{
			continue;
		}
		if (FileName != ParentDirName + MainVersionMarkerFileSuffix)
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
