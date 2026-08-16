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

template <typename ItemType>
class SListView;

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
	/** 清理打包目录勾选（打包开始前清空 Release 目录） */
	TSharedPtr<SCheckBox> CleanReleaseCheckBox;

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
	/** 全量进首包·本次是否为首次打包（GenerateHotPatcherConfig 在 HotPatcher 打包前判定：无历史基础版本时置 true）。
	 *  CopyResultsToRelease 据此跳过本次全量 pak 的 CDN 复制，仅生成空版本清单（files 为空）。 */
	bool bFirstFullInFirstPak = false;
	/** 是否在打包开始前清空 Release 目录（默认不勾选，避免误删） */
	bool bCleanRelease = false;
	/** 是否因本地无历史版本而锁定"是否打整包"选项 */
	bool bFullPackageLocked = false;

	EPackStage CurrentStage = EPackStage::Idle;
	TSharedPtr<FPackProcessWorker> ProcessWorker;
	/** 当前阶段（UAT / HotPatcher）子进程输出的累积副本，供失败时特征匹配诊断（DLL 占用等） */
	TArray<FString> LastStageOutput;

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
	/** UAT 阶段失败时对 LastStageOutput 做特征匹配，输出可操作的解决指引（如编辑器 DLL 被占用导致 LNK1104） */
	void DiagnoseUATFailure();
	void FinishPack(bool bSuccess);
	void SetProcessing(bool bProcessing);

	/** 打包成功后把本次 CDN 资源与整包复制到 Release 目录 */
	void CopyResultsToRelease();

	/** 打整包成功后写入主版本标记文件 {版本Id}_Release.json（含版本号/渠道/平台/时间），供历史版本下拉框识别整包主版本 */
	void WriteMainVersionMarker(const FString& VersionDir);

	/** 勾选"清理打包目录"时，在打包开始前清空 Release 交付目录（CDN 累积目录 + 整包留档），从干净状态重新交付 */
	void CleanReleaseDirectory();

	/** UAT 中间输出根目录（Build/Pack/Output/{渠道}） */
	FString GetOutputRootPath() const;
	/** HotPatcher 资源输出根目录（{项目}/HotPatcherRes/{渠道}_{平台}） */
	FString GetHotPatcherOutputRoot() const;
	/** HotPatcher 内部平台名（Windows/Android/IOS）：仅用于 HotPatcher 产物目录、TargetPlatform 枚举与补丁配置文件名，不对外暴露 */
	FString GetHotPatcherPlatformName() const;
	/** CDN 对外平台名（Windows→pc、Android→android、IOS→ios）：用于 Release/CDN 目录 {渠道}_{平台} 与版本清单 platform 字段，与客户端运行时 GetPlatformName() 保持一致 */
	FString GetCDNPlatformName() const;
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

	// ---- 随包目录白名单（Git 提交共享）----
	TArray<TSharedPtr<FString>> WhiteListPaths;      // SListView 数据源
	TArray<FString> WhiteListRaw;                    // 实际目录列表
	TSharedPtr<SEditableTextBox> WhiteListInputBox;
	TSharedPtr<SListView<TSharedPtr<FString>>> WhiteListView;

	/** 白名单配置文件路径：{项目}/Config/CDNInFirstPak.ini */
	static FString GetWhiteListConfigPath();
	/** 从 Config/CDNInFirstPak.ini 加载白名单（[WhiteList] 节 Paths= 列表，兼容旧版 +Paths=），文件不存在时写入默认值 */
	void LoadWhiteListConfig();
	/** 把当前白名单写回 Config/CDNInFirstPak.ini */
	void SaveWhiteListConfig();
	void OnAddWhiteListClicked();
	void OnRemoveWhiteList(TSharedPtr<FString> InItem);
	/** 从任意 JSON 文件导入白名单（覆盖当前列表） */
	void OnImportWhiteListClicked();
	/** 把当前白名单导出到任意 JSON 文件 */
	void OnExportWhiteListClicked();
	void RefreshWhiteListView();

	// ---- CDN Chunk 隔离 ----
	/**
	 * 同步 Chunk 分配规则到 Config/DefaultGame.ini 的 AssetManagerSettings。
	 * CDN 模式（bCdnMode=true）：
	 *  - FirstPakAsset(ChunkId=0, 高优先级 100, 白名单目录)：白名单普通资源进首包；
	 *  - CdnAsset(ChunkId=100, 低优先级 10, /Game 全量)：非白名单普通资源归 CDN chunk100 被剔除；
	 *  - Map(引擎原生类型, ChunkId=100, /Game 全量)：引擎对 .umap 硬编码注册为 Map 类型，自定义类型
	 *    无法覆盖（日志 "Ignoring ... - Conflicts with Map"），故补 Map 全量规则使地图默认归 CDN；
	 *  - +CustomPrimaryAssetRules(Map, 白名单目录, ChunkId=0)：白名单目录内地图按路径每资产覆盖回首包。
	 * 全量模式（bCdnMode=false）：移除上述全部规则，恢复默认全部资源进 chunk0（行为与现状一致）。
	 */
	void SyncChunkRulesToGameIni(bool bCdnMode);

	/** 把面板渠道与包内版本号写入 Config/DefaultGame.ini（[QiongQi] ChannelName / ResourceVersion），随包固化供运行时读取 */
	void SyncChannelToGameIni();

	// ---- CDN 版本清单 ----
	/** 计算文件 MD5（分块读取，大文件安全），文件不存在返回空串 */
	static FString CalcFileMd5(const FString& FilePath);
	/**
	 * 在 CDN 根目录生成版本清单 {版本号}.json。
	 * CDN pak 保留原始文件名，files 含 name（CDN 根目录下的文件名）、md5 与 size。
	 * 与历史版本清单做文件并集兜底：CDN 根缺失的历史文件保留历史记录并输出红色告警，
	 * 保证最新版本清单恒为全量，首包玩家只拉最新清单也能拿到全部 CDN 资源。
	 */
	bool GenerateVersionManifest(const FString& CdnDestDir);

	// ---- UI 辅助 ----
	void AppendLog(const FString& Text, const FLinearColor& Color = FLinearColor::White);
};
