#include "Tools/LevelRVT/SDevKitLevelRVTWidget.h"

#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine/World.h"
#include "IContentBrowserSingleton.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Tools/LevelRVT/DevKitLevelRVTService.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SDevKitLevelRVTWidget"

void SDevKitLevelRVTWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("InitialStatus", "就绪。选择需要写入 RVT 的地面 Actor，然后创建/绑定地面 RVT。");

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
					.Text(LOCTEXT("Title", "关卡地面 RVT 工具"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 8.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "选择地面/地形相关 Actor 后创建 RVT。工具会在 BakeInfo 下创建 Runtime Virtual Texture 资产，生成覆盖所选物体 Bounds 的 RVT Volume，把选中组件加入 Draw in Virtual Textures，并写入 EnvBatch.Source.<Level>.Ground.Batched.Ground.xx，后续关卡合批会尝试保留 Mesh Paint 顶点色。材质仍需项目地面材质支持 RVT Output/Sample。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitLevelRVTWidget::GetSelectionSummaryText)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(LOCTEXT("BakeInfoFolderLabel", "BakeInfo 目录"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(BakeInfoFolderTextBox, SEditableTextBox)
					.HintText(LOCTEXT("BakeInfoFolderHint", "/Game/Art/Map/Map_Data/LevelName/BakeInfo"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock).Text(LOCTEXT("RVTNameLabel", "RVT 资产名称"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SAssignNew(RVTNameTextBox, SEditableTextBox)
					.HintText(LOCTEXT("RVTNameHint", "RVT_L1_CommonLevel_xxx_Ground"))
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
						.Text(LOCTEXT("UseCurrentLevelPaths", "使用当前关卡标准路径"))
						.ToolTipText(LOCTEXT("UseCurrentLevelPathsTooltip", "根据当前关卡所在的 LevelAsset 文件夹推断同级 BakeInfo 目录和默认 RVT 名称。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::UseCurrentLevelPaths)
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("CreateGroundRVT", "创建/绑定地面 RVT"))
						.ToolTipText(LOCTEXT("CreateGroundRVTTooltip", "创建或复用 BakeInfo 下的 Runtime Virtual Texture，生成 RVT Volume，给选中组件写入 Draw in Virtual Textures，并给选中 Actor 写入 Ground.Batched Source Tag。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::CreateGroundRVT)
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 12.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("VisibilityNote", "说明：本工具不会隐藏源地面，也不会自动生成替代模型。源模型继续渲染并写入 RVT；是否在后续合批阶段隐藏或替换，由关卡合批流程处理。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SDevKitLevelRVTWidget::GetStatusText)
					.ColorAndOpacity(this, &SDevKitLevelRVTWidget::GetStatusColor)
					.AutoWrapText(true)
				]
			]
		]
	];

	RefreshDefaultPaths();
}

FString SDevKitLevelRVTWidget::GetCurrentWorldPackagePath() const
{
	if (!GEditor)
	{
		return FString();
	}

	const UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World || !World->GetOutermost())
	{
		return FString();
	}

	return World->GetOutermost()->GetName();
}

FText SDevKitLevelRVTWidget::GetSelectionSummaryText() const
{
	return FText::Format(
		LOCTEXT("SelectionSummary", "当前选中 Actor 数量：{0}"),
		FText::AsNumber(FDevKitLevelRVTService::GetSelectedActors().Num()));
}

FText SDevKitLevelRVTWidget::GetStatusText() const
{
	return StatusText;
}

FSlateColor SDevKitLevelRVTWidget::GetStatusColor() const
{
	return bStatusIsError
		? FSlateColor(FLinearColor(0.9f, 0.2f, 0.15f))
		: FSlateColor::UseForeground();
}

void SDevKitLevelRVTWidget::RefreshDefaultPaths()
{
	const FString WorldPackagePath = GetCurrentWorldPackagePath();
	if (WorldPackagePath.IsEmpty())
	{
		return;
	}

	if (BakeInfoFolderTextBox.IsValid())
	{
		BakeInfoFolderTextBox->SetText(FText::FromString(FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(WorldPackagePath)));
	}
	if (RVTNameTextBox.IsValid())
	{
		RVTNameTextBox->SetText(FText::FromString(FDevKitLevelRVTService::BuildDefaultGroundRVTNameFromWorldPackage(WorldPackagePath)));
	}
}

FReply SDevKitLevelRVTWidget::UseCurrentLevelPaths()
{
	RefreshDefaultPaths();
	SetStatus(LOCTEXT("UsedCurrentLevelPaths", "已根据当前关卡刷新 BakeInfo 目录和 RVT 名称。"), false);
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::CreateGroundRVT()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTRequest Request{
		BakeInfoFolderTextBox.IsValid() ? BakeInfoFolderTextBox->GetText().ToString() : FString(),
		RVTNameTextBox.IsValid() ? RVTNameTextBox->GetText().ToString() : FString()
	};

	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::CreateGroundRVTForSelection(World, Request);
	SetStatus(Result.Message, !Result.bSuccess);

	if (Result.bSuccess)
	{
		IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
		ContentBrowser.SyncBrowserToFolders({Result.Paths.BakeInfoFolder}, false, true);
	}

	return FReply::Handled();
}

void SDevKitLevelRVTWidget::SetStatus(const FText& InStatus, bool bInIsError)
{
	StatusText = InStatus;
	bStatusIsError = bInIsError;
}

#undef LOCTEXT_NAMESPACE
