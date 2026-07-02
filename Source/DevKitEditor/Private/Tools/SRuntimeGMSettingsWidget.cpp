#include "Tools/SRuntimeGMSettingsWidget.h"

#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "IDetailsView.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "Styling/AppStyle.h"
#include "System/YogRuntimeGMSettings.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SRuntimeGMSettingsWidget"

void SRuntimeGMSettingsWidget::Construct(const FArguments& InArgs)
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bHideSelectionTip = true;
	DetailsArgs.bLockable = false;
	DetailsArgs.bSearchInitialKeyFocus = false;
	DetailsArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;

	SettingsDetailsView = PropertyModule.CreateDetailView(DetailsArgs);
	SettingsDetailsView->SetObject(GetMutableDefault<UYogRuntimeGMSettings>());

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
					.Text(LOCTEXT("Title", "运行时 GM 配置"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "这里配置游戏内 F12 Runtime GM 面板使用的默认武器、敌人、刷敌数量和刷敌半径。打包快速性能测试使用 Development Win64，F12 面板会保留；Shipping 包默认不启用。"))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("SaveSettings", "保存 GM 配置"))
						.ToolTipText(LOCTEXT("SaveSettingsTooltip", "把当前面板中的 Yog Runtime GM 设置保存到项目配置。"))
						.OnClicked(this, &SRuntimeGMSettingsWidget::SaveSettings)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.Text(LOCTEXT("RunPackage", "打包快速性能测试"))
						.ToolTipText(LOCTEXT("RunPackageTooltip", "运行 PackagePlayablePerfTestWin64.bat，生成 Development Win64 可游玩性能测试包。"))
						.OnClicked(this, &SRuntimeGMSettingsWidget::RunPlayablePerfPackage)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("OpenScriptFolder", "打开脚本目录"))
						.ToolTipText(LOCTEXT("OpenScriptFolderTooltip", "打开快速打包脚本所在目录，方便手动执行或查看日志。"))
						.OnClicked(this, &SRuntimeGMSettingsWidget::OpenPackageScriptFolder)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SNew(STextBlock)
					.Text(this, &SRuntimeGMSettingsWidget::GetPackageScriptText)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 10.f)
				[
					SAssignNew(StatusTextBlock, STextBlock)
					.Text(LOCTEXT("InitialStatus", "就绪。配置资产后点击“保存 GM 配置”，再进行 PIE 或快速打包测试。"))
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.f)
				[
					SettingsDetailsView.ToSharedRef()
				]
			]
		]
	];
}

FReply SRuntimeGMSettingsWidget::SaveSettings()
{
	if (UYogRuntimeGMSettings* Settings = GetMutableDefault<UYogRuntimeGMSettings>())
	{
		Settings->SaveConfig(CPF_Config, *Settings->GetDefaultConfigFilename());
		if (GConfig)
		{
			GConfig->Flush(false, Settings->GetDefaultConfigFilename());
		}

		SetStatus(LOCTEXT("SavedStatus", "已保存 Runtime GM 配置。PIE 和 Development 快速测试包会读取这些默认值。"));
	}
	return FReply::Handled();
}

FReply SRuntimeGMSettingsWidget::RunPlayablePerfPackage()
{
	const FString BatchPath = GetPackageBatchPath();
	if (!FPaths::FileExists(BatchPath))
	{
		SetStatus(FText::Format(LOCTEXT("PackageScriptMissing", "找不到快速打包脚本：{0}"), FText::FromString(BatchPath)));
		return FReply::Handled();
	}

	FString CmdExe = FPlatformMisc::GetEnvironmentVariable(TEXT("ComSpec"));
	if (CmdExe.IsEmpty())
	{
		CmdExe = TEXT("C:\\Windows\\System32\\cmd.exe");
	}
	const FString Args = FString::Printf(TEXT("/C \"\"%s\"\""), *BatchPath);
	const FProcHandle ProcHandle = FPlatformProcess::CreateProc(
		*CmdExe,
		*Args,
		true,
		false,
		false,
		nullptr,
		0,
		*FPaths::ProjectDir(),
		nullptr);

	if (ProcHandle.IsValid())
	{
		SetStatus(LOCTEXT("PackageStarted", "已启动快速性能测试打包。请查看弹出的命令行窗口或 Build/Packages/PlayablePerfTest 输出目录。"));
	}
	else
	{
		SetStatus(LOCTEXT("PackageStartFailed", "快速性能测试打包启动失败，请手动运行脚本。"));
	}

	return FReply::Handled();
}

FReply SRuntimeGMSettingsWidget::OpenPackageScriptFolder()
{
	FPlatformProcess::ExploreFolder(*FPaths::GetPath(GetPackageBatchPath()));
	return FReply::Handled();
}

FText SRuntimeGMSettingsWidget::GetPackageScriptText() const
{
	return FText::Format(
		LOCTEXT("PackageScriptText", "快速打包脚本：{0}"),
		FText::FromString(GetPackageBatchPath()));
}

FString SRuntimeGMSettingsWidget::GetPackageBatchPath() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("PackagePlayablePerfTestWin64.bat")));
}

void SRuntimeGMSettingsWidget::SetStatus(const FText& InStatus) const
{
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(InStatus);
	}
}

#undef LOCTEXT_NAMESPACE
