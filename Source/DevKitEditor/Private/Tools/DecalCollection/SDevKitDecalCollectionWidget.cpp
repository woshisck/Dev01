#include "Tools/DecalCollection/SDevKitDecalCollectionWidget.h"

#include "Editor.h"
#include "EditorModeManager.h"
#include "AssetThumbnail.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Components/DecalComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "Factories/DataAssetFactory.h"
#include "ILevelEditor.h"
#include "IAssetTools.h"
#include "InputCoreTypes.h"
#include "LevelEditor.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "SLevelViewport.h"
#include "ScopedTransaction.h"
#include "Surface/DevKitDecalAsset.h"
#include "Surface/DevKitDecalCollectionActor.h"
#include "Tools/DecalCollection/DevKitDecalCollectionEdMode.h"
#include "Selection.h"
#include "Styling/AppStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DevKitDecalCollectionWidget"

namespace
{
	// Keep the authoring surface visually quiet.  The accent is reserved for the
	// active workspace and the current placement asset, so an artist can read
	// the state without decoding a row of default Slate buttons.
	const FLinearColor DecalWorkspaceBackground(0.019f, 0.024f, 0.035f, 1.f);
	const FLinearColor DecalPanelBackground(0.035f, 0.047f, 0.070f, 1.f);
	const FLinearColor DecalActiveAccent(0.075f, 0.360f, 0.650f, 1.f);
	const FLinearColor DecalInactiveTab(0.055f, 0.070f, 0.100f, 1.f);
	const FLinearColor DecalStatusBackground(0.040f, 0.100f, 0.155f, 1.f);

	DECLARE_DELEGATE_RetVal(FReply, FOnPaletteRowDragDetected);
	DECLARE_DELEGATE_RetVal_FourParams(bool, FOnPaletteAssetDragDropped, TWeakObjectPtr<UDevKitDecalAsset>, FLevelEditorViewportClient*, int32, int32);

	/**
	 * A small row wrapper is used instead of making the material picker itself
	 * draggable.  Buttons and the object picker keep their normal Slate input;
	 * dragging from the row background starts a palette-to-viewport placement.
	 */
	class SDevKitDecalPaletteRow final : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SDevKitDecalPaletteRow) {}
			SLATE_EVENT(FSimpleDelegate, OnSelected)
			SLATE_EVENT(FOnPaletteRowDragDetected, OnDragged)
			SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnSelected = InArgs._OnSelected;
			OnDragged = InArgs._OnDragged;
			ChildSlot[InArgs._Content.Widget];
		}

		virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
			{
				OnSelected.ExecuteIfBound();
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}
			return FReply::Unhandled();
		}

		virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
		{
			return OnDragged.IsBound() ? OnDragged.Execute() : FReply::Unhandled();
		}

	private:
		FSimpleDelegate OnSelected;
		FOnPaletteRowDragDetected OnDragged;
	};

	/** Drag operation shared by the Mode palette and the Level Editor viewport. */
	class FDevKitDecalAssetDragDropOp final : public FDecoratedDragDropOp
	{
	public:
		DRAG_DROP_OPERATOR_TYPE(FDevKitDecalAssetDragDropOp, FDecoratedDragDropOp)

		static TSharedRef<FDevKitDecalAssetDragDropOp> New(
			UDevKitDecalAsset* InAsset,
			FOnPaletteAssetDragDropped InOnDropped)
		{
			TSharedRef<FDevKitDecalAssetDragDropOp> Operation = MakeShared<FDevKitDecalAssetDragDropOp>();
			Operation->Asset = InAsset;
			Operation->OnDropped = MoveTemp(InOnDropped);
			Operation->CurrentHoverText = FText::Format(
				LOCTEXT("PaletteDragDecorator", "放置 1 个 {0}"),
				InAsset && !InAsset->DisplayName.IsEmpty()
					? InAsset->DisplayName
					: FText::FromString(InAsset ? InAsset->GetName() : TEXT("贴花资产")));
			Operation->CurrentIconBrush = FAppStyle::GetBrush(TEXT("Icons.Plus"));
			Operation->SetupDefaults();
			Operation->Construct();
			return Operation;
		}

		virtual void OnDrop(bool bDropWasHandled, const FPointerEvent& MouseEvent) override
		{
			bool bHandled = false;
			if (Asset.IsValid())
			{
				FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
				if (const TSharedPtr<ILevelEditor> LevelEditor = LevelEditorModule.GetFirstLevelEditor())
				{
					for (const TSharedPtr<SLevelViewport>& ViewportWidget : LevelEditor->GetViewports())
					{
						if (ViewportWidget.IsValid()
							&& ViewportWidget->GetCachedGeometry().IsUnderLocation(MouseEvent.GetScreenSpacePosition()))
						{
							const FVector2D LocalPosition = ViewportWidget->GetCachedGeometry().AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
							bHandled = OnDropped.Execute(Asset, &ViewportWidget->GetLevelViewportClient(), FMath::RoundToInt(LocalPosition.X), FMath::RoundToInt(LocalPosition.Y));
							break;
						}
					}
				}
			}
			FDecoratedDragDropOp::OnDrop(bDropWasHandled || bHandled, MouseEvent);
		}

	private:
		TWeakObjectPtr<UDevKitDecalAsset> Asset;
		FOnPaletteAssetDragDropped OnDropped;
	};
}

