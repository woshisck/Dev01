#include "Tools/SModelAssetComplianceWidget.h"
#include "Tools/DevKitArtToolUI.h"

#include "AdvancedPreviewScene.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetThumbnail.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Modules/ModuleManager.h"
#include "PhysicsEngine/BodySetup.h"
#include "SEditorViewport.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

#include <initializer_list>

#define LOCTEXT_NAMESPACE "SModelAssetComplianceWidget"

namespace
{
const TCHAR* ArtRootPath = TEXT("/Game/Art");

struct FCategoryFilterDefinition
{
	EModelAssetComplianceCategory Category;
	const TCHAR* Label;
};

const TArray<FCategoryFilterDefinition>& GetCategoryFilterDefinitions()
{
	static const TArray<FCategoryFilterDefinition> Definitions = {
		{ EModelAssetComplianceCategory::Interactive, TEXT("交互物") },
		{ EModelAssetComplianceCategory::Character, TEXT("角色") },
		{ EModelAssetComplianceCategory::Weapon, TEXT("武器") },
		{ EModelAssetComplianceCategory::Prop, TEXT("道具") },
		{ EModelAssetComplianceCategory::SceneProp, TEXT("场景模型-物件") },
		{ EModelAssetComplianceCategory::SceneBuilding, TEXT("场景模型-建筑") },
		{ EModelAssetComplianceCategory::SceneGround, TEXT("场景模型-地面") },
		{ EModelAssetComplianceCategory::SceneOther, TEXT("场景模型-其他") },
		{ EModelAssetComplianceCategory::Decal, TEXT("贴花模型") },
		{ EModelAssetComplianceCategory::OtherAsset, TEXT("其他资产") },
	};
	return Definitions;
}

bool PathContainsAny(const FString& LowerPath, std::initializer_list<const TCHAR*> Keywords)
{
	for (const TCHAR* Keyword : Keywords)
	{
		if (LowerPath.Contains(Keyword))
		{
			return true;
		}
	}
	return false;
}

FString EscapeRichText(const FString& Value)
{
	FString Result = Value;
	Result.ReplaceInline(TEXT("&"), TEXT("&amp;"));
	Result.ReplaceInline(TEXT("<"), TEXT("&lt;"));
	Result.ReplaceInline(TEXT(">"), TEXT("&gt;"));
	return Result;
}

const TCHAR* RichTagForStatus(EModelAssetComplianceStatus Status)
{
	switch (Status)
	{
	case EModelAssetComplianceStatus::Pass:
		return TEXT("ok");
	case EModelAssetComplianceStatus::Warning:
		return TEXT("warn");
	case EModelAssetComplianceStatus::Blocked:
		return TEXT("bad");
	case EModelAssetComplianceStatus::Excluded:
		return TEXT("muted");
	default:
		return TEXT("text");
	}
}

const FSlateStyleSet& GetComplianceRichTextStyle()
{
	static TSharedPtr<FSlateStyleSet> StyleSet;
	if (!StyleSet.IsValid())
	{
		StyleSet = MakeShared<FSlateStyleSet>(TEXT("ModelAssetComplianceRichTextStyle"));

		const FTextBlockStyle BaseStyle = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText"));

		FTextBlockStyle DefaultStyle(BaseStyle);
		DefaultStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.86f, 0.86f)));
		StyleSet->Set(TEXT("Default"), DefaultStyle);
		StyleSet->Set(TEXT("text"), DefaultStyle);

		FTextBlockStyle LabelStyle(DefaultStyle);
		LabelStyle.SetFont(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")));
		LabelStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.82f, 1.00f)));
		StyleSet->Set(TEXT("label"), LabelStyle);

		FTextBlockStyle OkStyle(DefaultStyle);
		OkStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.20f, 0.78f, 0.34f)));
		StyleSet->Set(TEXT("ok"), OkStyle);

		FTextBlockStyle WarnStyle(DefaultStyle);
		WarnStyle.SetColorAndOpacity(FSlateColor(FLinearColor(1.00f, 0.68f, 0.22f)));
		StyleSet->Set(TEXT("warn"), WarnStyle);

		FTextBlockStyle BadStyle(DefaultStyle);
		BadStyle.SetColorAndOpacity(FSlateColor(FLinearColor(1.00f, 0.28f, 0.20f)));
		StyleSet->Set(TEXT("bad"), BadStyle);

		FTextBlockStyle InfoStyle(DefaultStyle);
		InfoStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.42f, 0.72f, 1.00f)));
		StyleSet->Set(TEXT("info"), InfoStyle);

		FTextBlockStyle MutedStyle(DefaultStyle);
		MutedStyle.SetColorAndOpacity(FSlateColor(FLinearColor(0.58f, 0.58f, 0.58f)));
		StyleSet->Set(TEXT("muted"), MutedStyle);
	}
	return *StyleSet;
}

TSharedRef<SWidget> MakeRichTextBlock(TAttribute<FText> Text)
{
	const FSlateStyleSet& RichTextStyle = GetComplianceRichTextStyle();
	return SNew(SRichTextBlock)
		.Text(Text)
		.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
		.DecoratorStyleSet(&RichTextStyle)
		.AutoWrapText(true);
}

FString BuildMessageRichText(const FModelAssetComplianceItem* Item)
{
	if (!Item)
	{
		return TEXT("<muted>未选择模型。</>");
	}

	TArray<FString> Lines;
	for (const FText& Message : Item->Blockers)
	{
		Lines.Add(FString::Printf(TEXT("<bad>[阻断]</> %s"), *EscapeRichText(Message.ToString())));
	}
	for (const FText& Message : Item->Warnings)
	{
		Lines.Add(FString::Printf(TEXT("<warn>[警告]</> %s"), *EscapeRichText(Message.ToString())));
	}
	for (const FText& Message : Item->Infos)
	{
		Lines.Add(FString::Printf(TEXT("<info>[说明]</> %s"), *EscapeRichText(Message.ToString())));
	}

	if (Lines.IsEmpty())
	{
		return TEXT("<ok>当前模型没有阻断项或警告项。</>");
	}
	return FString::Join(Lines, TEXT("\n"));
}

void SortTreeNodes(TArray<TSharedPtr<FModelAssetComplianceTreeNode>>& Nodes)
{
	Nodes.Sort([](const TSharedPtr<FModelAssetComplianceTreeNode>& Left, const TSharedPtr<FModelAssetComplianceTreeNode>& Right)
	{
		if (!Left.IsValid() || !Right.IsValid())
		{
			return Left.IsValid();
		}
		if (Left->bIsFolder != Right->bIsFolder)
		{
			return Left->bIsFolder;
		}
		return Left->DisplayName.Compare(Right->DisplayName, ESearchCase::IgnoreCase) < 0;
	});

	for (const TSharedPtr<FModelAssetComplianceTreeNode>& Node : Nodes)
	{
		if (Node.IsValid() && Node->bIsFolder)
		{
			SortTreeNodes(Node->Children);
		}
	}
}
}

