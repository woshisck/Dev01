#include "Tools/LevelRVT/SDevKitLevelRVTWidget.h"

#include "ContentBrowserModule.h"
#include "Components/PrimitiveComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "IContentBrowserSingleton.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "Styling/AppStyle.h"
#include "Tools/EnvBatchSourceTagRules.h"
#include "Tools/LevelRVT/DevKitLevelRVTService.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "SDevKitLevelRVTWidget"

namespace
{
bool ActorHasTagString(const AActor* Actor, const FString& TagString)
{
	return Actor && Actor->Tags.Contains(FName(*TagString));
}

TSharedRef<SWidget> MakeIconLabel(const FName IconName, const FText& Label)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		.Padding(0.f, 0.f, 6.f, 0.f)
		[
			SNew(SBox)
			.WidthOverride(16.f)
			.HeightOverride(16.f)
			[
				SNew(SImage)
				.Image(FAppStyle::GetBrush(IconName))
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(Label)
		];
}
}

void SDevKitLevelRVTWidget::Construct(const FArguments& InArgs)
{
	StatusText = LOCTEXT("InitialStatus", "就绪。先选择地面 Tag 批次，再创建 RVT，最后按需添加或移除模型写入。");

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Title", "关卡地面 RVT"))
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 4.f, 0.f, 14.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Description", "操作对象始终是当前 Ground.Batched Tag 下的整批模型。创建资产与模型写入互相独立，可随时添加、移除或清空。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]

				// Step 1: choose or create a ground batch.
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 6.f)
				[
					BuildStepHeader(1, LOCTEXT("Step1Title", "选择地面批次"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Step1Hint", "已有批次直接点选列表。新批次先选择地面 Actor，手动填写末尾编号，再添加或替换 Tag。"))
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
				.Padding(0.f, 0.f, 0.f, 5.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("SourceTagLabel", "地面批次 Tag（末尾编号由美术手动填写，例如 .01 / .02）"))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SAssignNew(SourceTagTextBox, SEditableTextBox)
					.HintText(LOCTEXT("SourceTagHint", "EnvBatch.Source.LevelName.Ground.Batched.01"))
					.OnTextCommitted(this, &SDevKitLevelRVTWidget::OnSourceTagTextCommitted)
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
						.ContentPadding(FMargin(8.f, 3.f))
						.ToolTipText(LOCTEXT("AssignSourceTagTooltip", "把输入的 Ground.Batched Tag 写入当前场景选择。每个 Actor 只保留一个 EnvBatch.Source Tag。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::AssignSourceTagToSelection)
						[
							MakeIconLabel(TEXT("Icons.Edit"), LOCTEXT("AssignSourceTag", "添加 / 替换 Tag"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.ContentPadding(FMargin(8.f, 3.f))
						.ToolTipText(LOCTEXT("RemoveSourceTagTooltip", "只从当前所选 Actor 移除输入框中的 Tag。先点击列表行可选中并清除整批；不会清理 RVT 写入或资产。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::RemoveSourceTagFromSelection)
						[
							MakeIconLabel(TEXT("Icons.Delete"), LOCTEXT("RemoveSourceTag", "清除所选 Actor 的 Tag"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.ContentPadding(FMargin(8.f, 3.f))
						.ToolTipText(LOCTEXT("SelectSourceTagActorsTooltip", "在场景中选择当前 Tag 下的全部 Actor，便于检查。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::SelectSelectedSourceTagActors)
						[
							MakeIconLabel(TEXT("Icons.Search"), LOCTEXT("SelectSourceTagActors", "选择该批次模型"))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(8.f, 3.f))
						.OnClicked(this, &SDevKitLevelRVTWidget::RefreshSourceTags)
						[
							MakeIconLabel(TEXT("Icons.Refresh"), LOCTEXT("RefreshSourceTagList", "刷新"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 5.f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitLevelRVTWidget::GetActiveBatchSummaryText)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 5.f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitLevelRVTWidget::GetSourceTagListSummaryText)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 16.f)
				[
					SNew(SBorder)
					.Padding(6.f)
					.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
					[
						SNew(SBox)
						.HeightOverride(132.f)
						[
							SAssignNew(SourceTagListView, SListView<FSourceTagItemPtr>)
							.ListItemsSource(&SourceTagItems)
							.SelectionMode(ESelectionMode::Single)
							.OnGenerateRow(this, &SDevKitLevelRVTWidget::GenerateSourceTagRow)
							.OnSelectionChanged(this, &SDevKitLevelRVTWidget::OnSourceTagSelectionChanged)
						]
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(SSeparator)
				]

				// Step 2: create assets and volumes without changing primitive bindings.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					BuildStepHeader(2, LOCTEXT("Step2Title", "创建 RVT 资产与 Volume"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Step2Hint", "勾选本次需要的类型。Volume 会放入 _DataBake，并按当前 Tag 批次的整体 Bounds 更新；此步骤不会修改模型。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SUniformGridPanel)
					.IsEnabled(this, &SDevKitLevelRVTWidget::IsStep1Complete)
					.SlotPadding(FMargin(4.f, 2.f))
					+ SUniformGridPanel::Slot(0, 0)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::BaseColorNormalRoughness)
						.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::BaseColorNormalRoughness)
						[ SNew(STextBlock).Text(LOCTEXT("LayoutBCNR", "基础颜色 + 法线 + 粗糙度（推荐）")) ]
					]
					+ SUniformGridPanel::Slot(1, 0)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::Mask4)
						.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::Mask4)
						[ SNew(STextBlock).Text(LOCTEXT("LayoutMask4", "四通道遮罩（Mask4）")) ]
					]
					+ SUniformGridPanel::Slot(0, 1)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::BaseColor)
						.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::BaseColor)
						[ SNew(STextBlock).Text(LOCTEXT("LayoutBaseColor", "仅基础颜色")) ]
					]
					+ SUniformGridPanel::Slot(1, 1)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::BaseColorNormalSpecular)
						.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::BaseColorNormalSpecular)
						[ SNew(STextBlock).Text(LOCTEXT("LayoutBCNS", "基础颜色 + 法线 + 粗糙度 + 高光")) ]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SExpandableArea)
					.IsEnabled(this, &SDevKitLevelRVTWidget::IsStep1Complete)
					.InitiallyCollapsed(true)
					.HeaderContent()
					[
						SNew(STextBlock).Text(LOCTEXT("AdvancedLayouts", "高级类型（YCoCg / 高度 / 位移）"))
					]
					.BodyContent()
					[
						SNew(SUniformGridPanel)
						.SlotPadding(FMargin(4.f, 3.f))
						+ SUniformGridPanel::Slot(0, 0)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular)
							.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular)
							[ SNew(STextBlock).Text(LOCTEXT("LayoutYCoCg", "YCoCg 颜色 + 法线 + 粗糙度 + 高光")) ]
						]
						+ SUniformGridPanel::Slot(1, 0)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask)
							.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask)
							[ SNew(STextBlock).Text(LOCTEXT("LayoutYCoCgMask", "YCoCg 颜色 + 法线 + 粗糙度 + 高光 + 遮罩")) ]
						]
						+ SUniformGridPanel::Slot(0, 1)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::WorldHeight)
							.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::WorldHeight)
							[ SNew(STextBlock).Text(LOCTEXT("LayoutWorldHeight", "世界高度（World Height）")) ]
						]
						+ SUniformGridPanel::Slot(1, 1)
						[
							SNew(SCheckBox)
							.IsChecked(this, &SDevKitLevelRVTWidget::IsLayoutEnabled, EDevKitLevelRVTLayout::Displacement)
							.OnCheckStateChanged(this, &SDevKitLevelRVTWidget::SetLayoutEnabled, EDevKitLevelRVTLayout::Displacement)
							[ SNew(STextBlock).Text(LOCTEXT("LayoutDisplacement", "位移（Displacement）")) ]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SExpandableArea)
					.IsEnabled(this, &SDevKitLevelRVTWidget::IsStep1Complete)
					.InitiallyCollapsed(true)
					.HeaderContent()
					[
						SNew(STextBlock).Text(LOCTEXT("AdvancedPaths", "高级：资产路径与命名"))
					]
					.BodyContent()
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 4.f)
						[
							SNew(STextBlock).Text(LOCTEXT("BakeInfoFolderLabel", "BakeInfo 目录"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
						[
							SAssignNew(BakeInfoFolderTextBox, SEditableTextBox)
							.HintText(LOCTEXT("BakeInfoFolderHint", "/Game/Art/Map/Map_Data/LevelName/BakeInfo"))
							.OnTextCommitted(this, &SDevKitLevelRVTWidget::OnRVTSettingsTextCommitted)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(STextBlock).Text(LOCTEXT("RVTNameLabel", "RVT 基础名称"))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
						[
							SAssignNew(RVTNameTextBox, SEditableTextBox)
							.HintText(LOCTEXT("RVTNameHint", "RVT_L1_CommonLevel_xxx_Ground"))
							.OnTextCommitted(this, &SDevKitLevelRVTWidget::OnRVTSettingsTextCommitted)
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SButton)
							.ContentPadding(FMargin(8.f, 3.f))
							.ToolTipText(LOCTEXT("UseCurrentLevelPathsTooltip", "根据当前关卡所在的 LevelAsset 文件夹推断同级 BakeInfo 目录和默认 RVT 名称。"))
							.OnClicked(this, &SDevKitLevelRVTWidget::UseCurrentLevelPaths)
							[
								MakeIconLabel(TEXT("Icons.FolderOpen"), LOCTEXT("UseCurrentLevelPaths", "恢复当前关卡标准路径"))
							]
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 16.f)
				[
					SNew(SButton)
					.IsEnabled(this, &SDevKitLevelRVTWidget::IsStep1Complete)
					.ContentPadding(FMargin(10.f, 5.f))
					.ToolTipText(LOCTEXT("CreateGroundRVTTooltip", "创建或复用勾选类型的 RVT 资产，并在 _DataBake 中创建/更新对应 Volume。不会修改模型的 Draw in Virtual Textures。"))
					.OnClicked(this, &SDevKitLevelRVTWidget::CreateGroundRVTAssetsAndVolumes)
					[
						MakeIconLabel(TEXT("Icons.PlusCircle"), LOCTEXT("CreateGroundRVT", "创建 / 更新所选 RVT 资产与 Volume"))
					]
				]

				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(SSeparator)
				]

				// Step 3: explicitly control component RuntimeVirtualTextures arrays.
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					BuildStepHeader(3, LOCTEXT("Step3Title", "模型 RVT 写入控制"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitLevelRVTWidget::GetSelectedLayoutSummaryText)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("Step3Hint", "添加和移除都作用于当前 Tag 的全部独立模型。移除最后一个 RVT 后会自动恢复 Main Pass；清空不会删除 RVT 资产或 Volume。"))
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 14.f)
				[
					SNew(SHorizontalBox)
					.IsEnabled(this, &SDevKitLevelRVTWidget::IsStep2Complete)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.IsEnabled(this, &SDevKitLevelRVTWidget::CanAddSelectedRVT)
						.ContentPadding(FMargin(10.f, 5.f))
						.ToolTipText(LOCTEXT("AddGroundRVTBindingsTooltip", "把步骤 2 中勾选且已创建的 RVT 添加到该 Tag 全部组件的 Draw in Virtual Textures。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::AddGroundRVTBindings)
						[
							MakeIconLabel(TEXT("Icons.PlusCircle"), LOCTEXT("AddGroundRVTBindings", "添加所选 RVT"))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
					[
						SNew(SButton)
						.ContentPadding(FMargin(10.f, 5.f))
						.ToolTipText(LOCTEXT("RemoveGroundRVTBindingsTooltip", "只移除当前勾选类型，其他 RVT 引用保持不变。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::RemoveGroundRVTBindings)
						[
							MakeIconLabel(TEXT("Icons.Delete"), LOCTEXT("RemoveGroundRVTBindings", "移除所选 RVT"))
						]
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.ContentPadding(FMargin(10.f, 5.f))
						.ToolTipText(LOCTEXT("ClearGroundRVTBindingsTooltip", "清空该 Tag 全部组件的 Draw in Virtual Textures，并恢复 Draw in Main Pass = Always。"))
						.OnClicked(this, &SDevKitLevelRVTWidget::ClearGroundRVTBindings)
						[
							MakeIconLabel(TEXT("Icons.Delete"), LOCTEXT("ClearGroundRVTBindings", "清空全部 RVT 写入"))
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.f, 0.f, 0.f, 8.f)
				[
					SNew(SSeparator)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SBorder)
					.Padding(FMargin(8.f, 6.f))
					.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
					[
						SNew(STextBlock)
						.Text(this, &SDevKitLevelRVTWidget::GetStatusText)
						.ColorAndOpacity(this, &SDevKitLevelRVTWidget::GetStatusColor)
						.AutoWrapText(true)
					]
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

FString SDevKitLevelRVTWidget::GetCurrentSourceTag() const
{
	FString SourceTag = SourceTagTextBox.IsValid() ? SourceTagTextBox->GetText().ToString() : FString();
	SourceTag.TrimStartAndEndInline();
	return SourceTag;
}

FDevKitLevelRVTRequest SDevKitLevelRVTWidget::BuildRequest() const
{
	FDevKitLevelRVTRequest Request;
	Request.BakeInfoFolder = BakeInfoFolderTextBox.IsValid() ? BakeInfoFolderTextBox->GetText().ToString() : FString();
	Request.RuntimeVirtualTextureName = RVTNameTextBox.IsValid() ? RVTNameTextBox->GetText().ToString() : FString();
	Request.GroundBatchSourceTag = GetCurrentSourceTag();
	Request.Layouts = GetSelectedLayouts();
	Request.BakeInfoFolder.TrimStartAndEndInline();
	Request.RuntimeVirtualTextureName.TrimStartAndEndInline();
	return Request;
}

TSharedRef<SWidget> SDevKitLevelRVTWidget::BuildStepHeader(int32 StepNumber, const FText& Title)
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("DetailsView.CategoryTop")))
		.BorderBackgroundColor(this, &SDevKitLevelRVTWidget::GetStepHeaderTint, StepNumber)
		.Padding(FMargin(10.f, 7.f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 12.f, 0.f)
			[
				SNew(STextBlock)
				.Text(FText::Format(LOCTEXT("StepNumber", "步骤 {0}"), FText::AsNumber(StepNumber)))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(Title)
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(12.f, 0.f, 0.f, 0.f)
			[
				SNew(STextBlock)
				.Text(this, &SDevKitLevelRVTWidget::GetStepStatusText, StepNumber)
				.ColorAndOpacity(this, &SDevKitLevelRVTWidget::GetStepStatusColor, StepNumber)
			]
		];
}

bool SDevKitLevelRVTWidget::IsStep1Complete() const
{
	FEnvBatchSourceTagSpec Spec;
	const bool bValidTag = ParseEnvBatchSourceTag(GetCurrentSourceTag(), Spec)
		&& !Spec.bHasExplicitVTCGroup
		&& Spec.ActorKind == TEXT("Ground")
		&& Spec.ProcessingMode == TEXT("Batched");
	return bValidTag
		&& ActiveBatchStats.ActorCount > 0
		&& ActiveBatchStats.PrimitiveComponentCount > 0;
}

bool SDevKitLevelRVTWidget::IsStep2Complete() const
{
	return IsStep1Complete() && ActiveRVTAssetState.IsReady();
}

bool SDevKitLevelRVTWidget::CanAddSelectedRVT() const
{
	return IsStep2Complete();
}

FText SDevKitLevelRVTWidget::GetStepStatusText(int32 StepNumber) const
{
	if (StepNumber == 1)
	{
		return IsStep1Complete()
			? FText::Format(
				LOCTEXT("Step1Complete", "已完成  |  {0} Actor / {1} 组件"),
				FText::AsNumber(ActiveBatchStats.ActorCount),
				FText::AsNumber(ActiveBatchStats.PrimitiveComponentCount))
			: LOCTEXT("Step1Incomplete", "未完成  |  请选择或写入有效 Tag");
	}

	if (!IsStep1Complete())
	{
		return LOCTEXT("LaterStepLocked", "已锁定  |  请先完成步骤 1");
	}

	if (StepNumber == 2)
	{
		if (ActiveRVTAssetState.RequestedLayoutCount == 0)
		{
			return LOCTEXT("Step2NoLayouts", "未完成  |  请勾选 RVT 类型");
		}
		if (ActiveRVTAssetState.IsReady())
		{
			return FText::Format(
				LOCTEXT("Step2Complete", "已完成  |  {0} 个 RVT 资产与 Volume"),
				FText::AsNumber(ActiveRVTAssetState.RequestedLayoutCount));
		}
		if (!ActiveRVTAssetState.bDataBakeLevelLoaded)
		{
			return LOCTEXT("Step2DataBakeMissing", "未完成  |  _DataBake 子关卡未加载");
		}
		return FText::Format(
			LOCTEXT("Step2Pending", "待创建  |  资产 {0}/{1}，Volume {2}/{1}"),
			FText::AsNumber(ActiveRVTAssetState.ExistingAssetCount),
			FText::AsNumber(ActiveRVTAssetState.RequestedLayoutCount),
			FText::AsNumber(ActiveRVTAssetState.ExistingVolumeCount));
	}

	return IsStep2Complete()
		? LOCTEXT("Step3Ready", "已解锁  |  可添加、移除或清空")
		: LOCTEXT("Step3LockedByStep2", "已锁定  |  请先完成步骤 2");
}

FSlateColor SDevKitLevelRVTWidget::GetStepStatusColor(int32 StepNumber) const
{
	const bool bComplete = StepNumber == 1 ? IsStep1Complete() : IsStep2Complete();
	if (bComplete)
	{
		return FSlateColor(FLinearColor(0.30f, 0.85f, 0.48f));
	}
	return IsStep1Complete()
		? FSlateColor(FLinearColor(0.95f, 0.72f, 0.22f))
		: FSlateColor::UseSubduedForeground();
}

FSlateColor SDevKitLevelRVTWidget::GetStepHeaderTint(int32 StepNumber) const
{
	if (StepNumber == 1)
	{
		return IsStep1Complete()
			? FSlateColor(FLinearColor(0.07f, 0.28f, 0.16f, 0.95f))
			: FSlateColor(FLinearColor(0.30f, 0.19f, 0.05f, 0.95f));
	}
	if (!IsStep1Complete())
	{
		return FSlateColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.95f));
	}
	if (StepNumber == 2 && IsStep2Complete())
	{
		return FSlateColor(FLinearColor(0.07f, 0.28f, 0.16f, 0.95f));
	}
	if (StepNumber == 3 && !IsStep2Complete())
	{
		return FSlateColor(FLinearColor(0.08f, 0.08f, 0.08f, 0.95f));
	}
	return FSlateColor(FLinearColor(0.05f, 0.18f, 0.30f, 0.95f));
}

FString SDevKitLevelRVTWidget::GetDefaultGroundSourceTag() const
{
	const FString BakeInfoFolder = BakeInfoFolderTextBox.IsValid()
		? BakeInfoFolderTextBox->GetText().ToString()
		: FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(GetCurrentWorldPackagePath());

	FString NormalizedBakeInfoFolder = BakeInfoFolder;
	NormalizedBakeInfoFolder.TrimStartAndEndInline();
	while (NormalizedBakeInfoFolder.EndsWith(TEXT("/")))
	{
		NormalizedBakeInfoFolder.LeftChopInline(1);
	}

	const FString LevelFolder = FPackageName::GetLongPackagePath(NormalizedBakeInfoFolder);
	const FString LevelName = SanitizeEnvBatchTagToken(FPackageName::GetLongPackageAssetName(LevelFolder), TEXT("Level"));

	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = LevelName;
	Spec.ActorKind = TEXT("Ground");
	Spec.ProcessingMode = TEXT("Batched");
	Spec.bHasExplicitVTCGroup = false;
	Spec.SerialNumber = 1;
	return BuildEnvBatchSourceTag(Spec);
}

TArray<EDevKitLevelRVTLayout> SDevKitLevelRVTWidget::GetSelectedLayouts() const
{
	const EDevKitLevelRVTLayout AllLayouts[] =
	{
		EDevKitLevelRVTLayout::BaseColorNormalRoughness,
		EDevKitLevelRVTLayout::Mask4,
		EDevKitLevelRVTLayout::BaseColor,
		EDevKitLevelRVTLayout::BaseColorNormalSpecular,
		EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular,
		EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask,
		EDevKitLevelRVTLayout::WorldHeight,
		EDevKitLevelRVTLayout::Displacement
	};

	TArray<EDevKitLevelRVTLayout> Layouts;
	for (const EDevKitLevelRVTLayout Layout : AllLayouts)
	{
		if (IsLayoutEnabled(Layout) == ECheckBoxState::Checked)
		{
			Layouts.Add(Layout);
		}
	}
	return Layouts;
}

void SDevKitLevelRVTWidget::SetLayoutEnabled(ECheckBoxState NewState, EDevKitLevelRVTLayout Layout)
{
	const bool bEnabled = NewState == ECheckBoxState::Checked;
	switch (Layout)
	{
	case EDevKitLevelRVTLayout::Mask4: bMask4 = bEnabled; break;
	case EDevKitLevelRVTLayout::BaseColor: bBaseColor = bEnabled; break;
	case EDevKitLevelRVTLayout::BaseColorNormalSpecular: bBaseColorNormalSpecular = bEnabled; break;
	case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular: bYCoCgBaseColorNormalSpecular = bEnabled; break;
	case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask: bYCoCgBaseColorNormalSpecularMask = bEnabled; break;
	case EDevKitLevelRVTLayout::WorldHeight: bWorldHeight = bEnabled; break;
	case EDevKitLevelRVTLayout::Displacement: bDisplacement = bEnabled; break;
	case EDevKitLevelRVTLayout::BaseColorNormalRoughness: bBaseColorNormalRoughness = bEnabled; break;
	}
	RefreshRVTAssetState();
}

ECheckBoxState SDevKitLevelRVTWidget::IsLayoutEnabled(EDevKitLevelRVTLayout Layout) const
{
	bool bEnabled = false;
	switch (Layout)
	{
	case EDevKitLevelRVTLayout::Mask4: bEnabled = bMask4; break;
	case EDevKitLevelRVTLayout::BaseColor: bEnabled = bBaseColor; break;
	case EDevKitLevelRVTLayout::BaseColorNormalSpecular: bEnabled = bBaseColorNormalSpecular; break;
	case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecular: bEnabled = bYCoCgBaseColorNormalSpecular; break;
	case EDevKitLevelRVTLayout::YCoCgBaseColorNormalSpecularMask: bEnabled = bYCoCgBaseColorNormalSpecularMask; break;
	case EDevKitLevelRVTLayout::WorldHeight: bEnabled = bWorldHeight; break;
	case EDevKitLevelRVTLayout::Displacement: bEnabled = bDisplacement; break;
	case EDevKitLevelRVTLayout::BaseColorNormalRoughness: bEnabled = bBaseColorNormalRoughness; break;
	}
	return bEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

FText SDevKitLevelRVTWidget::GetSelectionSummaryText() const
{
	return FText::Format(
		LOCTEXT("SelectionSummary", "当前选中 Actor 数量：{0}"),
		FText::AsNumber(FDevKitLevelRVTService::GetSelectedActors().Num()));
}

FText SDevKitLevelRVTWidget::GetSourceTagListSummaryText() const
{
	return FText::Format(
		LOCTEXT("SourceTagListSummary", "当前已加载场景：{0} 个 Actor，{1} 个地面批次。点击列表行可切换当前批次。"),
		FText::AsNumber(LastScannedActorCount),
		FText::AsNumber(SourceTagItems.Num()));
}

FText SDevKitLevelRVTWidget::GetActiveBatchSummaryText() const
{
	const FString SourceTag = GetCurrentSourceTag();
	if (SourceTag.IsEmpty())
	{
		return LOCTEXT("NoActiveBatch", "当前批次：未选择");
	}

	return FText::Format(
		LOCTEXT("ActiveBatchSummary", "当前批次：{0}  |  {1} Actor / {2} 组件  |  已写入 {3} 组件，共 {4} 个 RVT 引用"),
		FText::FromString(SourceTag),
		FText::AsNumber(ActiveBatchStats.ActorCount),
		FText::AsNumber(ActiveBatchStats.PrimitiveComponentCount),
		FText::AsNumber(ActiveBatchStats.BoundComponentCount),
		FText::AsNumber(ActiveBatchStats.RuntimeVirtualTextureReferenceCount));
}

FText SDevKitLevelRVTWidget::GetSelectedLayoutSummaryText() const
{
	const TArray<EDevKitLevelRVTLayout> Layouts = GetSelectedLayouts();
	if (Layouts.IsEmpty())
	{
		return LOCTEXT("NoSelectedLayoutsSummary", "本次操作类型：未勾选");
	}

	TArray<FString> LayoutNames;
	for (const EDevKitLevelRVTLayout Layout : Layouts)
	{
		LayoutNames.Add(FDevKitLevelRVTService::GetLayoutDisplayName(Layout));
	}
	return FText::Format(
		LOCTEXT("SelectedLayoutsSummary", "本次操作类型（{0}）：{1}"),
		FText::AsNumber(Layouts.Num()),
		FText::FromString(FString::Join(LayoutNames, TEXT("、"))));
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
	const FString ExistingSourceTag = GetCurrentSourceTag();
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
	RefreshSourceTagList();
	if (SourceTagTextBox.IsValid())
	{
		if (!ExistingSourceTag.IsEmpty())
		{
			SourceTagTextBox->SetText(FText::FromString(ExistingSourceTag));
		}
		else if (!SourceTagItems.IsEmpty())
		{
			SelectedSourceTagItem = SourceTagItems[0];
			SourceTagTextBox->SetText(FText::FromString(SelectedSourceTagItem->Tag));
			if (SourceTagListView.IsValid())
			{
				SourceTagListView->SetSelection(SelectedSourceTagItem, ESelectInfo::Direct);
			}
		}
		else
		{
			SourceTagTextBox->SetText(FText::FromString(GetDefaultGroundSourceTag()));
		}
	}
	RefreshActiveBatchStats();
}

void SDevKitLevelRVTWidget::RefreshSourceTagList()
{
	const FString PreferredSourceTag = GetCurrentSourceTag();
	SourceTagItems.Reset();
	SelectedSourceTagItem.Reset();
	LastScannedActorCount = 0;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		if (SourceTagListView.IsValid())
		{
			SourceTagListView->RequestListRefresh();
		}
		ActiveBatchStats = FDevKitLevelRVTBatchStats();
		ActiveRVTAssetState = FDevKitLevelRVTAssetState();
		return;
	}

	TMap<FString, FDevKitLevelRVTSourceTagListItem> TagStats;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		++LastScannedActorCount;

		for (const FName& ActorTag : Actor->Tags)
		{
			FEnvBatchSourceTagSpec Spec;
			const FString TagString = ActorTag.ToString();
			if (ParseEnvBatchSourceTag(TagString, Spec)
				&& !Spec.bHasExplicitVTCGroup
				&& Spec.ActorKind == TEXT("Ground")
				&& Spec.ProcessingMode == TEXT("Batched"))
			{
				FDevKitLevelRVTSourceTagListItem& Stats = TagStats.FindOrAdd(TagString);
				Stats.Tag = TagString;
				++Stats.ActorCount;

				TInlineComponentArray<UPrimitiveComponent*> Components;
				Actor->GetComponents(Components);
				for (const UPrimitiveComponent* Component : Components)
				{
					if (!Component || !Component->IsRegistered())
					{
						continue;
					}
					++Stats.PrimitiveComponentCount;
					const int32 BindingCount = Component->RuntimeVirtualTextures.Num();
					if (BindingCount > 0)
					{
						++Stats.BoundComponentCount;
						Stats.RuntimeVirtualTextureReferenceCount += BindingCount;
					}
				}
			}
		}
	}

	TagStats.KeySort([](const FString& A, const FString& B)
	{
		return A < B;
	});

	for (const TPair<FString, FDevKitLevelRVTSourceTagListItem>& Entry : TagStats)
	{
		FSourceTagItemPtr Item = MakeShared<FDevKitLevelRVTSourceTagListItem>(Entry.Value);
		SourceTagItems.Add(Item);
		if (Item->Tag == PreferredSourceTag)
		{
			SelectedSourceTagItem = Item;
		}
	}

	if (SourceTagListView.IsValid())
	{
		SourceTagListView->RequestListRefresh();
		if (SelectedSourceTagItem.IsValid())
		{
			SourceTagListView->SetSelection(SelectedSourceTagItem, ESelectInfo::Direct);
		}
	}
	RefreshActiveBatchStats();
}

void SDevKitLevelRVTWidget::RefreshActiveBatchStats()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	ActiveBatchStats = FDevKitLevelRVTService::GetGroundBatchStats(World, GetCurrentSourceTag());
	RefreshRVTAssetState();
}

void SDevKitLevelRVTWidget::RefreshRVTAssetState()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	ActiveRVTAssetState = FDevKitLevelRVTService::GetRVTAssetState(World, BuildRequest());
}

void SDevKitLevelRVTWidget::SelectActorsWithSourceTag(const FString& SourceTag)
{
	if (!GEditor)
	{
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return;
	}

	GEditor->SelectNone(false, true, false);

	int32 SelectedCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (ActorHasTagString(Actor, SourceTag))
		{
			GEditor->SelectActor(Actor, true, false);
			++SelectedCount;
		}
	}

	GEditor->NoteSelectionChange();
	SetStatus(FText::Format(
		LOCTEXT("SelectedByTag", "已选中 {0} 个带有 {1} 的 Actor。"),
		FText::AsNumber(SelectedCount),
		FText::FromString(SourceTag)), false);
}