void SDevKitDecalCollectionWidget::Construct(const FArguments& InArgs)
{
	ThumbnailPool = MakeShared<FAssetThumbnailPool>(256);
	RefreshCollections();
	ActionStatus = TEXT("从左侧选择资产后，可拖入视口、在中心单点放置，或开启画笔。选中实例后直接使用 W/E/R；所有改动会回写到当前 Collection。\n");
	ChildSlot
	[
		SNew(SBorder)
		.BorderBackgroundColor(DecalWorkspaceBackground)
		.Padding(12.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 10)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor(DecalPanelBackground)
				.Padding(FMargin(12.f, 10.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Title", "贴花与地表物件"))
						.Font(FAppStyle::GetFontStyle("HeadingExtraSmall"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("Description", "统一管理 RVT 网格贴花、场景网格贴花、地表物件与延迟投射。打开后默认进入放置工作区。"))
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.BorderBackgroundColor(DecalStatusBackground)
				.Padding(FMargin(9.f, 6.f))
				[
					SNew(STextBlock)
					.Text_Lambda([this] { return GetCollectionSummary(); })
					.AutoWrapText(true)
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this] { return ActiveSection == 0 ? DecalActiveAccent : DecalInactiveTab; })
					.ContentPadding(FMargin(9.f, 6.f))
					.Text_Lambda([this] { return ActiveSection == 0 ? LOCTEXT("ManageTabActive", "● 管理") : LOCTEXT("ManageTab", "管理"); })
					.OnClicked_Lambda([this] { return SelectSection(0); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this] { return ActiveSection == 1 ? DecalActiveAccent : DecalInactiveTab; })
					.ContentPadding(FMargin(9.f, 6.f))
					.Text_Lambda([this] { return ActiveSection == 1 ? LOCTEXT("PlaceTabActive", "● 放置") : LOCTEXT("PlaceTab", "放置"); })
					.OnClicked_Lambda([this] { return SelectSection(1); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0, 0, 4, 0)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this] { return ActiveSection == 2 ? DecalActiveAccent : DecalInactiveTab; })
					.ContentPadding(FMargin(9.f, 6.f))
					.Text_Lambda([this] { return ActiveSection == 2 ? LOCTEXT("AssetTabActive", "● 资产") : LOCTEXT("AssetTab", "资产"); })
					.OnClicked_Lambda([this] { return SelectSection(2); })
				]
				+ SHorizontalBox::Slot().FillWidth(1.f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this] { return ActiveSection == 3 ? DecalActiveAccent : DecalInactiveTab; })
					.ContentPadding(FMargin(9.f, 6.f))
					.Text_Lambda([this] { return ActiveSection == 3 ? LOCTEXT("AuditTabActive", "● 审计") : LOCTEXT("AuditTab", "审计"); })
					.OnClicked_Lambda([this] { return SelectSection(3); })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
			[
				SNew(STextBlock)
				.Text_Lambda([this] { return GetSectionHelp(); })
				.AutoWrapText(true)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Padding(FMargin(8.f, 6.f))
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CollectionsTitle", "当前关卡的 Collection"))
						.Font(FAppStyle::GetFontStyle("SmallFontBold"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 5.f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CollectionsHelp", "每个 Level 或逻辑区域使用一个 Collection。选择后点击 Edit，所有编辑均回写 Placement Record。"))
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(CollectionRows, SVerticalBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton).Text(LOCTEXT("Create", "+ 新建 Collection"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::CreateCollection)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton).Text(LOCTEXT("Edit", "Edit 当前 Collection"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::EnterSelectedCollection)
						]
						]
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Padding(FMargin(8.f, 6.f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text_Lambda([this] { return GetSectionTitle(); })
						.Font(FAppStyle::GetFontStyle("SmallFontBold"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 0)
					[
						SNew(STextBlock)
						.Text_Lambda([this] { return GetActionStatus(); })
						.AutoWrapText(true)
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Visibility_Lambda([this] { return ActiveSection == 3 ? EVisibility::Visible : EVisibility::Collapsed; })
				.Padding(FMargin(8.f, 6.f))
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AuditSummaryTitle", "当前关卡审计摘要"))
						.Font(FAppStyle::GetFontStyle("SmallFontBold"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 5.f)
					[
						SNew(STextBlock)
						.Text_Lambda([this] { return GetAuditSummary(); })
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("ValidateCurrentCollection", "校验当前 Collection"))
							.OnClicked_Lambda([this]
							{
								if (ADevKitDecalCollectionActor* Collection = GetTargetCollection())
								{
									const bool bValid = Collection->ValidateCollection();
									ActionStatus = bValid ? TEXT("Collection 校验通过：记录与派生渲染状态有效。") : TEXT("Collection 校验失败：请检查资产定义、RVT 绑定或失效记录。");
								}
								else
								{
									ActionStatus = TEXT("没有可校验的当前 Collection。");
								}
								Invalidate(EInvalidateWidget::LayoutAndVolatility);
								return FReply::Handled();
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AdoptDeferred", "接管选中 Deferred"))
							.ToolTipText(LOCTEXT("AdoptDeferredTip", "将明确选中的旧 Deferred Decal 写入当前 Collection，并仅隐藏原组件；不会自动扫描或删除来源。先在资产库选择一个 Deferred 贴花资产。"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::AdoptSelectedDeferredDecal)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("AdoptISM", "接管选中网格批次"))
							.ToolTipText(LOCTEXT("AdoptISMTip", "将明确选中的单材质、无碰撞、无导航 ISM 组件接管为 Placement Records。来源与资产的阴影状态必须一致；含实例自定义数据或玩法语义的组件会被拒绝，避免数据丢失。"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::AdoptSelectedInstancedMesh)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("RestoreSource", "恢复当前记录来源"))
							.ToolTipText(LOCTEXT("RestoreSourceTip", "恢复当前选中记录原本隐藏的 Deferred 或 ISM 来源，并禁用 Collection 中对应的替代记录。"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::RestoreSelectedRecordSource)
						]
					]
					]
				]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Visibility_Lambda([this] { return ActiveSection == 1 || ActiveSection == 2 ? EVisibility::Visible : EVisibility::Collapsed; })
				.Padding(FMargin(8.f, 6.f))
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PaletteTitle", "贴花资产库 / 放置"))
						.Font(FAppStyle::GetFontStyle("SmallFontBold"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0, 3, 0, 5)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("PaletteHelp", "先选择类型和资产，再拖入视口、单点放置或使用画笔。网格类会按 Mesh+材质合批；延迟贴花保持独立投射，仍可单独移动和调整。"))
						.AutoWrapText(true)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
						.BorderBackgroundColor(FLinearColor(0.025f, 0.034f, 0.052f, 1.f))
						.Padding(FMargin(6.f, 5.f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text_Lambda([this] { return GetSelectedPaletteSummary(); })
								.AutoWrapText(true)
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
								[
									SNew(STextBlock).Text(LOCTEXT("BackendFilterLabel", "类型筛选"))
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("BackendAll", "全部"))
									.ButtonColorAndOpacity_Lambda([this] { return PaletteBackendFilter == -1 ? DecalActiveAccent : DecalInactiveTab; })
									.OnClicked_Lambda([this] { return SelectPaletteBackendFilter(-1); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("PaletteBackendRVTPlane", "RVT 平面"))
									.ButtonColorAndOpacity_Lambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::RVTPlane) ? DecalActiveAccent : DecalInactiveTab; })
									.OnClicked_Lambda([this] { return SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::RVTPlane)); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("PaletteBackendRVTObject", "RVT 物件"))
									.ButtonColorAndOpacity_Lambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::RVTVisibleMesh) ? DecalActiveAccent : DecalInactiveTab; })
									.OnClicked_Lambda([this] { return SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::RVTVisibleMesh)); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("PaletteBackendMesh", "网格贴花"))
									.ButtonColorAndOpacity_Lambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::MeshDecal) ? DecalActiveAccent : DecalInactiveTab; })
									.OnClicked_Lambda([this] { return SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::MeshDecal)); })
								]
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 3.f, 0.f)
								[
									SNew(SButton).Text(LOCTEXT("PaletteBackendOverlay", "地表物件"))
									.ButtonColorAndOpacity_Lambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::StaticMeshOverlay) ? DecalActiveAccent : DecalInactiveTab; })
									.OnClicked_Lambda([this] { return SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::StaticMeshOverlay)); })
								]
								+ SHorizontalBox::Slot().AutoWidth()
								[
									SNew(SButton).Text(LOCTEXT("PaletteBackendDeferred", "延迟投射"))
									.ButtonColorAndOpacity_Lambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::DeferredProjection) ? DecalActiveAccent : DecalInactiveTab; })
									.OnClicked_Lambda([this] { return SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::DeferredProjection)); })
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 5.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
						[
							SNew(STextBlock).Text(LOCTEXT("BrushSpacingLabel", "画笔间距"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SSlider)
							.Value_Lambda([]
							{
								if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
								{
									return (Mode->GetBrushSpacing() - 25.f) / 975.f;
								}
								return (100.f - 25.f) / 975.f;
							})
							.OnValueChanged_Lambda([](float Value)
							{
								if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
								{
									Mode->SetBrushSpacing(FMath::Lerp(25.f, 1000.f, Value));
								}
							})
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text_Lambda([]
							{
								if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
								{
									return FText::Format(LOCTEXT("BrushSpacingValue", "{0} cm"), FText::AsNumber(FMath::RoundToInt(Mode->GetBrushSpacing())));
								}
								return LOCTEXT("BrushSpacingDefault", "100 cm");
							})
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("NewDecalAsset", "+ RVT 贴花"))
							.ToolTipText(LOCTEXT("NewDecalAssetTip", "创建一个新的 RVT Plane 贴花资产，随后选择模型和材质。"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::CreateNewDecalAsset)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("NewModelAsset", "+ 模型 ISM"))
							.ToolTipText(LOCTEXT("NewModelAssetTip", "创建一个新的 RVT Ground Object 资产，并作为独立 ISM 批次加入当前 Collection。"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::CreateNewModelAsset)
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("NewMeshDecalAsset", "+ 网格贴花"))
							.ToolTipText(LOCTEXT("NewMeshDecalAssetTip", "创建基于 Static Mesh 的普通网格贴花资产；它使用 ISM 合批，但不会写入 RVT。"))
							.OnClicked_Lambda([this] { return CreateNewAssetDefinition(EDevKitDecalBackend::MeshDecal); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("NewOverlayAsset", "+ 地表物件"))
							.ToolTipText(LOCTEXT("NewOverlayAssetTip", "创建保留模型的静态地表物件资产；可用于有 Z 轴起伏或遮挡的结构。"))
							.OnClicked_Lambda([this] { return CreateNewAssetDefinition(EDevKitDecalBackend::StaticMeshOverlay); })
						]
						+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("NewDeferredAsset", "+ 延迟贴花"))
							.ToolTipText(LOCTEXT("NewDeferredAssetTip", "创建 Deferred Decal 资产；每条记录保留一个真实的投射代理，不伪装成 ISM 合批。"))
							.OnClicked_Lambda([this] { return CreateNewAssetDefinition(EDevKitDecalBackend::DeferredProjection); })
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
						[
							SNew(SSearchBox)
							.HintText(LOCTEXT("PaletteSearchHint", "搜索模型、材质或贴花资产"))
							.OnTextChanged(this, &SDevKitDecalCollectionWidget::OnPaletteSearchChanged)
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(STextBlock)
							.Text_Lambda([this] { return GetPaletteCountText(); })
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PaletteDisplayLimitLabel", "显示数量"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SSlider)
							.Value_Lambda([this] { return GetPaletteDisplayLimitSliderValue(); })
							.OnValueChanged(this, &SDevKitDecalCollectionWidget::OnPaletteDisplayLimitChanged)
							.ToolTipText(LOCTEXT("PaletteDisplayLimitTip", "限制当前面板生成的资源卡片数量，避免一次性加载过多缩略图。搜索后仍会应用此上限。"))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text_Lambda([this] { return GetPaletteDisplayLimitText(); })
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 8.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("PaletteBrowseLabel", "浏览资源"))
						]
						+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
						[
							SNew(SSlider)
							.Value_Lambda([this] { return GetPaletteBrowseSliderValue(); })
							.OnValueChanged(this, &SDevKitDecalCollectionWidget::OnPaletteBrowseOffsetChanged)
							.ToolTipText(LOCTEXT("PaletteBrowseTip", "在当前显示数量上限内浏览后续资源卡片。"))
						]
						+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text_Lambda([this] { return GetPaletteBrowseRangeText(); })
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SAssignNew(PaletteRows, SWrapBox)
						.UseAllottedSize(true)
						.InnerSlotPadding(FVector2D(4.f, 4.f))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Visibility_Lambda([this] { return ActiveSection == 0 || ActiveSection == 1 ? EVisibility::Visible : EVisibility::Collapsed; })
				.Padding(FMargin(8.f, 6.f))
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectedInstanceTitle", "当前实例 / 直接调整"))
						.Font(FAppStyle::GetFontStyle("SmallFontBold"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 3.f)
					[
						SNew(STextBlock)
						.Text_Lambda([this] { return GetSelectedInstanceSummary(); })
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SObjectPropertyEntryBox)
							.AllowedClass(UMaterialInterface::StaticClass())
							.ObjectPath_Lambda([this] { return GetSelectedInstanceMaterialPath(); })
							.AllowClear(true)
							.DisplayBrowse(true)
							.OnObjectChanged_Lambda([this](const FAssetData& AssetData)
							{
								OnSelectedInstanceMaterialChanged(AssetData);
							})
						]
						+ SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("BakeSelectedInstance", "Bake 为独立批次"))
							.ToolTipText(LOCTEXT("BakeSelectedInstanceTip", "将当前单个 ISM 实例的材质预览固化为新的贴花资产和独立 ISM 批次。W/E/R 调整会立即回写实例记录。"))
							.OnClicked(this, &SDevKitDecalCollectionWidget::BakeSelectedInstance)
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Visibility_Lambda([this] { return ActiveSection == 2 ? EVisibility::Visible : EVisibility::Collapsed; })
				.Padding(FMargin(8.f, 6.f))
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("CompatibilityTitle", "兼容入口"))
						.Font(FAppStyle::GetFontStyle("SmallFontBold"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
					[
						SNew(SButton).Text(LOCTEXT("Legacy", "打开旧版 RVT 地表物件库"))
						.OnClicked(this, &SDevKitDecalCollectionWidget::OpenLegacyRVTLibrary)
					]
				]
			]
	]
	];
	RefreshCollectionRows();
	RefreshPaletteRows();
}

void SDevKitDecalCollectionWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	FGuid CurrentGuid;
	FDevKitDecalPlacementRecord Record;
	if (GEditor && GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		if (const UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
			GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			if (Mode->GetSelectedInstanceDetails(Record))
			{
				CurrentGuid = Record.InstanceGuid;
				SelectedInstanceMaterial = Record.MaterialOverride
					? Record.MaterialOverride
					: (Record.Asset ? (Record.Asset->Material ? Record.Asset->Material : (Record.Asset->LegacyRVTAsset ? Record.Asset->LegacyRVTAsset->Material : nullptr)) : nullptr);
			}
		}
	}

	if (CurrentGuid != LastSelectedInstanceGuid)
	{
		LastSelectedInstanceGuid = CurrentGuid;
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

bool SDevKitDecalCollectionWidget::GetSelectedInstanceRecord(FDevKitDecalPlacementRecord& OutRecord) const
{
	OutRecord = FDevKitDecalPlacementRecord();
	if (!GEditor || !GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		return false;
	}
	if (const UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
		GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
	{
		return Mode->GetSelectedInstanceDetails(OutRecord);
	}
	return false;
}

FText SDevKitDecalCollectionWidget::GetSelectedInstanceSummary() const
{
	FDevKitDecalPlacementRecord Record;
	if (!GetSelectedInstanceRecord(Record) || !Record.Asset)
	{
		return LOCTEXT("NoSelectedInstance", "未选中当前 Collection 的实例。进入 Edit 后点击单个网格，或点击一个 Deferred 投射范围；可用 W/E/R 调整，材质预览只影响当前记录。");
	}

	UStaticMesh* Mesh = Record.Asset->Mesh ? Record.Asset->Mesh : (Record.Asset->LegacyRVTAsset ? Record.Asset->LegacyRVTAsset->Mesh : nullptr);
	UMaterialInterface* Material = Record.MaterialOverride
		? Record.MaterialOverride
		: (Record.Asset->Material ? Record.Asset->Material : (Record.Asset->LegacyRVTAsset ? Record.Asset->LegacyRVTAsset->Material : nullptr));
	FText BackendLabel;
	switch (Record.Asset->Backend)
	{
	case EDevKitDecalBackend::RVTPlane: BackendLabel = LOCTEXT("BackendRVTPlane", "RVT 网格贴花"); break;
	case EDevKitDecalBackend::RVTVisibleMesh: BackendLabel = LOCTEXT("BackendRVTVisible", "RVT 可见物件"); break;
	case EDevKitDecalBackend::MeshDecal: BackendLabel = LOCTEXT("BackendMeshDecal", "场景网格贴花"); break;
	case EDevKitDecalBackend::StaticMeshOverlay: BackendLabel = LOCTEXT("BackendOverlay", "地表静态物件"); break;
	case EDevKitDecalBackend::DeferredProjection: BackendLabel = LOCTEXT("BackendDeferred", "Deferred 投射贴花"); break;
	default: BackendLabel = LOCTEXT("BackendUnknown", "未识别后端"); break;
	}
	return FText::Format(
		LOCTEXT("SelectedInstanceSummary", "实例 {0}\n类型：{1}\n模型：{2}\n当前材质：{3}\n材质预览不会改动其他记录；Mesh 类记录可 Bake 为独立批次，Deferred 保持独立投射代理。"),
		FText::FromString(Record.InstanceGuid.ToString(EGuidFormats::DigitsWithHyphensInBraces)),
		BackendLabel,
		Mesh ? FText::FromString(Mesh->GetName()) : LOCTEXT("SelectedNoMesh", "未配置模型"),
		Material ? FText::FromString(Material->GetName()) : LOCTEXT("SelectedNoMaterial", "未配置材质"));
}

FString SDevKitDecalCollectionWidget::GetSelectedInstanceMaterialPath() const
{
	if (SelectedInstanceMaterial.IsValid())
	{
		return SelectedInstanceMaterial->GetPathName();
	}
	FDevKitDecalPlacementRecord Record;
	if (GetSelectedInstanceRecord(Record) && Record.Asset)
	{
		UMaterialInterface* Material = Record.MaterialOverride
			? Record.MaterialOverride
			: (Record.Asset->Material ? Record.Asset->Material : (Record.Asset->LegacyRVTAsset ? Record.Asset->LegacyRVTAsset->Material : nullptr));
		return Material ? Material->GetPathName() : FString();
	}
	return FString();
}

bool SDevKitDecalCollectionWidget::MatchesPaletteFilter(const UDevKitDecalAsset* Asset) const
{
	if (!Asset)
	{
		return false;
	}

	if (PaletteBackendFilter >= 0 && static_cast<int32>(Asset->Backend) != PaletteBackendFilter)
	{
		return false;
	}

	if (PaletteSearchText.IsEmpty())
	{
		return true;
	}

	const FString Filter = PaletteSearchText.TrimStartAndEnd();
	if (Filter.IsEmpty())
	{
		return true;
	}

	const UStaticMesh* Mesh = Asset->Mesh
		? Asset->Mesh.Get()
		: (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Mesh.Get() : nullptr);
	const UMaterialInterface* Material = Asset->Material
		? Asset->Material.Get()
		: (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Material.Get() : nullptr);

	return Asset->GetName().Contains(Filter, ESearchCase::IgnoreCase)
		|| Asset->DisplayName.ToString().Contains(Filter, ESearchCase::IgnoreCase)
		|| (Mesh && Mesh->GetName().Contains(Filter, ESearchCase::IgnoreCase))
		|| (Material && Material->GetName().Contains(Filter, ESearchCase::IgnoreCase));
}

FText SDevKitDecalCollectionWidget::GetPaletteBackendFilterLabel() const
{
	switch (PaletteBackendFilter)
	{
	case static_cast<int32>(EDevKitDecalBackend::RVTPlane):
		return LOCTEXT("PaletteFilterRVTPlane", "RVT 平面");
	case static_cast<int32>(EDevKitDecalBackend::RVTVisibleMesh):
		return LOCTEXT("PaletteFilterRVTObject", "RVT 物件");
	case static_cast<int32>(EDevKitDecalBackend::MeshDecal):
		return LOCTEXT("PaletteFilterMeshDecal", "网格贴花");
	case static_cast<int32>(EDevKitDecalBackend::StaticMeshOverlay):
		return LOCTEXT("PaletteFilterOverlay", "地表物件");
	case static_cast<int32>(EDevKitDecalBackend::DeferredProjection):
		return LOCTEXT("PaletteFilterDeferred", "延迟投射");
	default:
		return LOCTEXT("PaletteFilterAll", "全部类型");
	}
}

FText SDevKitDecalCollectionWidget::GetSelectedPaletteSummary() const
{
	if (const UDevKitDecalAsset* Asset = SelectedPaletteAsset.Get())
	{
		FText BackendLabel;
		switch (Asset->Backend)
		{
		case EDevKitDecalBackend::RVTPlane: BackendLabel = LOCTEXT("SelectedPaletteRVTPlane", "RVT 平面"); break;
		case EDevKitDecalBackend::RVTVisibleMesh: BackendLabel = LOCTEXT("SelectedPaletteRVTObject", "RVT 物件"); break;
		case EDevKitDecalBackend::MeshDecal: BackendLabel = LOCTEXT("SelectedPaletteMesh", "网格贴花"); break;
		case EDevKitDecalBackend::StaticMeshOverlay: BackendLabel = LOCTEXT("SelectedPaletteOverlay", "地表物件"); break;
		case EDevKitDecalBackend::DeferredProjection: BackendLabel = LOCTEXT("SelectedPaletteDeferred", "延迟投射"); break;
		default: BackendLabel = LOCTEXT("SelectedPaletteUnknown", "未识别"); break;
		}
		return FText::Format(
			LOCTEXT("SelectedPaletteSummary", "当前放置资产：{0}  ·  类型：{1}。拖入视口可按落点放置；画笔用于连续铺设。"),
			Asset->DisplayName.IsEmpty() ? FText::FromString(Asset->GetName()) : Asset->DisplayName,
			BackendLabel);
	}

	return FText::Format(
		LOCTEXT("NoSelectedPaletteSummary", "尚未选择放置资产。当前筛选：{0}。从下方资源卡点击“选择放置”。"),
		GetPaletteBackendFilterLabel());
}

int32 SDevKitDecalCollectionWidget::GetFilteredPaletteCount() const
{
	int32 Count = 0;
	for (const TWeakObjectPtr<UDevKitDecalAsset>& WeakAsset : PaletteAssets)
	{
		if (MatchesPaletteFilter(WeakAsset.Get()))
		{
			++Count;
		}
	}
	return Count;
}

int32 SDevKitDecalCollectionWidget::GetPaletteDisplayLimitUpperBound() const
{
	return FMath::Max(4, PaletteAssets.Num());
}

float SDevKitDecalCollectionWidget::GetPaletteDisplayLimitSliderValue() const
{
	const int32 MinCount = 4;
	const int32 MaxCount = GetPaletteDisplayLimitUpperBound();
	if (MaxCount <= MinCount)
	{
		return 1.f;
	}

	return FMath::Clamp(
		static_cast<float>(PaletteDisplayLimit - MinCount) / static_cast<float>(MaxCount - MinCount),
		0.f,
		1.f);
}

int32 SDevKitDecalCollectionWidget::GetPaletteBrowseOffsetUpperBound() const
{
	const int32 FilteredCount = GetFilteredPaletteCount();
	return FMath::Max(0, FilteredCount - FMath::Min(PaletteDisplayLimit, FilteredCount));
}

float SDevKitDecalCollectionWidget::GetPaletteBrowseSliderValue() const
{
	const int32 MaxOffset = GetPaletteBrowseOffsetUpperBound();
	if (MaxOffset <= 0)
	{
		return 0.f;
	}

	return FMath::Clamp(static_cast<float>(PaletteBrowseOffset) / static_cast<float>(MaxOffset), 0.f, 1.f);
}

FText SDevKitDecalCollectionWidget::GetPaletteBrowseRangeText() const
{
	const int32 FilteredCount = GetFilteredPaletteCount();
	if (FilteredCount <= 0)
	{
		return LOCTEXT("PaletteBrowseEmpty", "0 / 0");
	}

	const int32 DisplayCount = FMath::Min(PaletteDisplayLimit, FilteredCount);
	const int32 MaxOffset = FMath::Max(0, FilteredCount - DisplayCount);
	const int32 Offset = FMath::Clamp(PaletteBrowseOffset, 0, MaxOffset);
	return FText::Format(
		LOCTEXT("PaletteBrowseRange", "{0}–{1} / {2}"),
		FText::AsNumber(Offset + 1),
		FText::AsNumber(Offset + DisplayCount),
		FText::AsNumber(FilteredCount));
}

FText SDevKitDecalCollectionWidget::GetPaletteDisplayLimitText() const
{
	return FText::Format(LOCTEXT("PaletteDisplayLimitValue", "上限 {0}"), FText::AsNumber(PaletteDisplayLimit));
}

FText SDevKitDecalCollectionWidget::GetPaletteCountText() const
{
	const int32 FilteredCount = GetFilteredPaletteCount();
	const int32 DisplayedCount = FMath::Min(PaletteDisplayLimit, FilteredCount);
	return FText::Format(
		LOCTEXT("PaletteCountValue", "{0}/{1}"),
		FText::AsNumber(DisplayedCount),
		FText::AsNumber(FilteredCount));
}

void SDevKitDecalCollectionWidget::OnPaletteSearchChanged(const FText& SearchText)
{
	PaletteSearchText = SearchText.ToString();
	PaletteBrowseOffset = 0;
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

void SDevKitDecalCollectionWidget::OnPaletteDisplayLimitChanged(float SliderValue)
{
	const int32 MinCount = 4;
	const int32 MaxCount = GetPaletteDisplayLimitUpperBound();
	PaletteDisplayLimit = FMath::Clamp(
		FMath::RoundToInt(FMath::Lerp(static_cast<float>(MinCount), static_cast<float>(MaxCount), SliderValue)),
		MinCount,
		MaxCount);
	PaletteBrowseOffset = FMath::Clamp(PaletteBrowseOffset, 0, GetPaletteBrowseOffsetUpperBound());
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

void SDevKitDecalCollectionWidget::OnPaletteBrowseOffsetChanged(float SliderValue)
{
	const int32 MaxOffset = GetPaletteBrowseOffsetUpperBound();
	PaletteBrowseOffset = FMath::Clamp(FMath::RoundToInt(SliderValue * static_cast<float>(MaxOffset)), 0, MaxOffset);
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

void SDevKitDecalCollectionWidget::OnSelectedInstanceMaterialChanged(const FAssetData& AssetData)
{
	UMaterialInterface* NewMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
	if (!NewMaterial && AssetData.IsValid())
	{
		ActionStatus = TEXT("单实例材质预览失败：选择的资源不是材质。 ");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return;
	}

	if (GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
			GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			if (Mode->SetSelectedInstanceMaterialOverride(NewMaterial))
			{
				SelectedInstanceMaterial = NewMaterial;
				ActionStatus = TEXT("已预览当前单个实例的新材质；点击“Bake 为独立批次”固化。 ");
				RefreshPaletteRows();
				Invalidate(EInvalidateWidget::LayoutAndVolatility);
				return;
			}
		}
	}

	ActionStatus = TEXT("单实例材质预览失败：请进入 Edit 并在视口选中一个 ISM 实例。 ");
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

FReply SDevKitDecalCollectionWidget::BakeSelectedInstance()
{
	FDevKitDecalPlacementRecord Record;
	if (!GetSelectedInstanceRecord(Record) || !Record.Asset)
	{
		ActionStatus = TEXT("Bake 失败：请先在视口选中一个 ISM 实例。 ");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}
	if (Record.Asset->Backend == EDevKitDecalBackend::DeferredProjection)
	{
		ActionStatus = TEXT("Deferred 贴花没有 ISM 批次可 Bake；请直接修改当前记录的材质，或在资产页创建独立材质变体。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	UMaterialInterface* Material = SelectedInstanceMaterial.Get();
	if (!Material)
	{
		ActionStatus = TEXT("Bake 失败：当前实例没有可用材质。 ");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	const FString SourcePackage = Record.Asset->GetOutermost() ? Record.Asset->GetOutermost()->GetName() : FString();
	FString PackagePath = FPackageName::GetLongPackagePath(SourcePackage);
	if (PackagePath.IsEmpty())
	{
		PackagePath = TEXT("/Game/Developers/g/DecalCollections");
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(PackagePath / (Record.Asset->GetName() + TEXT("_BakedMaterial")), TEXT(""), UniquePackageName, UniqueAssetName);
	UDevKitDecalAsset* Variant = Cast<UDevKitDecalAsset>(AssetTools.DuplicateAsset(UniqueAssetName, PackagePath, Record.Asset));
	if (!Variant)
	{
		ActionStatus = TEXT("Bake 失败：无法创建独立贴花资产，请检查目录是否可写。 ");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	Variant->Modify();
	Variant->Material = Material;
	Variant->DisplayName = FText::Format(LOCTEXT("BakedInstanceDisplayName", "{0} 单实例 Bake"), Record.Asset->DisplayName.IsEmpty() ? FText::FromString(Record.Asset->GetName()) : Record.Asset->DisplayName);
	Variant->PostEditChange();
	Variant->MarkPackageDirty();

	if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
		GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
	{
		if (Mode->BakeSelectedInstanceAsset(Variant))
		{
			SelectedPaletteAsset = Variant;
			SelectedInstanceMaterial.Reset();
			ActionStatus = FString::Printf(TEXT("已 Bake：%s。当前实例已进入独立 ISM 批次。"), *Variant->GetName());
			RefreshPaletteRows();
			Invalidate(EInvalidateWidget::LayoutAndVolatility);
			return FReply::Handled();
		}
	}

	ActionStatus = TEXT("Bake 失败：当前实例选择已失效。 ");
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

void SDevKitDecalCollectionWidget::RefreshCollections()
{
	Collections.Reset();
	if (!SelectedCollection.IsValid())
	{
		SelectedCollection.Reset();
	}
	if (!GEditor)
	{
		return;
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World)
	{
		return;
	}
	for (TActorIterator<ADevKitDecalCollectionActor> It(World); It; ++It)
	{
		Collections.Add(*It);
	}
	RefreshCollectionRows();
	RefreshPaletteRows();
}

void SDevKitDecalCollectionWidget::RefreshCollectionRows()
{
	if (!CollectionRows.IsValid())
	{
		return;
	}

	CollectionRows->ClearChildren();
	if (Collections.Num() == 0)
	{
		CollectionRows->AddSlot().AutoHeight()
		[
			SNew(STextBlock).Text(LOCTEXT("NoCollections", "当前 Level 没有 Collection；先新建一个再放置贴花。"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
		return;
	}

	ADevKitDecalCollectionActor* ActiveCollection = GetTargetCollection();
	for (const TWeakObjectPtr<ADevKitDecalCollectionActor>& WeakCollection : Collections)
	{
		ADevKitDecalCollectionActor* Collection = WeakCollection.Get();
		if (!Collection)
		{
			continue;
		}

		const FText Label = FText::Format(
			LOCTEXT("CollectionRowLabel", "{0}  ·  {1} 条记录{2}"),
			FText::FromName(Collection->CollectionName),
			FText::AsNumber(Collection->Collection ? Collection->Collection->GetRecordCount() : 0),
			Collection == ActiveCollection ? LOCTEXT("CollectionRowActive", "  当前") : FText::GetEmpty());
		CollectionRows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			SNew(SButton)
			.Text(Label)
			.OnClicked_Lambda([this, WeakCollection]() { return SelectCollection(WeakCollection); })
		];
	}
}

FReply SDevKitDecalCollectionWidget::SelectCollection(TWeakObjectPtr<ADevKitDecalCollectionActor> WeakCollection)
{
	ADevKitDecalCollectionActor* Collection = WeakCollection.Get();
	if (!Collection || !GEditor)
	{
		return FReply::Handled();
	}

	if (GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		if (const UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
			GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			if (Mode->GetActiveCollection() != Collection)
			{
				ActionStatus = TEXT("当前已有一个编辑会话。请先点击底部“应用并退出”或“取消”，再切换到另一个 Collection。");
				Invalidate(EInvalidateWidget::LayoutAndVolatility);
				return FReply::Handled();
			}
		}
	}

	GEditor->SelectNone(false, true);
	GEditor->SelectActor(Collection, true, true);
	GEditor->NoteSelectionChange();
	ActionStatus = FString::Printf(TEXT("已选择 %s。点击 Edit 进入其独占编辑会话。"), *Collection->CollectionName.ToString());
	RefreshPaletteRows();
	RefreshCollectionRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

ADevKitDecalCollectionActor* SDevKitDecalCollectionWidget::GetTargetCollection() const
{
	if (GEditor && GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		if (const UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
			GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			if (ADevKitDecalCollectionActor* ActiveCollection = Mode->GetActiveCollection())
			{
				return ActiveCollection;
			}
		}
	}
	if (SelectedCollection.IsValid())
	{
		return SelectedCollection.Get();
	}

	if (GEditor)
	{
		if (ADevKitDecalCollectionActor* ActorSelectedCollection = Cast<ADevKitDecalCollectionActor>(GEditor->GetSelectedActors()->GetTop<AActor>()))
		{
			return ActorSelectedCollection;
		}
	}

	return Collections.Num() == 1 ? Collections[0].Get() : nullptr;
}

void SDevKitDecalCollectionWidget::RefreshPaletteRows()
{
	PaletteAssets.Reset();
	ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	if (Collection && Collection->Collection)
	{
		for (UDevKitDecalAsset* Asset : Collection->Collection->PaletteAssets)
		{
			if (Asset)
			{
				PaletteAssets.AddUnique(Asset);
			}
		}
		for (const FDevKitDecalPlacementRecord& Record : Collection->Collection->Records)
		{
			if (Record.Asset)
			{
				PaletteAssets.AddUnique(Record.Asset);
			}
		}
	}

	if (!PaletteRows.IsValid())
	{
		return;
	}

	PaletteRows->ClearChildren();
	if (!Collection)
	{
		PaletteRows->AddSlot()
		[
			SNew(STextBlock).Text(LOCTEXT("PaletteNoCollection", "请先选择并 Edit 一个 Collection。"))
		];
		return;
	}
	if (PaletteAssets.Num() == 0)
	{
		PaletteRows->AddSlot()
		[
			SNew(STextBlock).Text(LOCTEXT("PaletteEmpty", "当前 Collection 还没有模型批次。可从兼容库接管，或使用下方资产入口。"))
		];
		return;
	}

	TArray<TWeakObjectPtr<UDevKitDecalAsset>> FilteredPaletteAssets;
	FilteredPaletteAssets.Reserve(PaletteAssets.Num());
	for (const TWeakObjectPtr<UDevKitDecalAsset>& WeakAsset : PaletteAssets)
	{
		if (MatchesPaletteFilter(WeakAsset.Get()))
		{
			FilteredPaletteAssets.Add(WeakAsset);
		}
	}

	if (FilteredPaletteAssets.Num() == 0)
	{
		PaletteRows->AddSlot()
		[
			SNew(STextBlock).Text(LOCTEXT("PaletteNoSearchResult", "没有匹配的模型、材质或贴花资产。"))
		];
		return;
	}

	const int32 DisplayCount = FMath::Min(PaletteDisplayLimit, FilteredPaletteAssets.Num());
	const int32 BrowseMax = FMath::Max(0, FilteredPaletteAssets.Num() - DisplayCount);
	PaletteBrowseOffset = FMath::Clamp(PaletteBrowseOffset, 0, BrowseMax);
	for (int32 AssetIndex = PaletteBrowseOffset; AssetIndex < PaletteBrowseOffset + DisplayCount; ++AssetIndex)
	{
		const TWeakObjectPtr<UDevKitDecalAsset>& WeakAsset = FilteredPaletteAssets[AssetIndex];
		UDevKitDecalAsset* Asset = WeakAsset.Get();
		if (!Asset)
		{
			continue;
		}

		int32 InstanceCount = 0;
		for (const FDevKitDecalPlacementRecord& Record : Collection->Collection->Records)
		{
			InstanceCount += Record.Asset == Asset && Record.bEnabled ? 1 : 0;
		}

		UStaticMesh* PaletteMesh = Asset->Mesh ? Asset->Mesh : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Mesh : nullptr);
		UMaterialInterface* PaletteMaterial = Asset->Material ? Asset->Material : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Material : nullptr);
		FAssetThumbnailConfig ThumbnailConfig;
		ThumbnailConfig.ThumbnailLabel = EThumbnailLabel::NoLabel;
		ThumbnailConfig.bAllowHintText = false;
		ThumbnailConfig.bAllowRealTimeOnHovered = false;
		ThumbnailConfig.ShowAssetBorder = true;
		TSharedRef<SWidget> MeshThumbnailWidget = SNew(STextBlock).Text(LOCTEXT("NoMeshThumbnail", "无模型"));
		TSharedRef<SWidget> MaterialThumbnailWidget = SNew(STextBlock).Text(LOCTEXT("NoMaterialThumbnail", "无材质"));
		if (PaletteMesh && ThumbnailPool.IsValid())
		{
			MeshThumbnailWidget = MakeShared<FAssetThumbnail>(PaletteMesh, 72, 72, ThumbnailPool)->MakeThumbnailWidget(ThumbnailConfig);
		}
		if (PaletteMaterial && ThumbnailPool.IsValid())
		{
			MaterialThumbnailWidget = MakeShared<FAssetThumbnail>(PaletteMaterial, 72, 72, ThumbnailPool)->MakeThumbnailWidget(ThumbnailConfig);
		}

		PaletteRows->AddSlot().Padding(0.f, 0.f, 0.f, 4.f)
		[
			SNew(SBox)
			.WidthOverride(380.f)
			[
			SNew(SDevKitDecalPaletteRow)
			.OnSelected(FSimpleDelegate::CreateLambda([this, WeakAsset]()
			{
				SelectPaletteAsset(WeakAsset);
			}))
			.OnDragged(FOnPaletteRowDragDetected::CreateLambda([this, WeakAsset]()
			{
				return BeginPaletteAssetDrag(WeakAsset);
			}))
			[
				SNew(SBorder)
				.Padding(FMargin(6.f, 4.f))
				[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 5.f, 0.f)
					[
						SNew(SBox).WidthOverride(72.f).HeightOverride(72.f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								MeshThumbnailWidget
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Bottom)
							.Padding(FMargin(2.f))
							[
								SNew(SBorder)
								.Padding(FMargin(4.f, 1.f))
								.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
								[
									SNew(STextBlock)
									.Text(FText::AsNumber(InstanceCount))
									.Justification(ETextJustify::Center)
								]
							]
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 7.f, 0.f)
					[
						SNew(SBox).WidthOverride(72.f).HeightOverride(72.f)
						[
							MaterialThumbnailWidget
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Fill).Padding(4.f, 0.f, 0.f, 0.f)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("PaletteSelect", "选择放置"))
								.OnClicked_Lambda([this, WeakAsset]() { return SelectPaletteAsset(WeakAsset); })
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("PalettePlace", "视口中心放置"))
								.OnClicked_Lambda([this, WeakAsset]() { return PlacePaletteAssetAtViewportCenter(WeakAsset); })
							]
						]
						+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).Padding(0.f, 3.f, 0.f, 0.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("PaletteVariant", "复制材质变体"))
								.OnClicked_Lambda([this, WeakAsset]() { return CreateMaterialVariant(WeakAsset); })
							]
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(SButton)
								.Text(LOCTEXT("PaletteISMVariant", "新增 ISM 批次"))
								.ToolTipText(LOCTEXT("PaletteISMVariantTip", "复制当前模型和材质定义，创建一个可以独立放置/编辑的 ISM 批次。"))
								.OnClicked_Lambda([this, WeakAsset]() { return CreateISMVariant(WeakAsset); })
							]
						]
						+ SVerticalBox::Slot().FillHeight(1.f).VAlign(VAlign_Center).Padding(0.f, 3.f, 0.f, 0.f)
						[
							SNew(SButton)
							.Text_Lambda([this, WeakAsset]
							{
								if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
								{
									return Mode->IsBrushPlacementActiveFor(WeakAsset.Get()) ? LOCTEXT("PaletteBrushActive", "● 画笔放置：开") : LOCTEXT("PaletteBrush", "画笔放置");
								}
								return LOCTEXT("PaletteBrush", "画笔放置");
							})
							.ToolTipText(LOCTEXT("PaletteBrushTip", "开启后在视口用左键拖动，按上方画笔间距放置当前资产；一整笔画可撤销。再次点击关闭。"))
							.OnClicked_Lambda([this, WeakAsset]() { return TogglePaletteBrush(WeakAsset); })
						]
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 5.f, 0.f)
					[
						SNew(STextBlock).Text(LOCTEXT("PaletteMeshLabel", "模型"))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UStaticMesh::StaticClass())
						.ObjectPath_Lambda([WeakAsset]()
						{
							UStaticMesh* Mesh = WeakAsset.IsValid()
								? (WeakAsset->Mesh ? WeakAsset->Mesh : (WeakAsset->LegacyRVTAsset ? WeakAsset->LegacyRVTAsset->Mesh : nullptr))
								: nullptr;
							return Mesh ? Mesh->GetPathName() : FString();
						})
						.AllowClear(true)
						.DisplayBrowse(true)
						.OnObjectChanged_Lambda([this, WeakAsset](const FAssetData& AssetData)
						{
							OnPaletteMeshChanged(AssetData, WeakAsset);
						})
					]
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 5.f, 0.f)
					[
						SNew(STextBlock).Text(LOCTEXT("PaletteMaterialLabel", "材质"))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UMaterialInterface::StaticClass())
						.ObjectPath_Lambda([WeakAsset]()
						{
							UMaterialInterface* Material = WeakAsset.IsValid()
								? (WeakAsset->Material ? WeakAsset->Material : (WeakAsset->LegacyRVTAsset ? WeakAsset->LegacyRVTAsset->Material : nullptr))
								: nullptr;
							return Material
								? Material->GetPathName()
								: FString();
						})
						.AllowClear(true)
						.DisplayBrowse(true)
						.OnObjectChanged_Lambda([this, WeakAsset](const FAssetData& AssetData)
						{
							OnPaletteMaterialChanged(AssetData, WeakAsset);
						})
					]
				]
				]
			]
			]
		];
	}
}