class SModelAssetComplianceViewport : public SEditorViewport
{
public:
	SLATE_BEGIN_ARGS(SModelAssetComplianceViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		PreviewScene = MakeShared<FAdvancedPreviewScene>(FPreviewScene::ConstructionValues());
		PreviewScene->SetFloorVisibility(true);

		PreviewMeshComponent = TStrongObjectPtr<UStaticMeshComponent>(NewObject<UStaticMeshComponent>(GetTransientPackage()));
		PreviewMeshComponent->SetMobility(EComponentMobility::Movable);
		PreviewMeshComponent->SetVisibility(true);
		PreviewScene->AddComponent(PreviewMeshComponent.Get(), FTransform::Identity);

		SEditorViewport::Construct(SEditorViewport::FArguments());
	}

	void SetStaticMesh(UStaticMesh* StaticMesh)
	{
		if (!PreviewMeshComponent.IsValid())
		{
			return;
		}

		PreviewMeshComponent->SetStaticMesh(StaticMesh);
		PreviewMeshComponent->UpdateBounds();
		PreviewMeshComponent->MarkRenderStateDirty();

		if (StaticMesh)
		{
			FocusStaticMesh(StaticMesh);
		}
		Invalidate();
	}

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override
	{
		ViewportClient = MakeShared<FEditorViewportClient>(nullptr, PreviewScene.Get(), SharedThis(this));
		ViewportClient->ViewportType = LVT_Perspective;
		ViewportClient->SetViewMode(VMI_Lit);
		ViewportClient->SetRealtime(true);
		ViewportClient->EngineShowFlags.SetSelectionOutline(false);
		ViewportClient->EngineShowFlags.SetCompositeEditorPrimitives(true);
		return ViewportClient.ToSharedRef();
	}

private:
	void FocusStaticMesh(const UStaticMesh* StaticMesh)
	{
		if (!ViewportClient.IsValid() || !StaticMesh)
		{
			return;
		}

		const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
		const float Radius = FMath::Max(25.f, Bounds.SphereRadius);
		const FVector Target = Bounds.Origin;
		const FVector ViewLocation = Target + FVector(-Radius * 2.4f, Radius * 1.6f, Radius * 1.1f);
		ViewportClient->SetLookAtLocation(Target);
		ViewportClient->SetViewLocation(ViewLocation);
		ViewportClient->SetViewRotation((Target - ViewLocation).Rotation());
		ViewportClient->Invalidate();
	}

	TSharedPtr<FAdvancedPreviewScene> PreviewScene;
	TSharedPtr<FEditorViewportClient> ViewportClient;
	TStrongObjectPtr<UStaticMeshComponent> PreviewMeshComponent;
};