FReply SDevKitLevelRVTWidget::UseCurrentLevelPaths()
{
	RefreshDefaultPaths();
	SetStatus(LOCTEXT("UsedCurrentLevelPaths", "已根据当前关卡刷新 BakeInfo 目录和 RVT 名称。"), false);
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::RefreshSourceTags()
{
	RefreshSourceTagList();
	SetStatus(LOCTEXT("RefreshedSourceTags", "已刷新 Ground.Batched Source Tag 列表。"), false);
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::AssignSourceTagToSelection()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::AssignGroundBatchSourceTagToSelection(World, GetCurrentSourceTag());
	SetStatus(Result.Message, !Result.bSuccess);
	if (Result.bSuccess)
	{
		RefreshSourceTagList();
	}
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::RemoveSourceTagFromSelection()
{
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::RemoveGroundBatchSourceTagFromSelection(World, GetCurrentSourceTag());
	SetStatus(Result.Message, !Result.bSuccess);
	RefreshSourceTagList();
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::SelectSelectedSourceTagActors()
{
	const FString SourceTag = GetCurrentSourceTag();
	if (SourceTag.IsEmpty())
	{
		SetStatus(LOCTEXT("NoSourceTagSelected", "请先选择或输入 Ground.Batched Source Tag。"), true);
		return FReply::Handled();
	}

	SelectActorsWithSourceTag(SourceTag);
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::CreateGroundRVTAssetsAndVolumes()
{
	const FDevKitLevelRVTRequest Request = BuildRequest();
	if (Request.Layouts.IsEmpty())
	{
		SetStatus(LOCTEXT("NoRVTLayout", "请至少勾选一种 RVT 类型。"), true);
		return FReply::Handled();
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::CreateGroundRVTAssetsAndVolumes(World, Request);
	SetStatus(Result.Message, !Result.bSuccess);

	if (Result.bSuccess)
	{
		IContentBrowserSingleton& ContentBrowser = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
		ContentBrowser.SyncBrowserToFolders({Result.Paths.BakeInfoFolder}, false, true);
		RefreshSourceTagList();
	}

	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::AddGroundRVTBindings()
{
	const FDevKitLevelRVTRequest Request = BuildRequest();
	if (Request.Layouts.IsEmpty())
	{
		SetStatus(LOCTEXT("NoRVTLayoutAdd", "请至少勾选一种要添加的 RVT 类型。"), true);
		return FReply::Handled();
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::AddGroundRVTBindings(World, Request);
	SetStatus(Result.Message, !Result.bSuccess);
	RefreshSourceTagList();
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::RemoveGroundRVTBindings()
{
	const FDevKitLevelRVTRequest Request = BuildRequest();
	if (Request.Layouts.IsEmpty())
	{
		SetStatus(LOCTEXT("NoRVTLayoutRemove", "请至少勾选一种要移除的 RVT 类型。"), true);
		return FReply::Handled();
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::RemoveGroundRVTBindings(World, Request);
	SetStatus(Result.Message, !Result.bSuccess);
	RefreshSourceTagList();
	return FReply::Handled();
}

FReply SDevKitLevelRVTWidget::ClearGroundRVTBindings()
{
	const FString SourceTag = GetCurrentSourceTag();
	if (SourceTag.IsEmpty())
	{
		SetStatus(LOCTEXT("NoSourceTagClear", "请先选择要清理的 Ground.Batched Source Tag。"), true);
		return FReply::Handled();
	}

	const FText Confirmation = FText::Format(
		LOCTEXT("ConfirmClearBindings", "确定清空批次 {0} 下全部模型的 RVT 写入吗？\n\nRVT 资产和 DataBake Volume 会保留，Draw in Main Pass 将恢复为 Always。"),
		FText::FromString(SourceTag));
	if (FMessageDialog::Open(EAppMsgType::YesNo, Confirmation) != EAppReturnType::Yes)
	{
		return FReply::Handled();
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	const FDevKitLevelRVTResult Result = FDevKitLevelRVTService::ClearGroundRVTBindings(World, BuildRequest());
	SetStatus(Result.Message, !Result.bSuccess);
	RefreshSourceTagList();
	return FReply::Handled();
}

TSharedRef<ITableRow> SDevKitLevelRVTWidget::GenerateSourceTagRow(FSourceTagItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable) const
{
	return SNew(STableRow<FSourceTagItemPtr>, OwnerTable)
	.Padding(4.f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		[
			SNew(STextBlock)
			.Text(Item.IsValid() ? FText::FromString(Item->Tag) : LOCTEXT("InvalidSourceTagRow", "<invalid>"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(12.f, 0.f, 0.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Item.IsValid()
				? FText::Format(
					LOCTEXT("SourceTagStats", "{0} Actor  |  {1}/{2} 组件已写入  |  {3} 引用"),
					FText::AsNumber(Item->ActorCount),
					FText::AsNumber(Item->BoundComponentCount),
					FText::AsNumber(Item->PrimitiveComponentCount),
					FText::AsNumber(Item->RuntimeVirtualTextureReferenceCount))
				: FText::GetEmpty())
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
	];
}

void SDevKitLevelRVTWidget::OnSourceTagSelectionChanged(FSourceTagItemPtr Item, ESelectInfo::Type SelectInfo)
{
	SelectedSourceTagItem = Item;
	if (!Item.IsValid())
	{
		return;
	}

	if (SourceTagTextBox.IsValid())
	{
		SourceTagTextBox->SetText(FText::FromString(Item->Tag));
	}
	ActiveBatchStats.ActorCount = Item->ActorCount;
	ActiveBatchStats.PrimitiveComponentCount = Item->PrimitiveComponentCount;
	ActiveBatchStats.BoundComponentCount = Item->BoundComponentCount;
	ActiveBatchStats.RuntimeVirtualTextureReferenceCount = Item->RuntimeVirtualTextureReferenceCount;
	RefreshRVTAssetState();

	if (SelectInfo != ESelectInfo::Direct)
	{
		SelectActorsWithSourceTag(Item->Tag);
	}
}

void SDevKitLevelRVTWidget::OnSourceTagTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	RefreshSourceTagList();
}

void SDevKitLevelRVTWidget::OnRVTSettingsTextCommitted(const FText& Text, ETextCommit::Type CommitType)
{
	RefreshRVTAssetState();
}

void SDevKitLevelRVTWidget::SetStatus(const FText& InStatus, bool bInIsError)
{
	StatusText = InStatus;
	bStatusIsError = bInIsError;
}

#undef LOCTEXT_NAMESPACE
