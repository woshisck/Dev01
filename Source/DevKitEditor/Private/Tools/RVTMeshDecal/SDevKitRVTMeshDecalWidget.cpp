#include "Tools/RVTMeshDecal/SDevKitRVTMeshDecalWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "IContentBrowserSingleton.h"
#include "ILevelEditor.h"
#include "InputCoreTypes.h"
#include "LevelEditor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "RVT/DevKitRVTSurfaceInstanceActor.h"
#include "SLevelViewport.h"
#include "Styling/AppStyle.h"
#include "Tools/DevKitArtToolUI.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "SDevKitRVTMeshDecalWidget"

namespace
{
	FString ToObjectPath(const FAssetData& AssetData)
	{
		return AssetData.IsValid() ? AssetData.GetSoftObjectPath().ToString() : FString();
	}

	FText GetSurfaceTypeText(EDevKitRVTSurfaceAssetType Type)
	{
		return Type == EDevKitRVTSurfaceAssetType::PlaneDecal
			? LOCTEXT("PlaneDecalType", "平面贴花")
			: LOCTEXT("VisibleObjectType", "可见物件");
	}

	FText GetGeometryPolicyText(const UDevKitRVTSurfaceAsset* Asset)
	{
		if (!Asset || Asset->AssetType == EDevKitRVTSurfaceAssetType::PlaneDecal)
		{
			return LOCTEXT("GeometryRVTOnlySummary", "仅 RVT 投射");
		}

		switch (Asset->GeometryPolicy)
		{
		case EDevKitRVTSurfaceGeometryPolicy::RVTOnly:
			return LOCTEXT("GeometryExplicitRVTOnlySummary", "始终仅 RVT 投射");
		case EDevKitRVTSurfaceGeometryPolicy::QualityScaled:
			return LOCTEXT("GeometryQualityScaledSummary", "Quality Scaled（High/Epic 保留模型，Mid/Low 投射）");
		case EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible:
		case EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault:
		default:
			return LOCTEXT("GeometryAlwaysVisibleSummary", "始终保留模型");
		}
	}

	FText GetObjectDisplayName(const FString& ObjectPath)
	{
		return ObjectPath.IsEmpty()
			? LOCTEXT("NotSelected", "未选择")
			: FText::FromString(FPackageName::ObjectPathToObjectName(ObjectPath));
	}

	TSharedRef<SWidget> MakeSettingsRow(const FText& Label, const TSharedRef<SWidget>& ValueWidget)
	{
		return SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(0.0f, 0.0f, 8.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(104.0f)
				[
					SNew(STextBlock).Text(Label)
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				ValueWidget
			];
	}

	DECLARE_DELEGATE_OneParam(FOnSurfaceAssetCardSelected, const TSharedPtr<FAssetData>&);
	DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnSurfaceAssetCardDragged, const TSharedPtr<FAssetData>&);
	DECLARE_DELEGATE_OneParam(FOnSurfacePlacementFinished, const FDevKitRVTSurfaceControllerResult&);

	class FDevKitRVTSurfaceAssetDragDropOp final : public FDecoratedDragDropOp
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(FDevKitRVTSurfaceAssetDragDropOp, FDecoratedDragDropOp)

		static TSharedRef<FDevKitRVTSurfaceAssetDragDropOp> New(
			UDevKitRVTSurfaceAsset* InAsset,
			FOnSurfacePlacementFinished InOnFinished)
		{
			TSharedRef<FDevKitRVTSurfaceAssetDragDropOp> Operation = MakeShared<FDevKitRVTSurfaceAssetDragDropOp>();
			Operation->SurfaceAsset = InAsset;
			Operation->OnFinished = MoveTemp(InOnFinished);
			Operation->CurrentHoverText = FText::Format(
				LOCTEXT("DragDecoratorText", "放置 1 个 {0}"),
				InAsset && !InAsset->DisplayName.IsEmpty()
					? InAsset->DisplayName
					: FText::FromString(InAsset ? InAsset->GetName() : TEXT("RVT Surface Asset")));
			Operation->CurrentIconBrush = FAppStyle::GetBrush(TEXT("Icons.Plus"));
			Operation->SetupDefaults();
			Operation->Construct();
			return Operation;
		}

		virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override
		{
			FDevKitRVTSurfaceControllerResult Result;
			bool bFoundLevelViewport = false;

			if (!SurfaceAsset.IsValid())
			{
				Result.Message = LOCTEXT("DragAssetInvalid", "拖放失败：地表资产已失效。");
			}
			else
			{
				FLevelEditorModule& LevelEditorModule =
					FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				if (const TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor())
				{
					for (const TSharedPtr<SLevelViewport>& ViewportWidget : LevelEditor->GetViewports())
					{
						if (ViewportWidget.IsValid()
							&& ViewportWidget->GetCachedGeometry().IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
						{
							bFoundLevelViewport = true;
							Result = FDevKitRVTMeshDecalService::PlaceSurfaceAssetInstanceAtViewportCursor(
								&ViewportWidget->GetLevelViewportClient(),
								SurfaceAsset.Get(),
								true);
							break;
						}
					}
				}
			}

			if (!bFoundLevelViewport && Result.Message.IsEmpty())
			{
				Result.Message = LOCTEXT(
					"DragNotOverViewport",
					"未放置：请把地表资产卡片拖到关卡视口中的地面或 Static Mesh 表面。");
			}

			OnFinished.ExecuteIfBound(Result);
			FDecoratedDragDropOp::OnDrop(bDropWasHandled || Result.bSuccess, MouseEvent);
		}

	private:
		TWeakObjectPtr<UDevKitRVTSurfaceAsset> SurfaceAsset;
		FOnSurfacePlacementFinished OnFinished;
	};

	class SDevKitRVTSurfaceAssetCard final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDevKitRVTSurfaceAssetCard) {}
			SLATE_ARGUMENT(TSharedPtr<FAssetData>, Item)
			SLATE_EVENT(FOnSurfaceAssetCardSelected, OnSelected)
			SLATE_EVENT(FOnSurfaceAssetCardDragged, OnDragged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Item = InArgs._Item;
			OnSelected = InArgs._OnSelected;
			OnDragged = InArgs._OnDragged;

			FText Title = FText::GetEmpty();
			FText Type = FText::GetEmpty();
			FText Resources = FText::GetEmpty();
			if (Item.IsValid())
			{
				if (const UDevKitRVTSurfaceAsset* Asset = Cast<UDevKitRVTSurfaceAsset>(Item->GetAsset()))
				{
					Title = Asset->DisplayName.IsEmpty() ? FText::FromName(Item->AssetName) : Asset->DisplayName;
					Type = GetSurfaceTypeText(Asset->AssetType);
					Resources = FText::Format(
						LOCTEXT("CardResources", "模型：{0}  ·  材质：{1}"),
						Asset->Mesh ? FText::FromString(Asset->Mesh->GetName()) : LOCTEXT("CardNoMesh", "无"),
						Asset->Material ? FText::FromString(Asset->Material->GetName()) : LOCTEXT("CardNoMaterial", "无"));
				}
			}

			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
				.Padding(FMargin(8.0f, 7.0f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.VAlign(VAlign_Center)
					.Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush(TEXT("ClassIcon.DataAsset")))
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(Title)
							.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(Type)
							.ColorAndOpacity(FLinearColor(0.36f, 0.78f, 1.0f))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
							.Text(Resources)
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							.AutoWrapText(true)
						]
					]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Item.IsValid())
			{
				OnSelected.ExecuteIfBound(Item);
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}
			return FReply::Unhandled();
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			return Item.IsValid() && OnDragged.IsBound() ? OnDragged.Execute(Item) : FReply::Unhandled();
		}

	private:
		TSharedPtr<FAssetData> Item;
		FOnSurfaceAssetCardSelected OnSelected;
		FOnSurfaceAssetCardDragged OnDragged;
	};
}

