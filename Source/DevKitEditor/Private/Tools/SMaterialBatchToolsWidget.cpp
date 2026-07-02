#include "Tools/SMaterialBatchToolsWidget.h"

#include "HAL/PlatformApplicationMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SMaterialBatchToolsWidget"

namespace
{
const TCHAR* DefaultMapPath = TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_corridor_S_Dungeon/L1_CommonLevel_corridor_01b");
const TCHAR* DefaultClusterName = TEXT("Corridor_01b");
const TCHAR* DefaultTierName = TEXT("Mid");
const TCHAR* DefaultRequireTag = TEXT("EnvBatch.Source.");

FString GetEditorCmdPath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::EngineDir(), TEXT("Binaries/Win64/UnrealEditor-Cmd.exe")));
}

FString GetProjectFilePath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
}

FString GetReportFolderPath()
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Docs/GeneratedReports/CommandletReports")));
}
}

void SMaterialBatchToolsWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SBorder)
		.Padding(16.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "关卡模型材质合批"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT(
						"Description",
						"使用方式：先用“环境合批标记”给候选 Actor 写入 EnvBatch.Source.<Level>.<Prop|Building>.<Instance|Batched>.<Serial>。Prop.Batched 是自动合批主入口；Building 默认用于组织和审计。然后在这里填写 Map、Cluster、Tier、RequireTag，先运行 dry-run 查看候选、拒绝原因、VTC/UDIM/Proxy/Batch Material 计划。"))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT(
						"ApplyWarning",
						"注意：当前 MaterialBatchBuild 的 -Apply 全量关卡替换仍然被禁用。工具只允许安全启动 dry-run；写入 VT Atlas、Mapping、Proxy Mesh、Batch Material 等生成资产时，请复制 partial apply 命令，由 TA 审核报告后执行。"))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("MapLabel", "Map：要扫描的关卡包路径"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SAssignNew(MapTextBox, SEditableTextBox)
					.Text(FText::FromString(DefaultMapPath))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("ClusterLabel", "Cluster：输出目录和报告中的合批组名"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SAssignNew(ClusterTextBox, SEditableTextBox)
					.Text(FText::FromString(DefaultClusterName))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("TierLabel", "Tier：Epic / High / Mid / Low"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SAssignNew(TierTextBox, SEditableTextBox)
					.Text(FText::FromString(DefaultTierName))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("RequireTagLabel", "RequireTag：只扫描带此前缀的 Actor Tag"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(RequireTagTextBox, SEditableTextBox)
					.Text(FText::FromString(DefaultRequireTag))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RunDryRun", "运行 DryRun"))
						.ToolTipText(LOCTEXT("RunDryRunTooltip", "启动 UnrealEditor-Cmd 执行 MaterialBatchBuild dry-run，只生成报告，不写资产。"))
						.OnClicked(this, &SMaterialBatchToolsWidget::RunDryRunCommandlet)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("CopyDryRun", "复制 DryRun 命令"))
						.ToolTipText(LOCTEXT("CopyDryRunTooltip", "复制可在 PowerShell 中运行的 dry-run 命令。"))
						.OnClicked(this, &SMaterialBatchToolsWidget::CopyDryRunCommand)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("CopyPartialApply", "复制生成资产命令"))
						.ToolTipText(LOCTEXT("CopyPartialApplyTooltip", "复制 partial apply 命令，只写 VT Atlas、Mapping、Proxy Mesh、Batch Material 等生成资产，不做整关替换。"))
						.OnClicked(this, &SMaterialBatchToolsWidget::CopyPartialApplyCommand)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("OpenReportFolder", "打开报告目录"))
						.OnClicked(this, &SMaterialBatchToolsWidget::OpenReportFolder)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "就绪。建议先复制或运行 dry-run，查看 MaterialBatchBuildReport.md 后再执行生成资产命令。"))
					.AutoWrapText(true)
				]
			]
		]
	];
}