void SModelAssetComplianceWidget::Construct(const FArguments& InArgs)
{
	for (const FCategoryFilterDefinition& Definition : GetCategoryFilterDefinitions())
	{
		EnabledDirectoryCategoryFilters.Add(Definition.Category);
	}

	ThumbnailPool = MakeShared<FAssetThumbnailPool>(128);

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(12.f, 12.f, 12.f, 6.f)
		[
			BuildToolbar()
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		.Padding(12.f, 0.f, 12.f, 12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
			[
				DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("ReviewSection", "筛选并检查模型"), LOCTEXT("ReviewSectionDesc", "左侧选择资产，中间查看预览与问题，右侧核对规则和资产设置。"))
			]
			+ SVerticalBox::Slot().FillHeight(1.f)
			[
				SNew(SSplitter)
				+ SSplitter::Slot()
				.Value(0.30f)
				[
					BuildAssetListPanel()
				]
				+ SSplitter::Slot()
				.Value(0.42f)
				[
					BuildPreviewPanel()
				]
				+ SSplitter::Slot()
				.Value(0.28f)
				[
					BuildSettingsPanel()
				]
			]
		]
	];

	ScanAssets();
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildToolbar()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DevKitArtToolUI::MakeHeader(
				LOCTEXT("Title", "模型资产合规检查"),
				LOCTEXT("Description", "扫描 /Game/Art 下 StaticMesh，检查 LOD、材质槽、碰撞和环境分类。此工具只做检查，不执行合批、代理、替换或资产写入。"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 8.f, 0.f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("Refresh", "刷新 /Game/Art"))
				.ToolTipText(LOCTEXT("RefreshTooltip", "重新扫描 /Game/Art 下的 StaticMesh 资产并刷新合规结果。"))
				.OnClicked(this, &SModelAssetComplianceWidget::RefreshAssets)
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text(this, &SModelAssetComplianceWidget::GetSummaryText)
				.AutoWrapText(true)
			]
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildAssetListPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("AssetListTitle", "目录预览"))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 8.f, 0.f, 6.f)
			[
				SNew(SSearchBox)
				.HintText(LOCTEXT("SearchHint", "搜索模型或目录"))
				.OnTextChanged(this, &SModelAssetComplianceWidget::OnSearchTextChanged)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 0.f, 0.f, 8.f)
			[
				BuildDirectoryCategoryFilterButtons()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SAssignNew(AssetTreeView, STreeView<FTreeNodePtr>)
				.TreeItemsSource(&TreeRootNodes)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SModelAssetComplianceWidget::GenerateTreeRow)
				.OnGetChildren(this, &SModelAssetComplianceWidget::GetTreeChildren)
				.OnSelectionChanged(this, &SModelAssetComplianceWidget::OnTreeSelectionChanged)
				.OnContextMenuOpening(this, &SModelAssetComplianceWidget::BuildAssetTreeContextMenu)
			]
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildPreviewPanel()
{
	return SNew(SBorder)
		.Padding(8.f)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("PreviewTitle", "模型预览"))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(0.f, 8.f, 0.f, 0.f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SAssignNew(PreviewViewport, SModelAssetComplianceViewport)
				]
				+ SOverlay::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Top)
				.Padding(10.f)
				[
					SNew(SBox)
					.MaxDesiredWidth(760.f)
					.MaxDesiredHeight(190.f)
					[
						SNew(SBorder)
						.Padding(8.f)
						.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
						[
							SNew(SScrollBox)
							+ SScrollBox::Slot()
							[
								MakeRichTextBlock(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SModelAssetComplianceWidget::GetPreviewLogRichText)))
							]
						]
					]
				]
			]
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildSettingsPanel()
{
	return SNew(SBorder)
		.Padding(10.f)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SettingsTitle", "合规检查与设置"))
						.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 8.f, 0.f, 4.f)
					[
						MakeRichTextBlock(LOCTEXT("FilterNote", "<label>当前模型分类定义</>: 为选中的模型指定它属于哪一类。这个分类只用于左侧目录筛选，不参与阻断逻辑，也不会写入资产。"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						BuildModelCategoryButtons()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						MakeRichTextBlock(LOCTEXT("StatusSettingNote", "<label>当前模型状态设置</>: 自动会使用检查结果；手动状态只覆盖当前工具里的显示状态，不写入资产，也不改变检查项。"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						BuildStatusSettingButtons()
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 4.f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SModelAssetComplianceWidget::IsIgnoreMaterialSlotChecked)
						.OnCheckStateChanged(this, &SModelAssetComplianceWidget::OnIgnoreMaterialSlotChanged)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("IgnoreMaterialSlot", "忽略多材质槽警告"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						SNew(SCheckBox)
						.IsChecked(this, &SModelAssetComplianceWidget::IsIgnoreCollisionChecked)
						.OnCheckStateChanged(this, &SModelAssetComplianceWidget::OnIgnoreCollisionChanged)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("IgnoreCollision", "忽略碰撞警告"))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						MakeRichTextBlock(LOCTEXT("LODMappingNote", "<label>默认四档映射</>: <info>Epic=LOD0，High=LOD0，Mid=LOD1，Low=LOD1</>。<warn>场景模型至少需要 LOD0 和 LOD1 才算合规。</>"))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 0.f, 0.f, 10.f)
					[
						MakeRichTextBlock(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SModelAssetComplianceWidget::GetSelectedDetailsRichText)))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 8.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("MessagesTitle", "检查信息"))
						.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.f, 4.f, 0.f, 0.f)
					[
						MakeRichTextBlock(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &SModelAssetComplianceWidget::GetSelectedMessagesRichText)))
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.f, 10.f, 0.f, 0.f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.Text(LOCTEXT("OpenSelectedModelSettings", "打开模型设置"))
				.ToolTipText(LOCTEXT("OpenSelectedModelSettingsTooltip", "在 Content Drawer 中定位当前 StaticMesh，并打开 StaticMesh 资产编辑器。LOD、碰撞和材质槽等模型设置在资产编辑器中维护。"))
				.IsEnabled(this, &SModelAssetComplianceWidget::HasSelectedAssetAction)
				.OnClicked(this, &SModelAssetComplianceWidget::OpenSelectedModelSettings)
			]
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildDirectoryCategoryFilterButtons()
{
	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(4.f, 4.f));

	for (const FCategoryFilterDefinition& Definition : GetCategoryFilterDefinitions())
	{
		WrapBox->AddSlot()
		[
			BuildDirectoryCategoryFilterButton(Definition.Category, FText::FromString(Definition.Label))
		];
	}

	return WrapBox;
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildDirectoryCategoryFilterButton(EModelAssetComplianceCategory Category, const FText& Label)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SModelAssetComplianceWidget::IsDirectoryCategoryFilterChecked, Category)
		.OnCheckStateChanged(this, &SModelAssetComplianceWidget::OnDirectoryCategoryFilterChanged, Category)
		.ToolTipText(LOCTEXT("DirectoryCategoryFilterTooltip", "切换该分类是否显示在左侧目录预览中。"))
		[
			SNew(STextBlock)
			.Text(Label)
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildModelCategoryButtons()
{
	TSharedRef<SWrapBox> WrapBox = SNew(SWrapBox)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(4.f, 4.f));

	for (const FCategoryFilterDefinition& Definition : GetCategoryFilterDefinitions())
	{
		WrapBox->AddSlot()
		[
			BuildModelCategoryButton(Definition.Category, FText::FromString(Definition.Label))
		];
	}

	return WrapBox;
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildModelCategoryButton(EModelAssetComplianceCategory Category, const FText& Label)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SModelAssetComplianceWidget::IsModelCategoryChecked, Category)
		.OnCheckStateChanged(this, &SModelAssetComplianceWidget::OnModelCategoryChanged, Category)
		.ToolTipText(LOCTEXT("ModelCategoryTooltip", "把当前选中的模型定义为该分类；左侧目录筛选会按这个分类显示它。"))
		[
			SNew(STextBlock)
			.Text(Label)
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildStatusSettingButtons()
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 4.f, 4.f)
		[
			BuildAutoStatusButton()
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 4.f, 4.f)
		[
			BuildStatusSettingButton(EModelAssetComplianceStatus::Pass, LOCTEXT("StatusButtonPass", "通过"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 4.f, 4.f)
		[
			BuildStatusSettingButton(EModelAssetComplianceStatus::Warning, LOCTEXT("StatusButtonWarning", "警告"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 4.f, 4.f)
		[
			BuildStatusSettingButton(EModelAssetComplianceStatus::Blocked, LOCTEXT("StatusButtonBlocked", "阻断"))
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.f, 0.f, 0.f, 4.f)
		[
			BuildStatusSettingButton(EModelAssetComplianceStatus::Excluded, LOCTEXT("StatusButtonExcluded", "排除"))
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildAutoStatusButton()
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SModelAssetComplianceWidget::IsAutoStatusChecked)
		.OnCheckStateChanged(this, &SModelAssetComplianceWidget::OnAutoStatusChanged)
		.ToolTipText(LOCTEXT("AutoStatusTooltip", "使用当前合规检查自动计算的状态。"))
		[
			SNew(STextBlock)
			.Text(LOCTEXT("StatusButtonAuto", "自动"))
		];
}

TSharedRef<SWidget> SModelAssetComplianceWidget::BuildStatusSettingButton(EModelAssetComplianceStatus Status, const FText& Label)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SModelAssetComplianceWidget::IsStatusSettingChecked, Status)
		.OnCheckStateChanged(this, &SModelAssetComplianceWidget::OnStatusSettingChanged, Status)
		.ToolTipText(LOCTEXT("ManualStatusTooltip", "手动设置当前模型在工具中的显示状态；不会写入资产。"))
		[
			SNew(STextBlock)
			.Text(Label)
		];
}

FReply SModelAssetComplianceWidget::RefreshAssets()
{
	ScanAssets();
	return FReply::Handled();
}

void SModelAssetComplianceWidget::ScanAssets()
{
	AllItems.Reset();

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FString> PathsToScan;
	PathsToScan.Add(ArtRootPath);
	AssetRegistry.ScanPathsSynchronous(PathsToScan, true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(ArtRootPath));
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssets(Filter, AssetDataList);
	AssetDataList.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	for (const FAssetData& AssetData : AssetDataList)
	{
		FComplianceItemPtr Item = MakeShared<FModelAssetComplianceItem>();
		Item->AssetData = AssetData;
		Item->AssetName = AssetData.AssetName.ToString();
		Item->PackagePath = AssetData.PackagePath.ToString();
		Item->Category = ClassifyAsset(AssetData);
		Item->Mesh = Cast<UStaticMesh>(AssetData.GetAsset());
		Item->Thumbnail = MakeShared<FAssetThumbnail>(AssetData, 48, 48, ThumbnailPool);
		EvaluateAsset(*Item);
		AllItems.Add(Item);
	}

	RebuildFilteredItems();
}

void SModelAssetComplianceWidget::ReevaluateAssets()
{
	for (const FComplianceItemPtr& Item : AllItems)
	{
		if (Item.IsValid())
		{
			EvaluateAsset(*Item);
		}
	}
	RebuildFilteredItems();
}

void SModelAssetComplianceWidget::EvaluateAsset(FModelAssetComplianceItem& Item) const
{
	Item.Blockers.Reset();
	Item.Warnings.Reset();
	Item.Infos.Reset();
	Item.Status = EModelAssetComplianceStatus::Blocked;

	UStaticMesh* StaticMesh = Item.Mesh.Get();
	if (!StaticMesh && Item.AssetData.IsValid())
	{
		StaticMesh = Cast<UStaticMesh>(Item.AssetData.GetAsset());
		Item.Mesh = StaticMesh;
	}

	if (!StaticMesh)
	{
		Item.Blockers.Add(LOCTEXT("MeshLoadFailed", "无法加载 StaticMesh 资产，无法检查性能分级合规。"));
		return;
	}

	Item.LODCount = StaticMesh->GetNumLODs();
	Item.MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
	Item.LOD0Triangles = GetTriangleCount(StaticMesh, 0);
	Item.LOD1Triangles = GetTriangleCount(StaticMesh, 1);

	const UBodySetup* BodySetup = StaticMesh->GetBodySetup();
	Item.SimpleCollisionShapeCount = BodySetup ? BodySetup->AggGeom.GetElementCount() : 0;
	Item.bHasSimpleCollision = Item.SimpleCollisionShapeCount > 0;

	if (Item.LODCount < 2)
	{
		Item.Blockers.Add(LOCTEXT("MissingLOD1", "缺少 LOD1。当前默认四档映射为 Epic=LOD0，High=LOD0，Mid=LOD1，Low=LOD1；场景模型至少需要 LOD0 和 LOD1。"));
	}
	else
	{
		Item.Infos.Add(LOCTEXT("LODMappingReady", "LOD 映射可用：Epic=LOD0，High=LOD0，Mid=LOD1，Low=LOD1。"));
	}

	if (Item.MaterialSlotCount > 1)
	{
		if (bIgnoreMaterialSlotWarning)
		{
			Item.Infos.Add(LOCTEXT("MaterialSlotIgnored", "材质槽超过 1 个，但当前已勾选忽略多材质槽警告。"));
		}
		else
		{
			Item.Warnings.Add(LOCTEXT("MaterialSlotWarning", "材质槽超过 1 个。推荐场景模型合规资产使用 1 个材质槽；多槽不阻断，但会降低后续打包合批收益和维护稳定性。"));
		}
	}

	if (!Item.bHasSimpleCollision)
	{
		if (bIgnoreCollisionWarning)
		{
			Item.Infos.Add(LOCTEXT("CollisionIgnored", "资产没有简单碰撞，但当前已勾选忽略碰撞警告；场景中可按需要手动设置碰撞。"));
		}
		else
		{
			Item.Warnings.Add(LOCTEXT("CollisionWarning", "资产没有简单碰撞。允许无碰撞导入，但需要确认场景中是否会手动配置碰撞，或该模型完全不需要碰撞。"));
		}
	}

	Item.Infos.Add(LOCTEXT("TriangleDisplayOnly", "三角面数量仅展示，当前不作为警告或阻断标准。"));
	Item.Infos.Add(LOCTEXT("CategoryFilterOnly", "当前模型分类只用于左侧目录筛选，不参与阻断逻辑，也不会写入资产。"));
	Item.Infos.Add(LOCTEXT("BatchDeferred", "此窗口只检查资产合规；正式合批、代理、Texture Collection、替换和写资产统一在后续合批工具链执行。"));

	if (!Item.Blockers.IsEmpty())
	{
		Item.Status = EModelAssetComplianceStatus::Blocked;
	}
	else if (!Item.Warnings.IsEmpty())
	{
		Item.Status = EModelAssetComplianceStatus::Warning;
	}
	else
	{
		Item.Status = EModelAssetComplianceStatus::Pass;
	}

	if (Item.bHasStatusOverride)
	{
		Item.Status = Item.StatusOverride;
		Item.Infos.Add(FText::Format(
			LOCTEXT("ManualStatusOverrideInfo", "状态已手动设置为 {0}；该设置只影响当前工具显示，不写入资产，也不改变检查项。"),
			FText::FromString(StatusToString(Item.StatusOverride))));
	}
}

void SModelAssetComplianceWidget::RebuildFilteredItems()
{
	TSet<FString> ExpandedFolderPaths;
	const bool bHadTreeState = AssetTreeView.IsValid() && !TreeRootNodes.IsEmpty();
	if (bHadTreeState)
	{
		CaptureExpandedFolderPaths(ExpandedFolderPaths);
	}

	FilteredItems.Reset();
	for (const FComplianceItemPtr& Item : AllItems)
	{
		if (Item.IsValid() && PassesCurrentFilter(*Item))
		{
			FilteredItems.Add(Item);
		}
	}

	RebuildTreeNodes();

	if (AssetTreeView.IsValid())
	{
		AssetTreeView->RequestTreeRefresh();
		RestoreExpandedFolderPaths(ExpandedFolderPaths, !bHadTreeState);
	}

	if (!SelectedItem.IsValid() || !FilteredItems.Contains(SelectedItem))
	{
		SelectedItem = FilteredItems.IsEmpty() ? nullptr : FilteredItems[0];
		if (PreviewViewport.IsValid())
		{
			PreviewViewport->SetStaticMesh(SelectedItem.IsValid() ? SelectedItem->Mesh.Get() : nullptr);
		}
	}

	if (AssetTreeView.IsValid() && SelectedItem.IsValid())
	{
		SelectTreeNodeForItem(SelectedItem);
	}
}

void SModelAssetComplianceWidget::RebuildTreeNodes()
{
	TreeRootNodes.Reset();

	TMap<FString, FTreeNodePtr> FolderNodes;
	for (const FComplianceItemPtr& Item : FilteredItems)
	{
		if (!Item.IsValid())
		{
			continue;
		}

		TArray<FTreeNodePtr>* CurrentChildren = &TreeRootNodes;
		FString CurrentPath;
		TArray<FString> Segments;
		MakeRelativeFolderPath(Item->PackagePath).ParseIntoArray(Segments, TEXT("/"), true);

		for (const FString& Segment : Segments)
		{
			CurrentPath = CurrentPath.IsEmpty() ? Segment : FString::Printf(TEXT("%s/%s"), *CurrentPath, *Segment);
			FTreeNodePtr* ExistingNode = FolderNodes.Find(CurrentPath);
			if (!ExistingNode)
			{
				FTreeNodePtr FolderNode = MakeShared<FModelAssetComplianceTreeNode>();
				FolderNode->DisplayName = Segment;
				FolderNode->FullPath = CurrentPath;
				FolderNode->bIsFolder = true;
				FolderNodes.Add(CurrentPath, FolderNode);
				CurrentChildren->Add(FolderNode);
				ExistingNode = FolderNodes.Find(CurrentPath);
			}

			CurrentChildren = &((*ExistingNode)->Children);
		}

		FTreeNodePtr AssetNode = MakeShared<FModelAssetComplianceTreeNode>();
		AssetNode->DisplayName = Item->AssetName;
		AssetNode->FullPath = FString::Printf(TEXT("%s/%s"), *Item->PackagePath, *Item->AssetName);
		AssetNode->bIsFolder = false;
		AssetNode->AssetItem = Item;
		CurrentChildren->Add(AssetNode);
	}

	SortTreeNodes(TreeRootNodes);
}

void SModelAssetComplianceWidget::CaptureExpandedFolderPaths(TSet<FString>& OutExpandedFolderPaths) const
{
	if (!AssetTreeView.IsValid())
	{
		return;
	}

	TArray<FTreeNodePtr> Stack = TreeRootNodes;
	while (!Stack.IsEmpty())
	{
		const FTreeNodePtr Node = Stack.Pop(EAllowShrinking::No);
		if (!Node.IsValid())
		{
			continue;
		}

		if (Node->bIsFolder)
		{
			if (AssetTreeView->IsItemExpanded(Node))
			{
				OutExpandedFolderPaths.Add(Node->FullPath);
			}

			for (const FTreeNodePtr& Child : Node->Children)
			{
				Stack.Add(Child);
			}
		}
	}
}

void SModelAssetComplianceWidget::RestoreExpandedFolderPaths(const TSet<FString>& ExpandedFolderPaths, bool bExpandRootFolders)
{
	if (!AssetTreeView.IsValid())
	{
		return;
	}

	for (const FTreeNodePtr& RootNode : TreeRootNodes)
	{
		RestoreExpandedFolderPathsRecursive(RootNode, ExpandedFolderPaths, bExpandRootFolders, true);
	}
}

void SModelAssetComplianceWidget::RestoreExpandedFolderPathsRecursive(FTreeNodePtr Node, const TSet<FString>& ExpandedFolderPaths, bool bExpandRootFolders, bool bIsRoot)
{
	if (!Node.IsValid() || !Node->bIsFolder || !AssetTreeView.IsValid())
	{
		return;
	}

	const bool bShouldExpand = ExpandedFolderPaths.Contains(Node->FullPath) || (bExpandRootFolders && bIsRoot);
	AssetTreeView->SetItemExpansion(Node, bShouldExpand);

	for (const FTreeNodePtr& Child : Node->Children)
	{
		RestoreExpandedFolderPathsRecursive(Child, ExpandedFolderPaths, bExpandRootFolders, false);
	}
}

bool SModelAssetComplianceWidget::SelectTreeNodeForItem(const FComplianceItemPtr& Item)
{
	if (!AssetTreeView.IsValid() || !Item.IsValid())
	{
		return false;
	}

	for (const FTreeNodePtr& RootNode : TreeRootNodes)
	{
		if (SelectTreeNodeForItemRecursive(RootNode, Item))
		{
			return true;
		}
	}
	return false;
}

bool SModelAssetComplianceWidget::SelectTreeNodeForItemRecursive(FTreeNodePtr Node, const FComplianceItemPtr& Item)
{
	if (!Node.IsValid())
	{
		return false;
	}

	if (!Node->bIsFolder)
	{
		if (Node->AssetItem == Item)
		{
			AssetTreeView->SetSelection(Node);
			AssetTreeView->RequestScrollIntoView(Node);
			return true;
		}
		return false;
	}

	for (const FTreeNodePtr& Child : Node->Children)
	{
		if (SelectTreeNodeForItemRecursive(Child, Item))
		{
			AssetTreeView->SetItemExpansion(Node, true);
			return true;
		}
	}

	return false;
}

bool SModelAssetComplianceWidget::PassesCurrentFilter(const FModelAssetComplianceItem& Item) const
{
	if (!EnabledDirectoryCategoryFilters.Contains(Item.Category))
	{
		return false;
	}

	if (!SearchText.IsEmpty())
	{
		const FString Haystack = FString::Printf(TEXT("%s %s"), *Item.AssetName, *Item.PackagePath);
		if (!Haystack.Contains(SearchText, ESearchCase::IgnoreCase))
		{
			return false;
		}
	}

	return true;
}

TSharedRef<ITableRow> SModelAssetComplianceWidget::GenerateTreeRow(FTreeNodePtr Node, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!Node.IsValid() || Node->bIsFolder)
	{
		const FText FolderName = Node.IsValid() ? FText::FromString(Node->DisplayName) : LOCTEXT("InvalidFolder", "Folder");
		return SNew(STableRow<FTreeNodePtr>, OwnerTable)
			.Padding(4.f)
			[
				SNew(STextBlock)
				.Text(FolderName)
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
			];
	}

	FComplianceItemPtr Item = Node->AssetItem;
	TSharedRef<SWidget> ThumbnailWidget = SNew(SBox);
	if (Item.IsValid() && Item->Thumbnail.IsValid())
	{
		ThumbnailWidget = Item->Thumbnail->MakeThumbnailWidget();
	}

	const FText AssetName = Item.IsValid() ? FText::FromString(Item->AssetName) : LOCTEXT("InvalidAsset", "Invalid");
	const FText PackagePath = Item.IsValid() ? FText::FromString(Item->PackagePath) : FText::GetEmpty();
	const EModelAssetComplianceStatus DisplayStatus = Item.IsValid() ? GetDisplayStatus(*Item) : EModelAssetComplianceStatus::Blocked;
	const FText StatusText = Item.IsValid()
		? FText::FromString(FString::Printf(TEXT("%s / %s"), *CategoryToString(Item->Category), *StatusToString(DisplayStatus)))
		: FText::GetEmpty();
	const FSlateColor StatusColor = Item.IsValid() ? StatusToColor(DisplayStatus) : FSlateColor::UseForeground();

	return SNew(STableRow<FTreeNodePtr>, OwnerTable)
		.Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.f, 0.f, 8.f, 0.f)
			[
				SNew(SBox)
				.WidthOverride(48.f)
				.HeightOverride(48.f)
				[
					ThumbnailWidget
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.f)
			.VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(AssetName)
					.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(PackagePath)
					.ColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.62f, 0.62f)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.Text(StatusText)
					.ColorAndOpacity(StatusColor)
				]
			]
		];
}