void SDevKitRVTMeshDecalWidget::Construct(const FArguments& InArgs)
{
	EditMeshObjectPath = FDevKitRVTMeshDecalService::GetDefaultPlaneMeshObjectPath();
	StatusText = LOCTEXT(
		"InitialStatus",
		"先从左侧选择一个最终地表资产；可拖入视口，也可在右侧放置单个实例。");

	ChildSlot
	[
		SNew(SBorder)
		.Padding(12.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				BuildHeader()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(0.0f, 10.0f, 0.0f, 8.0f)
			[
				SAssignNew(ViewSwitcher, SWidgetSwitcher)
				.WidgetIndex(0)
				+ SWidgetSwitcher::Slot()
				[
					BuildUseView()
				]
				+ SWidgetSwitcher::Slot()
				[
					BuildEditView()
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.Padding(FMargin(8.0f, 6.0f))
				[
					SNew(STextBlock)
					.Text(this, &SDevKitRVTMeshDecalWidget::GetStatusText)
					.ColorAndOpacity(this, &SDevKitRVTMeshDecalWidget::GetStatusColor)
					.AutoWrapText(true)
				]
			]
		]
	];

	RefreshWorldContext(true);
	ResetEditForm();
	RefreshSurfaceAssetLibrary();
}

void SDevKitRVTMeshDecalWidget::Tick(
	const FGeometry& AllottedGeometry,
	double InCurrentTime,
	float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const FString CurrentWorldPackagePath = GetCurrentWorldPackagePath();
	if (!CurrentWorldPackagePath.Equals(LastWorldPackagePath, ESearchCase::CaseSensitive))
	{
		RefreshWorldContext(!EditingSurfaceAsset.IsValid());
	}
}

#if WITH_DEV_AUTOMATION_TESTS
bool SDevKitRVTMeshDecalWidget::IsUseViewActiveForAutomation() const
{
	return ViewMode == EViewMode::Use
		&& ViewSwitcher.IsValid()
		&& ViewSwitcher->GetActiveWidgetIndex() == 0;
}
#endif

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildHeader()
{
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			DevKitArtToolUI::MakeHeader(
				LOCTEXT("Title", "RVT 地表物件库"),
				LOCTEXT(
					"Description",
					"最终地表资产把模型、材质与放置规则组合成一个可复用条目；平面贴花与保留模型的可见物件使用同一套实例工作流。"))
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.0f, 10.0f, 0.0f, 0.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
				.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsViewModeChecked, EViewMode::Use)
				.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnViewModeChanged, EViewMode::Use)
				[
					SNew(STextBlock).Text(LOCTEXT("UseViewToggle", "使用地表资产"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
				.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsViewModeChecked, EViewMode::Edit)
				.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnViewModeChanged, EViewMode::Edit)
				[
					SNew(STextBlock).Text(LOCTEXT("EditViewToggle", "编辑地表资产"))
				]
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("WorkflowHint", "使用页放置  ·  编辑页组合模型 + 材质 + 规则"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildUseView()
{
	return SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.36f)
		[
			BuildLibraryPanel()
		]
		+ SSplitter::Slot()
		.Value(0.64f)
		[
			BuildUsePanel()
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildLibraryPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("LibraryTitle", "最终地表资产"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f)
			[
				SAssignNew(LibrarySearchBox, SSearchBox)
				.HintText(LOCTEXT("LibrarySearchHint", "搜索资产、模型或材质"))
				.OnTextChanged(this, &SDevKitRVTMeshDecalWidget::OnLibrarySearchChanged)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
					.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsLibraryFilterChecked, ELibraryFilter::All)
					.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnLibraryFilterChanged, ELibraryFilter::All)
					[SNew(STextBlock).Text(LOCTEXT("FilterAll", "全部"))]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
					.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsLibraryFilterChecked, ELibraryFilter::PlaneDecal)
					.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnLibraryFilterChanged, ELibraryFilter::PlaneDecal)
					[SNew(STextBlock).Text(LOCTEXT("FilterPlane", "平面贴花"))]
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SCheckBox)
					.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
					.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsLibraryFilterChecked, ELibraryFilter::VisibleObject)
					.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnLibraryFilterChanged, ELibraryFilter::VisibleObject)
					[SNew(STextBlock).Text(LOCTEXT("FilterObject", "可见物件"))]
				]
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				SAssignNew(SurfaceAssetListView, SListView<TSharedPtr<FAssetData>>)
				.ListItemsSource(&FilteredSurfaceAssets)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SDevKitRVTMeshDecalWidget::GenerateSurfaceAssetRow)
				.OnSelectionChanged(this, &SDevKitRVTMeshDecalWidget::OnSurfaceAssetSelectionChanged)
				.OnMouseButtonDoubleClick(this, &SDevKitRVTMeshDecalWidget::OnSurfaceAssetDoubleClicked)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 5.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("EditSelected", "编辑选中"))
					.IsEnabled(this, &SDevKitRVTMeshDecalWidget::HasSelectedSurfaceAsset)
					.OnClicked(this, &SDevKitRVTMeshDecalWidget::EditSelectedSurfaceAsset)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 5.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("NewAsset", "新建"))
					.OnClicked(this, &SDevKitRVTMeshDecalWidget::CreateNewSurfaceAsset)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("RefreshLibrary", "刷新"))
					.OnClicked(this, &SDevKitRVTMeshDecalWidget::RefreshLibrary)
				]
			]
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildUsePanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(10.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(this, &SDevKitRVTMeshDecalWidget::GetSelectedAssetTitle)
					.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f, 0.0f, 10.0f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitRVTMeshDecalWidget::GetSelectedAssetSummary)
					.AutoWrapText(true)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					DevKitArtToolUI::MakeSectionHeader(
						1,
						LOCTEXT("AutomaticRVTSection", "当前关卡 RVT 自动绑定"),
						LOCTEXT("AutomaticRVTSectionDesc", "放置前自动从当前关卡解析 Surface RVT；可见物件按资产设置同时绑定 WorldHeight。"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 12.0f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitRVTMeshDecalWidget::GetAutomaticRVTText)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					DevKitArtToolUI::MakeSectionHeader(
						2,
						LOCTEXT("ControllerSection", "控制 Actor 与单实例编辑"),
						LOCTEXT("ControllerSectionDesc", "每个最终资产由独立 ISM 控制 Actor 管理；同一模型和材质保持合批，同时每个实例可单独变换。"))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 8.0f)
				[
					SNew(STextBlock)
					.Text(this, &SDevKitRVTMeshDecalWidget::GetControllerSummary)
					.AutoWrapText(true)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
					[
						SNew(SButton)
						.Text(LOCTEXT("CreateSelectController", "创建/选择控制 Actor"))
						.IsEnabled(this, &SDevKitRVTMeshDecalWidget::HasSelectedSurfaceAsset)
						.OnClicked(this, &SDevKitRVTMeshDecalWidget::CreateOrSelectControllerActor)
					]
					+ SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("PlaceSingle", "在当前活动透视视口中心放置单个实例"))
						.ToolTipText(LOCTEXT(
							"PlaceSingleTooltip",
							"使用最近激活的透视 Level 视口中心射线放置；需要精确落点时，请把左侧资产卡片直接拖到视口。"))
						.IsEnabled(this, &SDevKitRVTMeshDecalWidget::HasSelectedSurfaceAsset)
						.OnClicked(this, &SDevKitRVTMeshDecalWidget::PlaceSingleInstanceAtViewportCenter)
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 8.0f)
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
					.Padding(9.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT(
							"DragHint",
							"拖放：左侧每张最终资产卡片都可以直接拖到 Level 视口中的准确位置。"))
						.AutoWrapText(true)
					]
				]
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
					.Padding(9.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT(
							"NativeEditHint",
							"单实例编辑：先选择控制 Actor，再在视口中点击某一个网格实例。按 W / E / R 可只移动、旋转或缩放该实例；不需要进入植被模式。"))
						.AutoWrapText(true)
					]
				]
			]
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildEditView()
{
	return SNew(SSplitter)
		.Orientation(Orient_Horizontal)
		+ SSplitter::Slot()
		.Value(0.27f)
		[
			BuildMeshPickerPanel()
		]
		+ SSplitter::Slot()
		.Value(0.33f)
		[
			BuildMaterialPickerPanel()
		]
		+ SSplitter::Slot()
		.Value(0.40f)
		[
			BuildAssetSettingsPanel()
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildMeshPickerPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MeshPickerTitle", "1  选择使用模型"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text(this, &SDevKitRVTMeshDecalWidget::GetSelectedMeshText)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				CreateMeshPicker()
			]
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildMaterialPickerPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaterialPickerTitle", "2  选择使用材质"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 4.0f, 0.0f, 2.0f)
			[
				SNew(STextBlock)
				.Text(this, &SDevKitRVTMeshDecalWidget::GetSelectedMaterialText)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("MaterialPickerHint", "这里不限制当前地图目录；材质库内容由项目自行配置。"))
				.AutoWrapText(true)
				.ColorAndOpacity(FLinearColor(0.36f, 0.78f, 1.0f))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				CreateMaterialPicker()
			]
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::BuildAssetSettingsPanel()
{
	return SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		.Padding(8.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(LOCTEXT("SettingsTitle", "3  配置最终地表资产"))
				.Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f).Padding(0.0f, 6.0f, 0.0f, 4.0f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						MakeSettingsRow(
							LOCTEXT("FolderLabel", "保存目录"),
							SAssignNew(AssetFolderTextBox, SEditableTextBox)
							.Text_Lambda([this]() { return FText::FromString(EditAssetFolder); })
							.IsReadOnly_Lambda([this]() { return EditingSurfaceAsset.IsValid(); })
							.ToolTipText(LOCTEXT(
								"ExistingAssetFolderLockedTooltip",
								"编辑已有资产时保存目录不可修改，以防覆盖其他库资产；如需另建资产，请返回使用页点击“新建”。"))
							.OnTextChanged_Lambda([this](const FText& Text) { EditAssetFolder = Text.ToString(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						MakeSettingsRow(
							LOCTEXT("AssetNameLabel", "资产名称"),
							SAssignNew(AssetNameTextBox, SEditableTextBox)
							.Text_Lambda([this]() { return FText::FromString(EditAssetName); })
							.IsReadOnly_Lambda([this]() { return EditingSurfaceAsset.IsValid(); })
							.ToolTipText(LOCTEXT(
								"ExistingAssetNameLockedTooltip",
								"编辑已有资产时资产名称不可修改，以防覆盖其他库资产；如需另建资产，请返回使用页点击“新建”。"))
							.OnTextChanged_Lambda([this](const FText& Text) { EditAssetName = Text.ToString(); }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						MakeSettingsRow(
							LOCTEXT("DisplayNameLabel", "显示名称"),
							SAssignNew(DisplayNameTextBox, SEditableTextBox)
							.Text_Lambda([this]() { return EditDisplayName; })
							.OnTextChanged_Lambda([this](const FText& Text) { EditDisplayName = Text; }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						MakeSettingsRow(
							LOCTEXT("DescriptionLabel", "说明"),
							SNew(SBox)
							.MinDesiredHeight(62.0f)
							[
								SNew(SMultiLineEditableTextBox)
								.Text_Lambda([this]() { return EditDescription; })
								.OnTextChanged_Lambda([this](const FText& Text) { EditDescription = Text; })
							])
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 6.0f)
					[
						MakeSettingsRow(
							LOCTEXT("TypeLabel", "资产类型"),
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
							[
								SNew(SCheckBox)
								.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
								.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsEditAssetTypeChecked, EDevKitRVTSurfaceAssetType::PlaneDecal)
								.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnEditAssetTypeChanged, EDevKitRVTSurfaceAssetType::PlaneDecal)
								[SNew(STextBlock).Text(LOCTEXT("EditPlaneType", "平面贴花"))]
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SCheckBox)
								.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
								.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsEditAssetTypeChecked, EDevKitRVTSurfaceAssetType::VisibleObject)
								.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnEditAssetTypeChanged, EDevKitRVTSurfaceAssetType::VisibleObject)
								[SNew(STextBlock).Text(LOCTEXT("EditObjectType", "可见物件"))]
							])
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
					[
						MakeSettingsRow(
							LOCTEXT("GeometryPolicyLabel", "几何策略"),
							SNew(SHorizontalBox)
							.IsEnabled_Lambda([this]() { return EditAssetType == EDevKitRVTSurfaceAssetType::VisibleObject; })
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
							[
								SNew(SCheckBox)
								.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
								.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsEditGeometryPolicyChecked, EDevKitRVTSurfaceGeometryPolicy::QualityScaled)
								.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnEditGeometryPolicyChanged, EDevKitRVTSurfaceGeometryPolicy::QualityScaled)
								.ToolTipText(LOCTEXT("QualityScaledTooltip", "PC High/Epic 保留模型；Mid/Low 仅保留 RVT 投射；掌机可由 DeviceProfile 强制投射。"))
								[SNew(STextBlock).Text(LOCTEXT("QualityScaledPolicy", "Quality Scaled"))]
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f)
							[
								SNew(SCheckBox)
								.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
								.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsEditGeometryPolicyChecked, EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible)
								.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnEditGeometryPolicyChanged, EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible)
								.ToolTipText(LOCTEXT("AlwaysVisibleTooltip", "所有画质与平台都保留源模型；用于玩法碰撞或必须保留轮廓的结构。"))
								[SNew(STextBlock).Text(LOCTEXT("AlwaysVisiblePolicy", "始终保留模型"))]
							]
							+ SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SCheckBox)
								.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
								.IsChecked(this, &SDevKitRVTMeshDecalWidget::IsEditGeometryPolicyChecked, EDevKitRVTSurfaceGeometryPolicy::RVTOnly)
								.OnCheckStateChanged(this, &SDevKitRVTMeshDecalWidget::OnEditGeometryPolicyChanged, EDevKitRVTSurfaceGeometryPolicy::RVTOnly)
								.ToolTipText(LOCTEXT("RVTOnlyTooltip", "所有画质都仅保留 RVT 投射，不进入主渲染与碰撞。"))
								[SNew(STextBlock).Text(LOCTEXT("RVTOnlyPolicy", "始终仅投射"))]
							])
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						MakeSettingsRow(
							LOCTEXT("PriorityLabel", "Priority"),
							SNew(SSpinBox<int32>)
							.MinValue(-1000)
							.MaxValue(1000)
							.Value_Lambda([this]() { return EditPriority; })
							.OnValueChanged_Lambda([this](int32 Value) { EditPriority = Value; }))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
					[
						MakeSettingsRow(
							LOCTEXT("ScaleLabel", "默认缩放"),
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
							[
								SNew(SSpinBox<double>)
								.MinValue(0.001)
								.MaxValue(1000.0)
								.Value_Lambda([this]() { return EditDefaultScale.X; })
								.OnValueChanged_Lambda([this](double Value) { EditDefaultScale.X = Value; })
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(0.0f, 0.0f, 3.0f, 0.0f)
							[
								SNew(SSpinBox<double>)
								.MinValue(0.001)
								.MaxValue(1000.0)
								.Value_Lambda([this]() { return EditDefaultScale.Y; })
								.OnValueChanged_Lambda([this](double Value) { EditDefaultScale.Y = Value; })
							]
							+ SHorizontalBox::Slot().FillWidth(1.0f)
							[
								SNew(SSpinBox<double>)
								.MinValue(0.001)
								.MaxValue(1000.0)
								.Value_Lambda([this]() { return EditDefaultScale.Z; })
								.OnValueChanged_Lambda([this](double Value) { EditDefaultScale.Z = Value; })
							])
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
					[
						MakeSettingsRow(
							LOCTEXT("ZOffsetLabel", "Z Offset"),
							SNew(SSpinBox<double>)
							.MinValue(-10000.0)
							.MaxValue(10000.0)
							.Value_Lambda([this]() { return EditZOffset; })
							.OnValueChanged_Lambda([this](double Value) { EditZOffset = Value; }))
					]
					+ SVerticalBox::Slot().AutoHeight()[SNew(SSeparator)]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 7.0f, 0.0f, 2.0f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bEditAlignToNormal ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bEditAlignToNormal = State == ECheckBoxState::Checked; })
						[SNew(STextBlock).Text(LOCTEXT("AlignNormal", "对齐表面法线"))]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bEditRandomYaw ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bEditRandomYaw = State == ECheckBoxState::Checked; })
						[SNew(STextBlock).Text(LOCTEXT("RandomYaw", "随机 Yaw"))]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SCheckBox)
						.IsEnabled_Lambda([this]() { return EditAssetType == EDevKitRVTSurfaceAssetType::VisibleObject; })
						.IsChecked_Lambda([this]() { return bEditBindWorldHeight ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bEditBindWorldHeight = State == ECheckBoxState::Checked; })
						[SNew(STextBlock).Text(LOCTEXT("BindHeight", "同时绑定 WorldHeight RVT"))]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SCheckBox)
						.IsEnabled_Lambda([this]()
						{
							return EditAssetType == EDevKitRVTSurfaceAssetType::VisibleObject
								&& (EditGeometryPolicy == EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible
									|| EditGeometryPolicy == EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault);
						})
						.IsChecked_Lambda([this]() { return bEditEnableCollision ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bEditEnableCollision = State == ECheckBoxState::Checked; })
						.ToolTipText(LOCTEXT("EnableCollisionTooltip", "玩法碰撞不能随画质或平台消失，因此仅“始终保留模型”的可见物件可以启用。"))
						[SNew(STextBlock).Text(LOCTEXT("EnableCollision", "启用碰撞"))]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f)
					[
						SNew(SCheckBox)
						.IsChecked_Lambda([this]() { return bEditCastShadow ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
						.OnCheckStateChanged_Lambda([this](ECheckBoxState State) { bEditCastShadow = State == ECheckBoxState::Checked; })
						[SNew(STextBlock).Text(LOCTEXT("CastShadow", "投射阴影"))]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("SaveSurfaceAsset", "保存最终地表资产"))
					.IsEnabled(this, &SDevKitRVTMeshDecalWidget::CanSaveSurfaceAsset)
					.OnClicked(this, &SDevKitRVTMeshDecalWidget::SaveSurfaceAsset)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton)
					.Text(LOCTEXT("ReturnToUse", "返回使用页"))
					.OnClicked(this, &SDevKitRVTMeshDecalWidget::ReturnToUseView)
				]
			]
		];
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::CreateMeshPicker()
{
	FAssetPickerConfig PickerConfig;
	PickerConfig.SelectionMode = ESelectionMode::Single;
	PickerConfig.Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	PickerConfig.Filter.bRecursiveClasses = true;
	PickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	PickerConfig.bAllowNullSelection = false;
	PickerConfig.bAllowDragging = false;
	PickerConfig.bAllowRename = false;
	PickerConfig.bCanShowClasses = false;
	PickerConfig.bCanShowDevelopersFolder = true;
	PickerConfig.bForceShowPluginContent = true;
	PickerConfig.bShowBottomToolbar = true;
	PickerConfig.SaveSettingsName = TEXT("DevKitRVTSurfaceMeshLibrary");
	PickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SDevKitRVTMeshDecalWidget::OnMeshSelected);
	PickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SDevKitRVTMeshDecalWidget::OnMeshSelected);

	IContentBrowserSingleton& ContentBrowser =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get();
	return ContentBrowser.CreateAssetPicker(PickerConfig);
}