FReply SMaterialBatchToolsWidget::RunDryRunCommandlet()
{
	const FString EditorCmd = GetEditorCmdPath();
	const FString Args = BuildCommandletArgs(false);
	FProcHandle ProcHandle = FPlatformProcess::CreateProc(*EditorCmd, *Args, true, false, false, nullptr, 0, nullptr, nullptr);
	const bool bStarted = ProcHandle.IsValid();
	if (bStarted)
	{
		FPlatformProcess::CloseProc(ProcHandle);
	}

	SetStatus(FText::Format(
		LOCTEXT("RunStatus", "DryRun commandlet {0}。报告目录：{1}"),
		bStarted ? LOCTEXT("Started", "已启动") : LOCTEXT("StartFailed", "启动失败"),
		FText::FromString(GetReportFolderPath())));
	return FReply::Handled();
}

FReply SMaterialBatchToolsWidget::CopyDryRunCommand()
{
	const FString Command = BuildCommand(false, true);
	FPlatformApplicationMisc::ClipboardCopy(*Command);
	SetStatus(LOCTEXT("CopiedDryRun", "已复制 DryRun 命令到剪贴板。"));
	return FReply::Handled();
}

FReply SMaterialBatchToolsWidget::CopyPartialApplyCommand()
{
	const FString Command = BuildCommand(true, true);
	FPlatformApplicationMisc::ClipboardCopy(*Command);
	SetStatus(LOCTEXT("CopiedPartialApply", "已复制生成资产命令到剪贴板。执行前请先审查 dry-run 报告；该命令不会做整关替换。"));
	return FReply::Handled();
}

FReply SMaterialBatchToolsWidget::OpenReportFolder()
{
	const FString ReportFolder = GetReportFolderPath();
	FPlatformProcess::ExploreFolder(*ReportFolder);
	SetStatus(FText::Format(LOCTEXT("ReportFolderStatus", "已请求打开报告目录：{0}"), FText::FromString(ReportFolder)));
	return FReply::Handled();
}

FString SMaterialBatchToolsWidget::BuildCommand(bool bPartialApply, bool bForPowerShell) const
{
	const FString Prefix = bForPowerShell ? TEXT("& ") : FString();
	return FString::Printf(
		TEXT("%s\"%s\" %s"),
		*Prefix,
		*GetEditorCmdPath(),
		*BuildCommandletArgs(bPartialApply));
}

FString SMaterialBatchToolsWidget::BuildCommandletArgs(bool bPartialApply) const
{
	const FString ApplySwitches = bPartialApply
		? TEXT(" -ApplyVTAtlasOnly -ApplyMappingOnly -ApplyPropertyTextureOnly -ApplyProxyMeshOnly -ApplyBatchMaterialOnly")
		: FString();

	return FString::Printf(
		TEXT("\"%s\" -run=MaterialBatchBuild -Map=%s -Cluster=%s -Tier=%s -TextureBackend=VTAtlas -RequireTag=%s%s -unattended -nopause -NoSound -NullRHI"),
		*GetProjectFilePath(),
		*GetTextOrDefault(MapTextBox, DefaultMapPath),
		*GetTextOrDefault(ClusterTextBox, DefaultClusterName),
		*GetTextOrDefault(TierTextBox, DefaultTierName),
		*GetTextOrDefault(RequireTagTextBox, DefaultRequireTag),
		*ApplySwitches);
}

FString SMaterialBatchToolsWidget::GetTextOrDefault(const TSharedPtr<SEditableTextBox>& TextBox, const FString& DefaultValue) const
{
	FString Value = TextBox.IsValid() ? TextBox->GetText().ToString() : FString();
	Value.TrimStartAndEndInline();
	return Value.IsEmpty() ? DefaultValue : Value;
}

void SMaterialBatchToolsWidget::SetStatus(const FText& InStatus) const
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(InStatus);
	}
}

#undef LOCTEXT_NAMESPACE