FReply SDevKitDecalCollectionWidget::SelectPaletteAsset(TWeakObjectPtr<UDevKitDecalAsset> Asset)
{
	SelectedPaletteAsset = Asset;
	ActionStatus = Asset.IsValid()
		? FString::Printf(TEXT("已选择批次：%s。可点击“视口中心放置”，或在视口中继续选择实例后使用 W/E/R。"), *Asset->GetName())
		: TEXT("选择的批次已失效。");
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::BeginPaletteAssetDrag(TWeakObjectPtr<UDevKitDecalAsset> Asset)
{
	if (!Asset.IsValid())
	{
		ActionStatus = TEXT("拖放失败：贴花资产已失效。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Unhandled();
	}

	SelectPaletteAsset(Asset);
	return FReply::Handled().BeginDragDrop(FDevKitDecalAssetDragDropOp::New(
		Asset.Get(),
		FOnPaletteAssetDragDropped::CreateSP(this, &SDevKitDecalCollectionWidget::HandlePaletteAssetDrop)));
}

FReply SDevKitDecalCollectionWidget::PlacePaletteAssetAtViewportCenter(TWeakObjectPtr<UDevKitDecalAsset> Asset)
{
	if (!Asset.IsValid())
	{
		return FReply::Handled();
	}
	if (!GEditor || !GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		ActionStatus = TEXT("请先点击 Edit 当前 Collection 进入贴花 Mode。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}
	if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
		GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
	{
		if (Mode->PlaceAssetAtViewportCenter(Asset.Get()))
		{
			SelectedPaletteAsset = Asset;
			ActionStatus = FString::Printf(TEXT("已放置批次：%s。现在可在视口点击新实例并使用 W/E/R。"), *Asset->GetName());
			RefreshPaletteRows();
			Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
		else
		{
			ActionStatus = TEXT("放置失败：没有可用的活动透视视口或有效地面命中。");
			Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::TogglePaletteBrush(TWeakObjectPtr<UDevKitDecalAsset> Asset)
{
	if (!Asset.IsValid() || !GEditor || !GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		ActionStatus = TEXT("请先选择有效资产，并点击 Edit 当前 Collection 进入贴花 Mode。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
		GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
	{
		SelectedPaletteAsset = Asset;
		const bool bEnabled = Mode->ToggleBrushPlacement(Asset.Get());
		ActionStatus = bEnabled
			? FString::Printf(TEXT("已开启 %s 的画笔放置：在视口左键拖动可按上方间距落点；整笔操作可撤销。"), *Asset->GetName())
			: TEXT("已关闭画笔放置；可继续使用拖拽、视口中心放置和单实例 W/E/R。 ");
		RefreshPaletteRows();
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
	return FReply::Handled();
}

bool SDevKitDecalCollectionWidget::HandlePaletteAssetDrop(
	TWeakObjectPtr<UDevKitDecalAsset> Asset,
	FLevelEditorViewportClient* ViewportClient,
	int32 ViewportX,
	int32 ViewportY)
{
	if (!Asset.IsValid() || !GEditor || !GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		ActionStatus = TEXT("拖放失败：请先进入贴花与地表物件 Mode 的 Edit 会话。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return false;
	}

	if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(
		GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
	{
		if (Mode->PlaceAssetAtViewportCursor(Asset.Get(), ViewportClient, ViewportX, ViewportY))
		{
			SelectedPaletteAsset = Asset;
			ActionStatus = FString::Printf(TEXT("已拖放批次：%s。实例已加入当前 Collection，可继续用 W/E/R 调整。"), *Asset->GetName());
			RefreshPaletteRows();
			Invalidate(EInvalidateWidget::LayoutAndVolatility);
			return true;
		}
	}

	ActionStatus = TEXT("拖放失败：鼠标位置没有命中可放置的地面或 Static Mesh 表面。");
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return false;
}

FReply SDevKitDecalCollectionWidget::CreateMaterialVariant(TWeakObjectPtr<UDevKitDecalAsset> SourceAsset)
{
	UDevKitDecalAsset* Source = SourceAsset.Get();
	ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	if (!Source || !Collection || !Collection->Collection)
	{
		return FReply::Handled();
	}

	const FString SourcePackage = Source->GetOutermost() ? Source->GetOutermost()->GetName() : FString();
	FString PackagePath = FPackageName::GetLongPackagePath(SourcePackage);
	if (PackagePath.IsEmpty())
	{
		PackagePath = TEXT("/Game/Developers/g/DecalCollections");
	}

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(
		PackagePath / (Source->GetName() + TEXT("_MaterialVariant")),
		TEXT(""),
		UniquePackageName,
		UniqueAssetName);
	UDevKitDecalAsset* Variant = Cast<UDevKitDecalAsset>(AssetTools.DuplicateAsset(UniqueAssetName, PackagePath, Source));
	if (!Variant)
	{
		ActionStatus = TEXT("复制材质变体失败：源资产或目标目录不可写。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	Variant->Modify();
	Variant->DisplayName = FText::Format(LOCTEXT("MaterialVariantDisplayName", "{0} 材质变体"), Source->DisplayName.IsEmpty() ? FText::FromString(Source->GetName()) : Source->DisplayName);
	Collection->Modify();
	Collection->Collection->AddPaletteAsset(Variant);
	SelectedPaletteAsset = Variant;
	ActionStatus = FString::Printf(TEXT("已创建材质变体：%s。可直接在右侧材质选择器替换材质，再点击“视口中心放置”。"), *Variant->GetName());
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::CreateISMVariant(TWeakObjectPtr<UDevKitDecalAsset> SourceAsset)
{
	FReply Reply = CreateMaterialVariant(SourceAsset);
	if (SourceAsset.IsValid())
	{
		ActionStatus = FString::Printf(TEXT("已新增 ISM 批次：%s。它与原批次共享模型/材质定义副本，可独立拖放和编辑。"), *SourceAsset->GetName());
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
	return Reply;
}

FReply SDevKitDecalCollectionWidget::CreateNewDecalAsset()
{
	return CreateNewAssetDefinition(EDevKitDecalBackend::RVTPlane);
}

FReply SDevKitDecalCollectionWidget::CreateNewModelAsset()
{
	return CreateNewAssetDefinition(EDevKitDecalBackend::RVTVisibleMesh);
}

FReply SDevKitDecalCollectionWidget::CreateNewAssetDefinition(EDevKitDecalBackend Backend)
{
	ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	if (!Collection || !Collection->Collection)
	{
		ActionStatus = TEXT("创建资产失败：请先选择或创建一个 Collection。 ");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	const bool bPlane = Backend == EDevKitDecalBackend::RVTPlane;
	const FString BaseName = Backend == EDevKitDecalBackend::RVTPlane ? TEXT("DA_RVTDecal_New")
		: Backend == EDevKitDecalBackend::RVTVisibleMesh ? TEXT("DA_RVTObject_New")
		: Backend == EDevKitDecalBackend::MeshDecal ? TEXT("DA_MeshDecal_New")
		: Backend == EDevKitDecalBackend::StaticMeshOverlay ? TEXT("DA_SurfaceObject_New")
		: TEXT("DA_DeferredDecal_New");
	const FString PackagePath = TEXT("/Game/Developers/g/DecalCollections");
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	FString UniquePackageName;
	FString UniqueAssetName;
	AssetTools.CreateUniqueAssetName(PackagePath / BaseName, TEXT(""), UniquePackageName, UniqueAssetName);

	UDataAssetFactory* Factory = NewObject<UDataAssetFactory>();
	Factory->DataAssetClass = UDevKitDecalAsset::StaticClass();
	UDevKitDecalAsset* NewAsset = Cast<UDevKitDecalAsset>(AssetTools.CreateAsset(
		UniqueAssetName, PackagePath, UDevKitDecalAsset::StaticClass(), Factory));
	if (!NewAsset)
	{
		ActionStatus = TEXT("创建资产失败：AssetTools 没有返回 UDevKitDecalAsset。 ");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	NewAsset->Modify();
	NewAsset->Backend = Backend;
	NewAsset->DisplayName = FText::FromString(UniqueAssetName);
	switch (Backend)
	{
	case EDevKitDecalBackend::RVTPlane:
		NewAsset->Description = LOCTEXT("NewDecalAssetDescription", "在贴花与地表物件 Mode 中创建的 RVT Plane 贴花资产，请选择 Plane Mesh、Material 和 RVT。 ");
		break;
	case EDevKitDecalBackend::RVTVisibleMesh:
		NewAsset->Description = LOCTEXT("NewModelAssetDescription", "在贴花与地表物件 Mode 中创建的 RVT 模型 ISM 批次，请选择 Mesh、Material 和 RVT。 ");
		break;
	case EDevKitDecalBackend::MeshDecal:
		NewAsset->Description = LOCTEXT("NewMeshDecalAssetDescription", "在贴花与地表物件 Mode 中创建的普通网格贴花资产，请选择 Mesh 和 Material。 ");
		break;
	case EDevKitDecalBackend::StaticMeshOverlay:
		NewAsset->Description = LOCTEXT("NewOverlayAssetDescription", "在贴花与地表物件 Mode 中创建的保留模型地表物件资产，请选择 Mesh 和 Material。 ");
		break;
	case EDevKitDecalBackend::DeferredProjection:
		NewAsset->Description = LOCTEXT("NewDeferredAssetDescription", "在贴花与地表物件 Mode 中创建的 Deferred Decal 资产，请选择 Decal Material 和投射尺寸。 ");
		break;
	default:
		break;
	}
	NewAsset->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewAsset);
	Collection->Modify();
	Collection->Collection->AddPaletteAsset(NewAsset);
	SelectedPaletteAsset = NewAsset;
	if (bPlane)
	{
		ActionStatus = FString::Printf(TEXT("已创建空 RVT 贴花资产：%s。请在卡片中选择模型和材质后，再拖到视口放置。"), *NewAsset->GetName());
	}
	else if (Backend == EDevKitDecalBackend::RVTVisibleMesh)
	{
		ActionStatus = FString::Printf(TEXT("已创建空模型 ISM 资产：%s。请在卡片中选择模型和材质后，再拖到视口放置。"), *NewAsset->GetName());
	}
	else if (Backend == EDevKitDecalBackend::DeferredProjection)
	{
		ActionStatus = FString::Printf(TEXT("已创建空延迟贴花资产：%s。选择 Decal Material 后即可拖到视口投射。"), *NewAsset->GetName());
	}
	else
	{
		ActionStatus = FString::Printf(TEXT("已创建空网格资产：%s。选择模型和材质后即可拖到视口放置。"), *NewAsset->GetName());
	}
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

void SDevKitDecalCollectionWidget::OnPaletteMeshChanged(const FAssetData& AssetData, TWeakObjectPtr<UDevKitDecalAsset> TargetAsset)
{
	UDevKitDecalAsset* Asset = TargetAsset.Get();
	UStaticMesh* NewMesh = Cast<UStaticMesh>(AssetData.GetAsset());
	if (!Asset)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ChangeDecalPaletteMesh", "修改贴花批次模型"));
	Asset->Modify();
	Asset->Mesh = NewMesh;
	Asset->PostEditChange();
	Asset->MarkPackageDirty();
	if (ADevKitDecalCollectionActor* Collection = GetTargetCollection())
	{
		Collection->Modify();
		if (Collection->Collection)
		{
			Collection->Collection->AddPaletteAsset(Asset);
		}
		Collection->RebuildDerivedRendering();
		Collection->MarkPackageDirty();
	}
	ActionStatus = NewMesh
		? FString::Printf(TEXT("批次 %s 的模型已更新；已有实例会使用新模型并重新生成 ISM。"), *Asset->GetName())
		: FString::Printf(TEXT("批次 %s 的模型已清空；重新选择模型后才能放置。"), *Asset->GetName());
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

void SDevKitDecalCollectionWidget::OnPaletteMaterialChanged(const FAssetData& AssetData, TWeakObjectPtr<UDevKitDecalAsset> TargetAsset)
{
	UDevKitDecalAsset* Asset = TargetAsset.Get();
	UMaterialInterface* NewMaterial = Cast<UMaterialInterface>(AssetData.GetAsset());
	if (!Asset)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("ChangeDecalPaletteMaterial", "修改贴花批次材质"));
	Asset->Modify();
	Asset->Material = NewMaterial;
	Asset->PostEditChange();
	Asset->MarkPackageDirty();
	if (ADevKitDecalCollectionActor* Collection = GetTargetCollection())
	{
		Collection->Modify();
		if (Collection->Collection)
		{
			Collection->Collection->AddPaletteAsset(Asset);
		}
		Collection->RebuildDerivedRendering();
		Collection->MarkPackageDirty();
	}
	SelectedPaletteAsset = Asset;
	ActionStatus = FString::Printf(TEXT("批次 %s 的材质已更新；同一批次的实例会一起刷新。需要保留旧材质时请先“复制材质变体”。"), *Asset->GetName());
	RefreshPaletteRows();
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
}

FReply SDevKitDecalCollectionWidget::SelectSection(int32 SectionIndex)
{
	ActiveSection = FMath::Clamp(SectionIndex, 0, 3);
	RefreshCollections();
	RefreshPaletteRows();
	ActionStatus = FString::Printf(TEXT("已切换到“%s”。当前操作仍在 Mode 内执行，不会打开独立窗口。"), *GetSectionTitle().ToString());
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::SelectPaletteBackendFilter(int32 BackendFilter)
{
	PaletteBackendFilter = BackendFilter;
	PaletteBrowseOffset = 0;
	RefreshPaletteRows();
	ActionStatus = FString::Printf(TEXT("已筛选“%s”资产。选择资源卡后可直接放置、拖放或刷地面。"), *GetPaletteBackendFilterLabel().ToString());
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FText SDevKitDecalCollectionWidget::GetSectionTitle() const
{
	switch (ActiveSection)
	{
	case 1:
		return LOCTEXT("PlaceTitle", "放置 / 刷地面 / 单点放置");
	case 2:
		return LOCTEXT("AssetTitle", "资产 / RVT 与材质校验");
	case 3:
		return LOCTEXT("AuditTitle", "审计 / 旧贴花与派生批次");
	default:
		return LOCTEXT("ManageTitle", "管理 / 当前 Collection 与逐实例编辑");
	}
}

FText SDevKitDecalCollectionWidget::GetSectionHelp() const
{
	switch (ActiveSection)
	{
	case 1:
		return LOCTEXT("PlaceHelp", "放置：先选择或新建 Active Collection，再从资产库拖放、刷地面、单点放置或使用拖动预览。没有 Active Collection 时不会创建匿名实例。按钮点击后会切换本页内容。 ");
	case 2:
		return LOCTEXT("AssetHelp", "资产：统一资产记录 Mesh、Material、Backend、Placement Profile、Quality/Platform Representation 和 RVT 校验。现有 RVT Surface Library 只作为兼容资产入口。 ");
	case 3:
		return LOCTEXT("AuditHelp", "审计：检查当前 Level 的 Collection、记录数量、派生批次和遗漏的旧贴花。PCG 可作为刷点/批量生成输入，接管后再逐个编辑。 ");
	default:
		return LOCTEXT("ManageHelp", "管理：Collection 是轻量的 Placement Record 容器；进入 Mode 后点击单个网格即可用 W/E/R 编辑，变换会回写到记录。 ");
	}
}

FText SDevKitDecalCollectionWidget::GetActionStatus() const
{
	return FText::FromString(ActionStatus);
}

FText SDevKitDecalCollectionWidget::GetAuditSummary() const
{
	int32 UnownedDeferredDecals = 0;
	int32 LegacyMeshDecalCandidates = 0;
	if (GEditor)
	{
		if (UWorld* World = GEditor->GetEditorWorldContext().World())
		{
			for (TActorIterator<AActor> It(World); It; ++It)
			{
				AActor* Actor = *It;
				if (!Actor || Actor->IsA<ADevKitDecalCollectionActor>())
				{
					continue;
				}
				TInlineComponentArray<UDecalComponent*> DeferredComponents(Actor);
				for (const UDecalComponent* Component : DeferredComponents)
				{
					if (Component && !Component->ComponentTags.Contains(TEXT("DevKit.DecalCollection.Derived")))
					{
						++UnownedDeferredDecals;
					}
				}
				TInlineComponentArray<UInstancedStaticMeshComponent*> ISMComponents(Actor);
				for (const UInstancedStaticMeshComponent* Component : ISMComponents)
				{
					if (!Component || Component->ComponentTags.Contains(TEXT("DevKit.DecalCollection.Derived")))
					{
						continue;
					}
					const bool bNamedLikeDecal = Component->GetName().Contains(TEXT("Decal"), ESearchCase::IgnoreCase);
					if (bNamedLikeDecal || Component->RuntimeVirtualTextures.Num() > 0)
					{
						++LegacyMeshDecalCandidates;
					}
				}
			}
		}
	}

	const ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	if (!Collection || !Collection->Collection)
	{
		return FText::Format(LOCTEXT("AuditNoTarget", "当前 Level：{0} 个 Collection。未接管候选：Deferred {1}，网格/RVT {2}。请选择一个后可校验记录和派生代理。"),
			FText::AsNumber(Collections.Num()), FText::AsNumber(UnownedDeferredDecals), FText::AsNumber(LegacyMeshDecalCandidates));
	}

	int32 DisabledRecords = 0;
	int32 InvalidRecords = 0;
	for (const FDevKitDecalPlacementRecord& Record : Collection->Collection->Records)
	{
		DisabledRecords += Record.bEnabled ? 0 : 1;
		InvalidRecords += Record.IsValid() && Record.Asset && Record.Asset->IsValidDefinition() ? 0 : 1;
	}
	return FText::Format(
		LOCTEXT("AuditSummary", "Collection：{0}\n记录：{1}（禁用 {2}，无效 {3}）\n派生：ISM 批次 {4}，Deferred 投射 {5}\n未接管候选：Deferred {6}，网格/RVT {7}\n来源记录保持为唯一真值；派生组件可随时重建。"),
		FText::FromName(Collection->CollectionName),
		FText::AsNumber(Collection->Collection->GetRecordCount()),
		FText::AsNumber(DisabledRecords),
		FText::AsNumber(InvalidRecords),
		FText::AsNumber(Collection->DerivedRVTComponents.Num()),
		FText::AsNumber(Collection->DerivedDeferredComponents.Num()),
		FText::AsNumber(UnownedDeferredDecals),
		FText::AsNumber(LegacyMeshDecalCandidates));
}

FText SDevKitDecalCollectionWidget::GetCollectionSummary() const
{
	if (!GEditor)
	{
		return LOCTEXT("NoEditor", "当前没有编辑器世界。");
	}
	const ADevKitDecalCollectionActor* ActiveCollection = nullptr;
	if (GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		if (const UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			ActiveCollection = Mode->GetActiveCollection();
		}
	}
	AActor* Selected = GEditor->GetSelectedActors()->GetTop<AActor>();
	if (!ActiveCollection)
	{
		ActiveCollection = Cast<ADevKitDecalCollectionActor>(Selected);
	}
	if (const ADevKitDecalCollectionActor* Collection = ActiveCollection)
	{
		return FText::Format(LOCTEXT("SelectedSummary", "当前选中：{0} | 记录：{1} | 状态：{2}"), FText::FromName(Collection->CollectionName), FText::AsNumber(Collection->Collection ? Collection->Collection->GetRecordCount() : 0), Collection->bEditSessionActive ? LOCTEXT("Editing", "编辑中") : LOCTEXT("Closed", "已关闭"));
	}
	return FText::Format(LOCTEXT("Summary", "已发现 {0} 个 Collection Actor。请选择一个，或新建当前 Level 的 Default Collection。"), FText::AsNumber(Collections.Num()));
}

FReply SDevKitDecalCollectionWidget::CreateCollection()
{
	if (!GEditor)
	{
		return FReply::Handled();
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World || World->IsGameWorld())
	{
		return FReply::Handled();
	}
	FActorSpawnParameters Params;
	Params.OverrideLevel = World->GetCurrentLevel();
	Params.Name = MakeUniqueObjectName(Params.OverrideLevel, ADevKitDecalCollectionActor::StaticClass(), TEXT("DecalCollection_Default"));
	ADevKitDecalCollectionActor* Collection = World->SpawnActor<ADevKitDecalCollectionActor>(ADevKitDecalCollectionActor::StaticClass(), FTransform::Identity, Params);
	if (Collection)
	{
		Collection->CollectionName = TEXT("Default");
	GEditor->SelectNone(false, true);
	GEditor->SelectActor(Collection, true, true);
	GEditor->NoteSelectionChange();
	SelectedCollection = Collection;
		RefreshCollections();
		ActionStatus = FString::Printf(TEXT("已新建并选中 Collection：%s。点击 Edit 后开始逐实例编辑。"), *Collection->CollectionName.ToString());
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::EnterSelectedCollection()
{
	ADevKitDecalCollectionActor* Collection = nullptr;
	if (GEditor && GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
	{
		if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			Collection = Mode->GetActiveCollection();
		}
	}
	if (!Collection && GEditor)
	{
		Collection = Cast<ADevKitDecalCollectionActor>(GEditor->GetSelectedActors()->GetTop<AActor>());
	}
	if (!Collection && Collections.Num() == 1)
	{
		Collection = Collections[0].Get();
	}

	if (Collection)
	{
		SelectedCollection = Collection;
		UE_LOG(LogTemp, Display, TEXT("DecalCollectionWidget Edit clicked collection=%s"), *Collection->GetPathName());
		UDevKitDecalCollectionEdMode::RequestCollectionForActivation(Collection);
		if (GEditor)
		{
			GEditor->SelectNone(false, true);
			GEditor->SelectActor(Collection, true, true);
			GEditor->NoteSelectionChange();
		}
		GLevelEditorModeTools().ActivateMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection, false);
		UE_LOG(LogTemp, Display, TEXT("DecalCollectionWidget ActivateMode active=%d"),
			GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection) ? 1 : 0);
		if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
		{
			UE_LOG(LogTemp, Display, TEXT("DecalCollectionWidget mode resolved; BeginEditing result=%d"),
				Mode->BeginEditingCollection(Collection) ? 1 : 0);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("DecalCollectionWidget could not resolve active scriptable mode"));
		}
		ActionStatus = FString::Printf(TEXT("编辑会话已激活：%s。请在视口中选取单个实例后使用 W/E/R；重新点击 Edit 会刷新选择与命中代理。"), *Collection->CollectionName.ToString());
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		if (GLevelEditorModeTools().IsModeActive(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection))
		{
			if (TSharedPtr<SDockTab> LegacyTab = FGlobalTabmanager::Get()->FindExistingLiveTab(FTabId(FName(TEXT("DevKitDecalCollection")))))
			{
				LegacyTab->RequestCloseTab();
			}
		}
	}
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::AdoptSelectedDeferredDecal()
{
	ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	AActor* SourceActor = GEditor ? GEditor->GetSelectedActors()->GetTop<AActor>() : nullptr;
	UDecalComponent* SourceComponent = SourceActor ? SourceActor->FindComponentByClass<UDecalComponent>() : nullptr;
	if (!Collection || !Collection->Collection || !SourceActor || SourceActor == Collection || !SourceComponent)
	{
		ActionStatus = TEXT("接管失败：先在管理页选择目标 Collection，再在视口/Outliner 明确选中一个旧 Deferred Decal Actor。不会自动接管扫描结果。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}
	if (SourceActor->GetLevel() != Collection->GetLevel())
	{
		ActionStatus = TEXT("接管失败：来源与目标 Collection 必须在同一个 Level。请为每个子关卡建立独立 Collection。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	UDevKitDecalAsset* DeferredAsset = SelectedPaletteAsset.Get();
	if (!DeferredAsset || DeferredAsset->Backend != EDevKitDecalBackend::DeferredProjection || !DeferredAsset->IsValidDefinition())
	{
		for (const TWeakObjectPtr<UDevKitDecalAsset>& Candidate : PaletteAssets)
		{
			if (UDevKitDecalAsset* CandidateAsset = Candidate.Get(); CandidateAsset
				&& CandidateAsset->Backend == EDevKitDecalBackend::DeferredProjection && CandidateAsset->IsValidDefinition())
			{
				DeferredAsset = CandidateAsset;
				break;
			}
		}
	}
	if (!DeferredAsset)
	{
		ActionStatus = TEXT("接管失败：资产库中需要一个有效的 Deferred 贴花资产。先在“资产”页创建并配置材质，再选择它。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AdoptLegacyDeferredDecal", "接管旧 Deferred 贴花"));
	FGuid AdoptedGuid;
	if (!Collection->AdoptDeferredDecal(SourceComponent, DeferredAsset, AdoptedGuid))
	{
		ActionStatus = TEXT("接管失败：来源组件、材质或 Collection 状态无效；原始组件没有被删除。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	SelectedCollection = Collection;
	SelectedPaletteAsset = DeferredAsset;
	ActionStatus = FString::Printf(TEXT("已接管 %s：原 Deferred 组件已隐藏但未删除；进入 Edit 后可点击替代投射并用 W/E/R 调整。"), *SourceActor->GetName());
	RefreshCollections();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::RestoreSelectedRecordSource()
{
	ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	FDevKitDecalPlacementRecord Record;
	if (!Collection || !GetSelectedInstanceRecord(Record) || !Record.bSourceHiddenForAdoption)
	{
		ActionStatus = TEXT("恢复失败：进入 Edit 后选中一个由“接管选中贴花/网格批次”生成的记录。直接新建的记录没有可恢复来源。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("RestoreAdoptedDecalSource", "恢复接管前来源"));
	if (!Collection->RestoreAdoptedSource(Record.InstanceGuid))
	{
		ActionStatus = TEXT("恢复失败：原始来源不在当前已加载世界，或组件名称已改变。替代记录保持不变。");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	ActionStatus = TEXT("已恢复原始组件，并禁用 Collection 中对应的替代记录；没有删除任何数据。");
	RefreshCollections();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::AdoptSelectedInstancedMesh()
{
	ADevKitDecalCollectionActor* Collection = GetTargetCollection();
	AActor* SourceActor = GEditor ? GEditor->GetSelectedActors()->GetTop<AActor>() : nullptr;
	UInstancedStaticMeshComponent* SourceComponent = SourceActor ? SourceActor->FindComponentByClass<UInstancedStaticMeshComponent>() : nullptr;
	if (!Collection || !Collection->Collection || !SourceActor || SourceActor == Collection || !SourceComponent)
	{
		ActionStatus = TEXT("接管失败：先选择目标 Collection，再明确选中一个旧 ISM 网格批次。不会自动把全关卡普通网格转为贴花。\n");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}
	if (SourceActor->GetLevel() != Collection->GetLevel())
	{
		ActionStatus = TEXT("接管失败：来源与目标 Collection 必须在同一个 Level。\n");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	UDevKitDecalAsset* MeshAsset = SelectedPaletteAsset.Get();
	if (!MeshAsset || MeshAsset->Backend == EDevKitDecalBackend::DeferredProjection || !MeshAsset->IsValidDefinition())
	{
		ActionStatus = TEXT("接管失败：先在资产库选择一个与来源网格相同的有效 RVT/网格贴花/地表物件资产。\n");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AdoptLegacyInstancedMesh", "接管旧网格贴花批次"));
	int32 AdoptedCount = 0;
	if (!Collection->AdoptInstancedMesh(SourceComponent, MeshAsset, AdoptedCount) || AdoptedCount <= 0)
	{
		ActionStatus = TEXT("接管失败：要求相同 Mesh、单材质、无实例自定义数据、无碰撞/导航，且资产与来源阴影状态一致。原始批次没有被删除或隐藏。\n");
		Invalidate(EInvalidateWidget::LayoutAndVolatility);
		return FReply::Handled();
	}

	SelectedCollection = Collection;
	ActionStatus = FString::Printf(TEXT("已接管 %s 的 %d 个实例：原 ISM 批次已隐藏但未删除；可进入 Edit 后逐个 W/E/R。\n"), *SourceActor->GetName(), AdoptedCount);
	RefreshCollections();
	if (GEditor)
	{
		GEditor->RedrawLevelEditingViewports();
	}
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::OpenLegacyRVTLibrary()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("DevKitRVTMeshDecal")));
	ActionStatus = TEXT("已打开兼容 RVT 地表物件库；新建/编辑仍请回到当前 Mode。\n");
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