TSharedRef<SWidget> SDevKitRVTMeshDecalWidget::CreateMaterialPicker()
{
	FAssetPickerConfig PickerConfig;
	PickerConfig.SelectionMode = ESelectionMode::Single;
	PickerConfig.Filter.ClassPaths.Add(UMaterialInterface::StaticClass()->GetClassPathName());
	PickerConfig.Filter.bRecursiveClasses = true;
	PickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	PickerConfig.bAllowNullSelection = false;
	PickerConfig.bAllowDragging = false;
	PickerConfig.bAllowRename = false;
	PickerConfig.bCanShowClasses = false;
	PickerConfig.bCanShowDevelopersFolder = true;
	PickerConfig.bForceShowPluginContent = true;
	PickerConfig.bShowBottomToolbar = true;
	PickerConfig.SaveSettingsName = TEXT("DevKitRVTSurfaceMaterialLibrary");
	PickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SDevKitRVTMeshDecalWidget::OnMaterialSelected);
	PickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SDevKitRVTMeshDecalWidget::OnMaterialSelected);

	IContentBrowserSingleton& ContentBrowser =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser")).Get();
	return ContentBrowser.CreateAssetPicker(PickerConfig);
}

UWorld* SDevKitRVTMeshDecalWidget::GetEditorWorld() const
{
	return GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
}