void SModelAssetComplianceWidget::GetTreeChildren(FTreeNodePtr Node, TArray<FTreeNodePtr>& OutChildren) const
{
	if (Node.IsValid())
	{
		OutChildren.Append(Node->Children);
	}
}

TSharedPtr<SWidget> SModelAssetComplianceWidget::BuildAssetTreeContextMenu()
{
	if (AssetTreeView.IsValid())
	{
		const TArray<FTreeNodePtr> SelectedNodes = AssetTreeView->GetSelectedItems();
		for (const FTreeNodePtr& SelectedNode : SelectedNodes)
		{
			if (SelectedNode.IsValid() && !SelectedNode->bIsFolder)
			{
				SelectedItem = SelectedNode->AssetItem;
				break;
			}
		}
	}

	if (!HasSelectedAssetAction())
	{
		return nullptr;
	}

	FMenuBuilder MenuBuilder(true, nullptr);
	MenuBuilder.AddMenuEntry(
		LOCTEXT("ContextOpenModel", "打开模型"),
		LOCTEXT("ContextOpenModelTooltip", "打开当前选中的 StaticMesh 资产编辑器。"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.Edit")),
		FUIAction(
			FExecuteAction::CreateSP(this, &SModelAssetComplianceWidget::ExecuteOpenSelectedAsset),
			FCanExecuteAction::CreateSP(this, &SModelAssetComplianceWidget::HasSelectedAssetAction)));

	MenuBuilder.AddMenuEntry(
		LOCTEXT("ContextShowModelInContent", "打开模型位置（Content）"),
		LOCTEXT("ContextShowModelInContentTooltip", "打开 Content Drawer 并定位当前 StaticMesh 资产。"),
		FSlateIcon(FAppStyle::GetAppStyleSetName(), TEXT("Icons.FolderOpen")),
		FUIAction(
			FExecuteAction::CreateSP(this, &SModelAssetComplianceWidget::ExecuteShowSelectedAssetInContentBrowser),
			FCanExecuteAction::CreateSP(this, &SModelAssetComplianceWidget::HasSelectedAssetAction)));

	return MenuBuilder.MakeWidget();
}

void SModelAssetComplianceWidget::OnTreeSelectionChanged(FTreeNodePtr Node, ESelectInfo::Type SelectInfo)
{
	if (!Node.IsValid() || Node->bIsFolder)
	{
		return;
	}

	SelectedItem = Node->AssetItem;
	if (PreviewViewport.IsValid())
	{
		PreviewViewport->SetStaticMesh(SelectedItem.IsValid() ? SelectedItem->Mesh.Get() : nullptr);
	}
}

void SModelAssetComplianceWidget::OnSearchTextChanged(const FText& InSearchText)
{
	SearchText = InSearchText.ToString();
	RebuildFilteredItems();
}

void SModelAssetComplianceWidget::OnDirectoryCategoryFilterChanged(ECheckBoxState NewState, EModelAssetComplianceCategory Category)
{
	if (NewState == ECheckBoxState::Checked)
	{
		EnabledDirectoryCategoryFilters.Add(Category);
	}
	else
	{
		EnabledDirectoryCategoryFilters.Remove(Category);
	}
	RebuildFilteredItems();
}

ECheckBoxState SModelAssetComplianceWidget::IsDirectoryCategoryFilterChecked(EModelAssetComplianceCategory Category) const
{
	return EnabledDirectoryCategoryFilters.Contains(Category) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SModelAssetComplianceWidget::OnModelCategoryChanged(ECheckBoxState NewState, EModelAssetComplianceCategory Category)
{
	if (NewState != ECheckBoxState::Checked || !SelectedItem.IsValid())
	{
		return;
	}

	SelectedItem->Category = Category;
	EvaluateAsset(*SelectedItem);
	RebuildFilteredItems();
}

ECheckBoxState SModelAssetComplianceWidget::IsModelCategoryChecked(EModelAssetComplianceCategory Category) const
{
	return SelectedItem.IsValid() && SelectedItem->Category == Category ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SModelAssetComplianceWidget::OnAutoStatusChanged(ECheckBoxState NewState)
{
	if (NewState != ECheckBoxState::Checked || !SelectedItem.IsValid())
	{
		return;
	}

	SelectedItem->bHasStatusOverride = false;
	EvaluateAsset(*SelectedItem);
	RebuildFilteredItems();
}

ECheckBoxState SModelAssetComplianceWidget::IsAutoStatusChecked() const
{
	return SelectedItem.IsValid() && !SelectedItem->bHasStatusOverride ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SModelAssetComplianceWidget::OnStatusSettingChanged(ECheckBoxState NewState, EModelAssetComplianceStatus Status)
{
	if (NewState != ECheckBoxState::Checked || !SelectedItem.IsValid())
	{
		return;
	}

	SelectedItem->bHasStatusOverride = true;
	SelectedItem->StatusOverride = Status;
	EvaluateAsset(*SelectedItem);
	RebuildFilteredItems();
}

ECheckBoxState SModelAssetComplianceWidget::IsStatusSettingChecked(EModelAssetComplianceStatus Status) const
{
	return SelectedItem.IsValid() && SelectedItem->bHasStatusOverride && SelectedItem->StatusOverride == Status
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
}

void SModelAssetComplianceWidget::OnIgnoreMaterialSlotChanged(ECheckBoxState NewState)
{
	bIgnoreMaterialSlotWarning = NewState == ECheckBoxState::Checked;
	ReevaluateAssets();
}

ECheckBoxState SModelAssetComplianceWidget::IsIgnoreMaterialSlotChecked() const
{
	return bIgnoreMaterialSlotWarning ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SModelAssetComplianceWidget::OnIgnoreCollisionChanged(ECheckBoxState NewState)
{
	bIgnoreCollisionWarning = NewState == ECheckBoxState::Checked;
	ReevaluateAssets();
}

ECheckBoxState SModelAssetComplianceWidget::IsIgnoreCollisionChecked() const
{
	return bIgnoreCollisionWarning ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool SModelAssetComplianceWidget::HasSelectedAssetAction() const
{
	return SelectedItem.IsValid() && SelectedItem->AssetData.IsValid();
}

void SModelAssetComplianceWidget::ExecuteOpenSelectedAsset()
{
	if (!HasSelectedAssetAction() || !GEditor)
	{
		return;
	}

	UObject* Asset = SelectedItem->AssetData.GetAsset();
	if (!Asset)
	{
		return;
	}

	if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
	{
		AssetEditorSubsystem->OpenEditorForAsset(Asset);
	}
}

void SModelAssetComplianceWidget::ExecuteShowSelectedAssetInContentBrowser()
{
	if (!HasSelectedAssetAction() || !GEditor)
	{
		return;
	}

	TArray<FAssetData> AssetsToSync;
	AssetsToSync.Add(SelectedItem->AssetData);
	GEditor->SyncBrowserToObjects(AssetsToSync, true);
}

FReply SModelAssetComplianceWidget::OpenSelectedModelSettings()
{
	if (!HasSelectedAssetAction())
	{
		return FReply::Handled();
	}

	ExecuteShowSelectedAssetInContentBrowser();
	ExecuteOpenSelectedAsset();
	return FReply::Handled();
}

FText SModelAssetComplianceWidget::GetSummaryText() const
{
	int32 PassCount = 0;
	int32 WarningCount = 0;
	int32 BlockedCount = 0;
	int32 ExcludedCount = 0;
	for (const FComplianceItemPtr& Item : AllItems)
	{
		if (!Item.IsValid())
		{
			continue;
		}

		switch (GetDisplayStatus(*Item))
		{
		case EModelAssetComplianceStatus::Pass:
			++PassCount;
			break;
		case EModelAssetComplianceStatus::Warning:
			++WarningCount;
			break;
		case EModelAssetComplianceStatus::Blocked:
			++BlockedCount;
			break;
		case EModelAssetComplianceStatus::Excluded:
			++ExcludedCount;
			break;
		default:
			break;
		}
	}

	return FText::Format(
		LOCTEXT("Summary", "已扫描 {0} 个 StaticMesh，当前显示 {1} 个。通过 {2}，警告 {3}，阻断 {4}，排除 {5}。"),
		FText::AsNumber(AllItems.Num()),
		FText::AsNumber(FilteredItems.Num()),
		FText::AsNumber(PassCount),
		FText::AsNumber(WarningCount),
		FText::AsNumber(BlockedCount),
		FText::AsNumber(ExcludedCount));
}

FText SModelAssetComplianceWidget::GetPreviewLogRichText() const
{
	if (!SelectedItem.IsValid())
	{
		return FText::FromString(FString::Printf(TEXT("<muted>%s</>"), *EscapeRichText(GetSummaryText().ToString())));
	}

	return FText::FromString(FString::Printf(
		TEXT("<label>%s</>  <%s>%s</>\n<muted>%s</>\n%s"),
		*EscapeRichText(SelectedItem->AssetName),
		RichTagForStatus(GetDisplayStatus(*SelectedItem)),
		*EscapeRichText(StatusToString(GetDisplayStatus(*SelectedItem))),
		*EscapeRichText(SelectedItem->PackagePath),
		*BuildMessageRichText(SelectedItem.Get())));
}

FText SModelAssetComplianceWidget::GetSelectedAssetTitleText() const
{
	return SelectedItem.IsValid() ? FText::FromString(SelectedItem->AssetName) : LOCTEXT("NoSelection", "未选择");
}

FText SModelAssetComplianceWidget::GetSelectedAssetPathText() const
{
	return SelectedItem.IsValid() ? FText::FromString(SelectedItem->PackagePath) : FText::GetEmpty();
}

FText SModelAssetComplianceWidget::GetSelectedCategoryText() const
{
	return SelectedItem.IsValid() ? FText::FromString(CategoryToString(SelectedItem->Category)) : FText::GetEmpty();
}

FText SModelAssetComplianceWidget::GetSelectedStatusText() const
{
	return SelectedItem.IsValid() ? FText::FromString(StatusToString(GetDisplayStatus(*SelectedItem))) : FText::GetEmpty();
}

FText SModelAssetComplianceWidget::GetSelectedLODText() const
{
	if (!SelectedItem.IsValid())
	{
		return FText::GetEmpty();
	}

	return FText::Format(
		LOCTEXT("LODText", "LOD 数量 {0}；Epic/High -> LOD0，Mid/Low -> LOD1"),
		FText::AsNumber(SelectedItem->LODCount));
}

FText SModelAssetComplianceWidget::GetSelectedMaterialText() const
{
	if (!SelectedItem.IsValid())
	{
		return FText::GetEmpty();
	}
	return FText::Format(LOCTEXT("MaterialText", "{0} 个材质槽；推荐 1 个，超过 1 个只作为警告。"), FText::AsNumber(SelectedItem->MaterialSlotCount));
}

FText SModelAssetComplianceWidget::GetSelectedCollisionText() const
{
	if (!SelectedItem.IsValid())
	{
		return FText::GetEmpty();
	}

	return SelectedItem->bHasSimpleCollision
		? FText::Format(LOCTEXT("CollisionReady", "有简单碰撞，形状数 {0}。"), FText::AsNumber(SelectedItem->SimpleCollisionShapeCount))
		: LOCTEXT("CollisionMissing", "没有简单碰撞；允许，但需要确认场景侧设置。");
}

FText SModelAssetComplianceWidget::GetSelectedTriangleText() const
{
	if (!SelectedItem.IsValid())
	{
		return FText::GetEmpty();
	}

	return FText::Format(
		LOCTEXT("TriangleText", "LOD0: {0} tris；LOD1: {1} tris。仅展示，不参与当前合规判定。"),
		FText::AsNumber(SelectedItem->LOD0Triangles),
		FText::AsNumber(SelectedItem->LOD1Triangles));
}

FText SModelAssetComplianceWidget::GetSelectedMessagesText() const
{
	return FText::FromString(BuildMessageRichText(SelectedItem.Get()));
}

FText SModelAssetComplianceWidget::GetSelectedDetailsRichText() const
{
	if (!SelectedItem.IsValid())
	{
		return LOCTEXT("NoSelectedDetails", "<muted>未选择模型。</>");
	}

	return FText::FromString(FString::Printf(
		TEXT("<label>资产</>: %s\n<label>路径</>: <muted>%s</>\n<label>分类</>: %s\n<label>状态</>: <%s>%s</>\n<label>LOD</>: %s\n<label>材质槽</>: %s\n<label>碰撞</>: %s\n<label>三角面</>: %s"),
		*EscapeRichText(GetSelectedAssetTitleText().ToString()),
		*EscapeRichText(GetSelectedAssetPathText().ToString()),
		*EscapeRichText(GetSelectedCategoryText().ToString()),
		RichTagForStatus(GetDisplayStatus(*SelectedItem)),
		*EscapeRichText(GetSelectedStatusText().ToString()),
		*EscapeRichText(GetSelectedLODText().ToString()),
		*EscapeRichText(GetSelectedMaterialText().ToString()),
		*EscapeRichText(GetSelectedCollisionText().ToString()),
		*EscapeRichText(GetSelectedTriangleText().ToString())));
}

FText SModelAssetComplianceWidget::GetSelectedMessagesRichText() const
{
	return FText::FromString(BuildMessageRichText(SelectedItem.Get()));
}

FSlateColor SModelAssetComplianceWidget::GetSelectedStatusColor() const
{
	return SelectedItem.IsValid() ? StatusToColor(GetDisplayStatus(*SelectedItem)) : FSlateColor::UseForeground();
}

EModelAssetComplianceCategory SModelAssetComplianceWidget::ClassifyAsset(const FAssetData& AssetData)
{
	FString LowerPath = FString::Printf(TEXT("%s/%s"), *AssetData.PackagePath.ToString(), *AssetData.AssetName.ToString());
	LowerPath.ToLowerInline();

	if (PathContainsAny(LowerPath, { TEXT("/character"), TEXT("/characters"), TEXT("/enemy"), TEXT("/enemies"), TEXT("/npc"), TEXT("/player") }))
	{
		return EModelAssetComplianceCategory::Character;
	}
	if (PathContainsAny(LowerPath, { TEXT("/weapon"), TEXT("/weapons") }))
	{
		return EModelAssetComplianceCategory::Weapon;
	}
	if (PathContainsAny(LowerPath, { TEXT("/interact"), TEXT("/interactive"), TEXT("/door"), TEXT("/pickup"), TEXT("/mechanism"), TEXT("/switch"), TEXT("/trigger") }))
	{
		return EModelAssetComplianceCategory::Interactive;
	}
	if (PathContainsAny(LowerPath, { TEXT("/decal"), TEXT("/decals") }))
	{
		return EModelAssetComplianceCategory::Decal;
	}
	if (PathContainsAny(LowerPath, { TEXT("/item"), TEXT("/items"), TEXT("/collectible"), TEXT("/gameplayprop"), TEXT("/usable") }))
	{
		return EModelAssetComplianceCategory::Prop;
	}
	if (PathContainsAny(LowerPath, { TEXT("/ui"), TEXT("/vfx"), TEXT("/fx"), TEXT("/effect"), TEXT("/effects"), TEXT("/dynamic"), TEXT("/movable"), TEXT("/physics") }))
	{
		return EModelAssetComplianceCategory::OtherAsset;
	}
	if (PathContainsAny(LowerPath, { TEXT("/ground"), TEXT("/floor"), TEXT("/terrain"), TEXT("/road"), TEXT("/pavement") }))
	{
		return EModelAssetComplianceCategory::SceneGround;
	}
	if (PathContainsAny(LowerPath, { TEXT("/wall"), TEXT("/building"), TEXT("/architecture"), TEXT("/arch"), TEXT("/house"), TEXT("/structure"), TEXT("/arcadewall") }))
	{
		return EModelAssetComplianceCategory::SceneBuilding;
	}
	if (PathContainsAny(LowerPath, { TEXT("/prop"), TEXT("/props"), TEXT("/deco"), TEXT("/decoration"), TEXT("/furniture"), TEXT("/object"), TEXT("/objects") }))
	{
		return EModelAssetComplianceCategory::SceneProp;
	}
	if (PathContainsAny(LowerPath, { TEXT("/environment"), TEXT("/environmentasset"), TEXT("/scene"), TEXT("/level") }))
	{
		return EModelAssetComplianceCategory::SceneOther;
	}

	return EModelAssetComplianceCategory::OtherAsset;
}

bool SModelAssetComplianceWidget::IsWarningStatus(EModelAssetComplianceStatus Status)
{
	return Status == EModelAssetComplianceStatus::Warning || Status == EModelAssetComplianceStatus::Blocked;
}

EModelAssetComplianceStatus SModelAssetComplianceWidget::GetDisplayStatus(const FModelAssetComplianceItem& Item)
{
	return Item.bHasStatusOverride ? Item.StatusOverride : Item.Status;
}

FString SModelAssetComplianceWidget::MakeRelativeFolderPath(const FString& PackagePath)
{
	FString RelativePath = PackagePath;
	const FString ArtPrefix = FString::Printf(TEXT("%s/"), ArtRootPath);
	if (RelativePath.StartsWith(ArtPrefix))
	{
		RelativePath.RightChopInline(ArtPrefix.Len());
	}
	else if (RelativePath == ArtRootPath)
	{
		RelativePath = TEXT("Art");
	}
	else if (RelativePath.StartsWith(TEXT("/Game/")))
	{
		RelativePath.RightChopInline(6);
	}

	RelativePath.RemoveFromStart(TEXT("/"));
	return RelativePath.IsEmpty() ? TEXT("Art") : RelativePath;
}

FString SModelAssetComplianceWidget::CategoryToString(EModelAssetComplianceCategory Category)
{
	switch (Category)
	{
	case EModelAssetComplianceCategory::Interactive:
		return TEXT("交互物");
	case EModelAssetComplianceCategory::Character:
		return TEXT("角色");
	case EModelAssetComplianceCategory::Weapon:
		return TEXT("武器");
	case EModelAssetComplianceCategory::Prop:
		return TEXT("道具");
	case EModelAssetComplianceCategory::SceneProp:
		return TEXT("场景模型-物件");
	case EModelAssetComplianceCategory::SceneBuilding:
		return TEXT("场景模型-建筑");
	case EModelAssetComplianceCategory::SceneGround:
		return TEXT("场景模型-地面");
	case EModelAssetComplianceCategory::SceneOther:
		return TEXT("场景模型-其他");
	case EModelAssetComplianceCategory::Decal:
		return TEXT("贴花模型");
	case EModelAssetComplianceCategory::OtherAsset:
		return TEXT("其他资产");
	default:
		return TEXT("未知");
	}
}

FString SModelAssetComplianceWidget::StatusToString(EModelAssetComplianceStatus Status)
{
	switch (Status)
	{
	case EModelAssetComplianceStatus::Pass:
		return TEXT("通过");
	case EModelAssetComplianceStatus::Warning:
		return TEXT("警告");
	case EModelAssetComplianceStatus::Blocked:
		return TEXT("阻断");
	case EModelAssetComplianceStatus::Excluded:
		return TEXT("已排除");
	default:
		return TEXT("未知");
	}
}

FSlateColor SModelAssetComplianceWidget::StatusToColor(EModelAssetComplianceStatus Status)
{
	switch (Status)
	{
	case EModelAssetComplianceStatus::Pass:
		return FSlateColor(FLinearColor(0.20f, 0.75f, 0.32f));
	case EModelAssetComplianceStatus::Warning:
		return FSlateColor(FLinearColor(0.95f, 0.65f, 0.18f));
	case EModelAssetComplianceStatus::Blocked:
		return FSlateColor(FLinearColor(0.95f, 0.22f, 0.18f));
	case EModelAssetComplianceStatus::Excluded:
		return FSlateColor(FLinearColor(0.55f, 0.55f, 0.55f));
	default:
		return FSlateColor::UseForeground();
	}
}

int32 SModelAssetComplianceWidget::GetTriangleCount(const UStaticMesh* StaticMesh, int32 LODIndex)
{
	if (!StaticMesh || StaticMesh->GetNumLODs() <= LODIndex)
	{
		return 0;
	}
	return StaticMesh->GetNumTriangles(LODIndex);
}

#undef LOCTEXT_NAMESPACE