FString SDevKitRVTMeshDecalWidget::GetCurrentWorldPackagePath() const
{
	const UWorld* World = GetEditorWorld();
	return World && World->GetOutermost() ? World->GetOutermost()->GetName() : FString();
}

void SDevKitRVTMeshDecalWidget::RefreshWorldContext(bool bResetDefaultFolder)
{
	LastWorldPackagePath = GetCurrentWorldPackagePath();
	if (bResetDefaultFolder && !LastWorldPackagePath.IsEmpty())
	{
		EditAssetFolder = FDevKitRVTMeshDecalService::InferDefaultSurfaceAssetFolderFromWorldPackage(
			LastWorldPackagePath);
	}
	RefreshAutomaticRVTBindings();
}

void SDevKitRVTMeshDecalWidget::RefreshAutomaticRVTBindings()
{
	const FDevKitRVTAutoBindingResult Binding =
		FDevKitRVTMeshDecalService::ResolveRuntimeVirtualTexturesForWorld(GetEditorWorld());
	AutomaticSurfaceRVTPath = Binding.SurfaceObjectPath;
	AutomaticHeightRVTPath = Binding.HeightObjectPath;
	AutomaticRVTMessage = Binding.Message;
}

void SDevKitRVTMeshDecalWidget::RefreshSurfaceAssetLibrary()
{
	const FString PreviousSelectionPath = SelectedSurfaceAssetItem.IsValid()
		? ToObjectPath(*SelectedSurfaceAssetItem)
		: FString();

	AllSurfaceAssets.Reset();
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FARFilter Filter;
	Filter.ClassPaths.Add(UDevKitRVTSurfaceAsset::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> Assets;
	AssetRegistryModule.Get().GetAssets(Filter, Assets);
	Assets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.AssetName.LexicalLess(Right.AssetName);
	});
	for (const FAssetData& AssetData : Assets)
	{
		AllSurfaceAssets.Add(MakeShared<FAssetData>(AssetData));
	}

	ApplyLibraryFilter();
	if (!PreviousSelectionPath.IsEmpty())
	{
		for (const TSharedPtr<FAssetData>& Item : FilteredSurfaceAssets)
		{
			if (Item.IsValid() && ToObjectPath(*Item).Equals(PreviousSelectionPath, ESearchCase::CaseSensitive))
			{
				SelectSurfaceAssetFromCard(Item);
				break;
			}
		}
	}
}

void SDevKitRVTMeshDecalWidget::ApplyLibraryFilter()
{
	FilteredSurfaceAssets.Reset();
	for (const TSharedPtr<FAssetData>& Item : AllSurfaceAssets)
	{
		if (Item.IsValid() && DoesAssetPassLibraryFilter(*Item))
		{
			FilteredSurfaceAssets.Add(Item);
		}
	}

	if (SurfaceAssetListView.IsValid())
	{
		SurfaceAssetListView->RequestListRefresh();
	}

	const bool bSelectionStillVisible = SelectedSurfaceAssetItem.IsValid()
		&& FilteredSurfaceAssets.Contains(SelectedSurfaceAssetItem);
	if (!bSelectionStillVisible)
	{
		SelectedSurfaceAssetItem.Reset();
		if (SurfaceAssetListView.IsValid())
		{
			SurfaceAssetListView->ClearSelection();
		}
	}

	if (!SelectedSurfaceAssetItem.IsValid() && FilteredSurfaceAssets.Num() > 0)
	{
		SelectSurfaceAssetFromCard(FilteredSurfaceAssets[0]);
	}
}

bool SDevKitRVTMeshDecalWidget::DoesAssetPassLibraryFilter(const FAssetData& AssetData) const
{
	const UDevKitRVTSurfaceAsset* Asset = Cast<UDevKitRVTSurfaceAsset>(AssetData.GetAsset());
	if (!Asset)
	{
		return false;
	}

	if (LibraryFilter == ELibraryFilter::PlaneDecal
		&& Asset->AssetType != EDevKitRVTSurfaceAssetType::PlaneDecal)
	{
		return false;
	}
	if (LibraryFilter == ELibraryFilter::VisibleObject
		&& Asset->AssetType != EDevKitRVTSurfaceAssetType::VisibleObject)
	{
		return false;
	}

	if (LibrarySearchText.IsEmpty())
	{
		return true;
	}

	const FString Searchable = FString::Printf(
		TEXT("%s %s %s %s"),
		*AssetData.AssetName.ToString(),
		*Asset->DisplayName.ToString(),
		Asset->Mesh ? *Asset->Mesh->GetName() : TEXT(""),
		Asset->Material ? *Asset->Material->GetName() : TEXT(""));
	return Searchable.Contains(LibrarySearchText, ESearchCase::IgnoreCase);
}

TSharedRef<ITableRow> SDevKitRVTMeshDecalWidget::GenerateSurfaceAssetRow(
	TSharedPtr<FAssetData> Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TSharedPtr<FAssetData>>, OwnerTable)
		.Padding(2.0f)
		[
			SNew(SDevKitRVTSurfaceAssetCard)
			.Item(Item)
			.OnSelected(FOnSurfaceAssetCardSelected::CreateSP(
				this,
				&SDevKitRVTMeshDecalWidget::SelectSurfaceAssetFromCard))
			.OnDragged(FOnSurfaceAssetCardDragged::CreateSP(
				this,
				&SDevKitRVTMeshDecalWidget::BeginSurfaceAssetDrag))
		];
}

void SDevKitRVTMeshDecalWidget::OnSurfaceAssetSelectionChanged(
	TSharedPtr<FAssetData> Item,
	ESelectInfo::Type SelectInfo)
{
	SelectedSurfaceAssetItem = Item;
	RefreshAutomaticRVTBindings();
}

void SDevKitRVTMeshDecalWidget::OnSurfaceAssetDoubleClicked(TSharedPtr<FAssetData> Item)
{
	SelectSurfaceAssetFromCard(Item);
	EditSelectedSurfaceAsset();
}

void SDevKitRVTMeshDecalWidget::SelectSurfaceAssetFromCard(const TSharedPtr<FAssetData>& Item)
{
	if (!Item.IsValid())
	{
		return;
	}
	SelectedSurfaceAssetItem = Item;
	if (SurfaceAssetListView.IsValid() && !SurfaceAssetListView->IsItemSelected(Item))
	{
		SurfaceAssetListView->SetSelection(Item, ESelectInfo::Direct);
	}
	RefreshAutomaticRVTBindings();
}

FReply SDevKitRVTMeshDecalWidget::BeginSurfaceAssetDrag(const TSharedPtr<FAssetData>& Item)
{
	SelectSurfaceAssetFromCard(Item);
	UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	if (!Asset)
	{
		SetStatus(LOCTEXT("CannotDragInvalidAsset", "无法拖放：选中的地表资产无效。"), true);
		return FReply::Unhandled();
	}

	return FReply::Handled().BeginDragDrop(FDevKitRVTSurfaceAssetDragDropOp::New(
		Asset,
		FOnSurfacePlacementFinished::CreateSP(
			this,
			&SDevKitRVTMeshDecalWidget::HandleControllerResult)));
}

void SDevKitRVTMeshDecalWidget::OnLibrarySearchChanged(const FText& NewText)
{
	LibrarySearchText = NewText.ToString().TrimStartAndEnd();
	ApplyLibraryFilter();
}

ECheckBoxState SDevKitRVTMeshDecalWidget::IsLibraryFilterChecked(ELibraryFilter Filter) const
{
	return LibraryFilter == Filter ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDevKitRVTMeshDecalWidget::OnLibraryFilterChanged(
	ECheckBoxState NewState,
	ELibraryFilter Filter)
{
	if (NewState == ECheckBoxState::Checked)
	{
		LibraryFilter = Filter;
		ApplyLibraryFilter();
	}
}

UDevKitRVTSurfaceAsset* SDevKitRVTMeshDecalWidget::GetSelectedSurfaceAsset() const
{
	return SelectedSurfaceAssetItem.IsValid()
		? Cast<UDevKitRVTSurfaceAsset>(SelectedSurfaceAssetItem->GetAsset())
		: nullptr;
}

ADevKitRVTSurfaceInstanceActor* SDevKitRVTMeshDecalWidget::FindSelectedSurfaceInstanceActor() const
{
	UWorld* World = GetEditorWorld();
	UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	if (!World || !Asset)
	{
		return nullptr;
	}
	ULevel* TargetLevel = World->GetCurrentLevel();
	if (!TargetLevel)
	{
		TargetLevel = World->PersistentLevel.Get();
	}

	for (TActorIterator<ADevKitRVTSurfaceInstanceActor> It(World); It; ++It)
	{
		if (It->GetLevel() == TargetLevel && It->SurfaceAsset == Asset)
		{
			return *It;
		}
	}
	return nullptr;
}

FText SDevKitRVTMeshDecalWidget::GetSelectedAssetTitle() const
{
	const UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	if (!Asset)
	{
		return LOCTEXT("NoSelectedAssetTitle", "请选择一个最终地表资产");
	}
	return Asset->DisplayName.IsEmpty() ? FText::FromString(Asset->GetName()) : Asset->DisplayName;
}

FText SDevKitRVTMeshDecalWidget::GetSelectedAssetSummary() const
{
	const UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	if (!Asset)
	{
		return LOCTEXT(
			"NoSelectedAssetSummary",
			"左侧只显示已经配置完成的地表资产。点击“编辑地表资产”可以用三栏界面创建新的条目。");
	}

	return FText::Format(
			LOCTEXT(
				"SelectedAssetSummary",
				"类型：{0}  ·  几何策略：{1}\n模型：{2}\n材质：{3}\n默认缩放：{4}  ·  Z Offset：{5}  ·  Priority：{6}"),
		GetSurfaceTypeText(Asset->AssetType),
		GetGeometryPolicyText(Asset),
		Asset->Mesh ? FText::FromString(Asset->Mesh->GetPathName()) : LOCTEXT("SummaryNoMesh", "未设置"),
		Asset->Material ? FText::FromString(Asset->Material->GetPathName()) : LOCTEXT("SummaryNoMaterial", "未设置"),
		FText::FromString(Asset->DefaultScale.ToCompactString()),
		FText::AsNumber(Asset->ZOffset),
		FText::AsNumber(Asset->Priority));
}

FText SDevKitRVTMeshDecalWidget::GetAutomaticRVTText() const
{
	return FText::Format(
		LOCTEXT("AutomaticRVTSummary", "Surface：{0}\nWorldHeight：{1}\n{2}"),
		AutomaticSurfaceRVTPath.IsEmpty()
			? LOCTEXT("SurfaceNotResolved", "未解析")
			: FText::FromString(AutomaticSurfaceRVTPath),
		AutomaticHeightRVTPath.IsEmpty()
			? LOCTEXT("HeightNotResolved", "未解析或当前资产不需要")
			: FText::FromString(AutomaticHeightRVTPath),
		AutomaticRVTMessage);
}

FText SDevKitRVTMeshDecalWidget::GetControllerSummary() const
{
	const ADevKitRVTSurfaceInstanceActor* Actor = FindSelectedSurfaceInstanceActor();
	if (!Actor)
	{
		return LOCTEXT("NoController", "当前关卡尚未创建这个资产的控制 Actor。");
	}
	return FText::Format(
		LOCTEXT("ControllerSummary", "控制 Actor：{0}  ·  实例数量：{1}"),
		FText::FromString(Actor->GetActorLabel()),
		FText::AsNumber(Actor->GetSurfaceInstanceCount()));
}

bool SDevKitRVTMeshDecalWidget::HasSelectedSurfaceAsset() const
{
	return GetSelectedSurfaceAsset() != nullptr;
}

FReply SDevKitRVTMeshDecalWidget::RefreshLibrary()
{
	RefreshSurfaceAssetLibrary();
	RefreshAutomaticRVTBindings();
	SetStatus(
		FText::Format(LOCTEXT("LibraryRefreshed", "地表资产库已刷新，共发现 {0} 个最终资产。"), FText::AsNumber(AllSurfaceAssets.Num())),
		false);
	return FReply::Handled();
}

FReply SDevKitRVTMeshDecalWidget::EditSelectedSurfaceAsset()
{
	UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	if (!Asset)
	{
		SetStatus(LOCTEXT("EditNeedsSelection", "请先从左侧选择一个地表资产。"), true);
		return FReply::Handled();
	}
	LoadEditFormFromAsset(Asset);
	SetViewMode(EViewMode::Edit);
	SetStatus(LOCTEXT("EditingExisting", "已载入选中的最终地表资产；保存后会更新原资产。"), false);
	return FReply::Handled();
}

FReply SDevKitRVTMeshDecalWidget::CreateNewSurfaceAsset()
{
	ResetEditForm();
	SetViewMode(EViewMode::Edit);
	SetStatus(LOCTEXT("EditingNew", "请选择模型和材质，然后配置并保存最终地表资产。"), false);
	return FReply::Handled();
}

FReply SDevKitRVTMeshDecalWidget::CreateOrSelectControllerActor()
{
	UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	UWorld* World = GetEditorWorld();
	if (!Asset || !World)
	{
		SetStatus(LOCTEXT("ControllerNeedsAsset", "无法创建控制 Actor：地表资产或当前关卡无效。"), true);
		return FReply::Handled();
	}

	FDevKitRVTSurfaceControllerResult Result =
		FDevKitRVTMeshDecalService::FindOrCreateSurfaceInstanceActor(World, Asset, true);
	if (Result.bSuccess)
	{
		const FDevKitRVTSurfaceControllerResult SelectResult =
			FDevKitRVTMeshDecalService::SelectSurfaceInstanceActor(World, Asset, true);
		if (SelectResult.bSuccess)
		{
			Result = SelectResult;
		}
	}
	HandleControllerResult(Result);
	return FReply::Handled();
}

FReply SDevKitRVTMeshDecalWidget::PlaceSingleInstanceAtViewportCenter()
{
	UDevKitRVTSurfaceAsset* Asset = GetSelectedSurfaceAsset();
	if (!Asset)
	{
		SetStatus(LOCTEXT("PlaceNeedsAsset", "请先选择一个最终地表资产。"), true);
		return FReply::Handled();
	}

	FLevelEditorViewportClient* ViewportClient = nullptr;
	FLevelEditorModule& LevelEditorModule =
		FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	if (const TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor())
	{
		// A tool-window button click moves the mouse outside the Level viewport.  Use the
		// explicitly active perspective viewport; drag/drop remains cursor-accurate.
		const TSharedPtr<SLevelViewport> ActiveViewport = LevelEditor->GetActiveViewportInterface();
		if (ActiveViewport.IsValid() && ActiveViewport->GetLevelViewportClient().IsPerspective())
		{
			ViewportClient = &ActiveViewport->GetLevelViewportClient();
		}
	}

	if (!ViewportClient)
	{
		SetStatus(
			LOCTEXT("NoPerspectiveViewport", "没有活动的透视关卡视口；请先点击一个透视 Level 视口，再返回此处放置。"),
			true);
		return FReply::Handled();
	}

	HandleControllerResult(FDevKitRVTMeshDecalService::PlaceSurfaceAssetInstanceAtViewportCenter(
		ViewportClient,
		Asset,
		true));
	return FReply::Handled();
}

void SDevKitRVTMeshDecalWidget::HandleControllerResult(const FDevKitRVTSurfaceControllerResult& Result)
{
	SetStatus(Result.Message, !Result.bSuccess);
	if (Result.bSuccess)
	{
		RefreshAutomaticRVTBindings();
	}
}

ECheckBoxState SDevKitRVTMeshDecalWidget::IsViewModeChecked(EViewMode Mode) const
{
	return ViewMode == Mode ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDevKitRVTMeshDecalWidget::OnViewModeChanged(ECheckBoxState NewState, EViewMode Mode)
{
	if (NewState == ECheckBoxState::Checked)
	{
		if (Mode == EViewMode::Edit && ViewMode != EViewMode::Edit && !EditingSurfaceAsset.IsValid())
		{
			ResetEditForm();
		}
		SetViewMode(Mode);
	}
}

void SDevKitRVTMeshDecalWidget::SetViewMode(EViewMode Mode)
{
	ViewMode = Mode;
	if (ViewSwitcher.IsValid())
	{
		ViewSwitcher->SetActiveWidgetIndex(Mode == EViewMode::Use ? 0 : 1);
	}
}

void SDevKitRVTMeshDecalWidget::ResetEditForm()
{
	EditingSurfaceAsset.Reset();
	EditMeshObjectPath = FDevKitRVTMeshDecalService::GetDefaultPlaneMeshObjectPath();
	EditMaterialObjectPath.Reset();
	EditAssetFolder = FDevKitRVTMeshDecalService::InferDefaultSurfaceAssetFolderFromWorldPackage(
		GetCurrentWorldPackagePath());
	EditAssetName.Reset();
	EditDisplayName = FText::GetEmpty();
	EditDescription = FText::GetEmpty();
	EditAssetType = EDevKitRVTSurfaceAssetType::PlaneDecal;
	EditGeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::RVTOnly;
	EditPriority = 0;
	EditDefaultScale = FVector::OneVector;
	EditZOffset = 0.5f;
	bEditAlignToNormal = true;
	bEditRandomYaw = true;
	bEditBindWorldHeight = false;
	bEditEnableCollision = false;
	bEditCastShadow = false;
}

void SDevKitRVTMeshDecalWidget::LoadEditFormFromAsset(UDevKitRVTSurfaceAsset* Asset)
{
	if (!Asset)
	{
		return;
	}
	EditingSurfaceAsset = Asset;
	EditMeshObjectPath = Asset->Mesh ? Asset->Mesh->GetPathName() : FString();
	EditMaterialObjectPath = Asset->Material ? Asset->Material->GetPathName() : FString();
	EditAssetFolder = FPackageName::GetLongPackagePath(Asset->GetOutermost()->GetName());
	EditAssetName = Asset->GetName();
	EditDisplayName = Asset->DisplayName;
	EditDescription = Asset->Description;
	EditAssetType = Asset->AssetType;
	EditGeometryPolicy = Asset->GeometryPolicy;
	if (EditGeometryPolicy == EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault)
	{
		EditGeometryPolicy = EditAssetType == EDevKitRVTSurfaceAssetType::PlaneDecal
			? EDevKitRVTSurfaceGeometryPolicy::RVTOnly
			: EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible;
	}
	EditPriority = Asset->Priority;
	EditDefaultScale = Asset->DefaultScale;
	EditZOffset = Asset->ZOffset;
	bEditAlignToNormal = Asset->bAlignToNormal;
	bEditRandomYaw = Asset->bRandomYaw;
	bEditBindWorldHeight = Asset->bBindWorldHeight;
	bEditEnableCollision = Asset->bEnableCollision;
	if (bEditEnableCollision
		&& EditAssetType == EDevKitRVTSurfaceAssetType::VisibleObject
		&& EditGeometryPolicy != EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible
		&& EditGeometryPolicy != EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault)
	{
		// Old or externally edited invalid assets already render as source geometry at runtime.
		// Reflect that safe behavior in the form so the next save repairs the serialized policy.
		EditGeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible;
	}
	bEditCastShadow = Asset->bCastShadow;
}

void SDevKitRVTMeshDecalWidget::UpdateSuggestedAssetName()
{
	if (EditingSurfaceAsset.IsValid() || EditMeshObjectPath.IsEmpty() || EditMaterialObjectPath.IsEmpty())
	{
		return;
	}

	EditAssetName = FDevKitRVTMeshDecalService::BuildDefaultSurfaceAssetName(
		EditMeshObjectPath,
		EditMaterialObjectPath,
		EditAssetType,
		EditPriority);
	if (EditDisplayName.IsEmpty())
	{
		EditDisplayName = FText::FromString(FPackageName::ObjectPathToObjectName(EditMaterialObjectPath));
	}
}

FDevKitRVTSurfaceAssetRequest SDevKitRVTMeshDecalWidget::BuildSurfaceAssetRequest() const
{
	FDevKitRVTSurfaceAssetRequest Request;
	Request.ExpectedExistingAsset = EditingSurfaceAsset;
	Request.AssetFolder = EditAssetFolder;
	Request.AssetName = EditAssetName;
	Request.DisplayName = EditDisplayName;
	Request.Description = EditDescription;
	Request.AssetType = EditAssetType;
	Request.GeometryPolicy = EditGeometryPolicy;
	Request.MeshObjectPath = EditMeshObjectPath;
	Request.MaterialObjectPath = EditMaterialObjectPath;
	Request.Priority = EditPriority;
	Request.DefaultScale = EditDefaultScale;
	Request.ZOffset = EditZOffset;
	Request.bAlignToNormal = bEditAlignToNormal;
	Request.bRandomYaw = bEditRandomYaw;
	Request.bBindWorldHeight = EditAssetType == EDevKitRVTSurfaceAssetType::VisibleObject && bEditBindWorldHeight;
	Request.bEnableCollision = bEditEnableCollision;
	Request.bCastShadow = bEditCastShadow;
	return Request;
}

bool SDevKitRVTMeshDecalWidget::CanSaveSurfaceAsset() const
{
	return !EditAssetFolder.TrimStartAndEnd().IsEmpty()
		&& !EditAssetName.TrimStartAndEnd().IsEmpty()
		&& !EditMeshObjectPath.IsEmpty()
		&& !EditMaterialObjectPath.IsEmpty()
		&& EditDefaultScale.X > KINDA_SMALL_NUMBER
		&& EditDefaultScale.Y > KINDA_SMALL_NUMBER
		&& EditDefaultScale.Z > KINDA_SMALL_NUMBER;
}

FReply SDevKitRVTMeshDecalWidget::SaveSurfaceAsset()
{
	if (!CanSaveSurfaceAsset())
	{
		SetStatus(LOCTEXT("SaveValidationFailed", "保存失败：请选择模型、材质，并检查保存目录、名称和默认缩放。"), true);
		return FReply::Handled();
	}

	const FDevKitRVTSurfaceAssetResult Result =
		FDevKitRVTMeshDecalService::CreateOrUpdateSurfaceAsset(BuildSurfaceAssetRequest());
	SetStatus(Result.Message, !Result.bSuccess);
	if (!Result.bSuccess)
	{
		return FReply::Handled();
	}

	EditingSurfaceAsset = Result.Asset;
	RefreshSurfaceAssetLibrary();
	for (const TSharedPtr<FAssetData>& Item : AllSurfaceAssets)
	{
		if (Item.IsValid() && ToObjectPath(*Item).Equals(Result.ObjectPath, ESearchCase::CaseSensitive))
		{
			LibraryFilter = ELibraryFilter::All;
			LibrarySearchText.Reset();
			if (LibrarySearchBox.IsValid())
			{
				LibrarySearchBox->SetText(FText::GetEmpty());
			}
			ApplyLibraryFilter();
			SelectSurfaceAssetFromCard(Item);
			break;
		}
	}
	SetViewMode(EViewMode::Use);
	return FReply::Handled();
}

FReply SDevKitRVTMeshDecalWidget::ReturnToUseView()
{
	SetViewMode(EViewMode::Use);
	return FReply::Handled();
}

void SDevKitRVTMeshDecalWidget::OnMeshSelected(const FAssetData& AssetData)
{
	EditMeshObjectPath = ToObjectPath(AssetData);
	UpdateSuggestedAssetName();
}

void SDevKitRVTMeshDecalWidget::OnMaterialSelected(const FAssetData& AssetData)
{
	EditMaterialObjectPath = ToObjectPath(AssetData);
	UpdateSuggestedAssetName();
}

FText SDevKitRVTMeshDecalWidget::GetSelectedMeshText() const
{
	return FText::Format(LOCTEXT("SelectedMesh", "当前：{0}"), GetObjectDisplayName(EditMeshObjectPath));
}

FText SDevKitRVTMeshDecalWidget::GetSelectedMaterialText() const
{
	return FText::Format(LOCTEXT("SelectedMaterial", "当前：{0}"), GetObjectDisplayName(EditMaterialObjectPath));
}

ECheckBoxState SDevKitRVTMeshDecalWidget::IsEditAssetTypeChecked(EDevKitRVTSurfaceAssetType Type) const
{
	return EditAssetType == Type ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDevKitRVTMeshDecalWidget::OnEditAssetTypeChanged(
	ECheckBoxState NewState,
	EDevKitRVTSurfaceAssetType Type)
{
	if (NewState == ECheckBoxState::Checked)
	{
		const bool bWasPlaneDecal = EditAssetType == EDevKitRVTSurfaceAssetType::PlaneDecal;
		EditAssetType = Type;
		if (Type == EDevKitRVTSurfaceAssetType::PlaneDecal)
		{
			bEditBindWorldHeight = false;
			bEditEnableCollision = false;
			EditGeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::RVTOnly;
		}
		else if (bWasPlaneDecal)
		{
			// New visible objects use the requested High/Epic mesh, Mid/Low projection policy.
			EditGeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::QualityScaled;
		}
		UpdateSuggestedAssetName();
	}
}

ECheckBoxState SDevKitRVTMeshDecalWidget::IsEditGeometryPolicyChecked(
	EDevKitRVTSurfaceGeometryPolicy Policy) const
{
	return EditGeometryPolicy == Policy ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDevKitRVTMeshDecalWidget::OnEditGeometryPolicyChanged(
	ECheckBoxState NewState,
	EDevKitRVTSurfaceGeometryPolicy Policy)
{
	if (NewState == ECheckBoxState::Checked
		&& EditAssetType == EDevKitRVTSurfaceAssetType::VisibleObject)
	{
		EditGeometryPolicy = Policy;
		if (Policy != EDevKitRVTSurfaceGeometryPolicy::AlwaysVisible
			&& Policy != EDevKitRVTSurfaceGeometryPolicy::UseAssetTypeDefault)
		{
			bEditEnableCollision = false;
		}
	}
}

ECheckBoxState SDevKitRVTMeshDecalWidget::IsEditFlagChecked(
	bool SDevKitRVTMeshDecalWidget::* Flag) const
{
	return this->*Flag ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SDevKitRVTMeshDecalWidget::OnEditFlagChanged(
	ECheckBoxState NewState,
	bool SDevKitRVTMeshDecalWidget::* Flag)
{
	this->*Flag = NewState == ECheckBoxState::Checked;
}

FText SDevKitRVTMeshDecalWidget::GetStatusText() const
{
	return StatusText;
}

FSlateColor SDevKitRVTMeshDecalWidget::GetStatusColor() const
{
	return bStatusIsError
		? FSlateColor(FLinearColor(1.0f, 0.22f, 0.15f))
		: FSlateColor::UseSubduedForeground();
}

void SDevKitRVTMeshDecalWidget::SetStatus(const FText& InStatus, bool bIsError)
{
	StatusText = InStatus;
	bStatusIsError = bIsError;
}

#undef LOCTEXT_NAMESPACE
