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
#include "Styling/SlateStyle.h"
#include "Styling/SlateTypes.h"
#include "Styling/StyleColors.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SSegmentedControl.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SExpandableArea.h"
#include "Widgets/Layout/SHeader.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/SRichTextBlock.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "DevKitDecalCollectionWidget"

struct FDevKitDecalPaletteItem
{
	explicit FDevKitDecalPaletteItem(TWeakObjectPtr<UDevKitDecalAsset> InAsset)
		: Asset(InAsset)
	{
	}

	TWeakObjectPtr<UDevKitDecalAsset> Asset;
};

namespace
{
	FString EscapeDecalRichText(const FString& Value)
	{
		FString Result = Value;
		Result.ReplaceInline(TEXT("&"), TEXT("&amp;"));
		Result.ReplaceInline(TEXT("<"), TEXT("&lt;"));
		Result.ReplaceInline(TEXT(">"), TEXT("&gt;"));
		return Result;
	}

	const FSlateStyleSet& GetDecalRichTextStyle()
	{
		static TSharedPtr<FSlateStyleSet> StyleSet;
		if (!StyleSet.IsValid())
		{
			StyleSet = MakeShared<FSlateStyleSet>(TEXT("DevKitDecalCollectionRichTextStyle"));
			const FTextBlockStyle Base = FAppStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("NormalText"));

			FTextBlockStyle DefaultStyle(Base);
			DefaultStyle.SetColorAndOpacity(FStyleColors::Foreground);
			StyleSet->Set(TEXT("Default"), DefaultStyle);
			StyleSet->Set(TEXT("text"), DefaultStyle);

			FTextBlockStyle TitleStyle(DefaultStyle);
			TitleStyle.SetFont(FAppStyle::GetFontStyle(TEXT("HeadingSmall")));
			TitleStyle.SetColorAndOpacity(FStyleColors::ForegroundHeader);
			StyleSet->Set(TEXT("title"), TitleStyle);

			FTextBlockStyle LabelStyle(DefaultStyle);
			LabelStyle.SetFont(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")));
			LabelStyle.SetColorAndOpacity(FStyleColors::ForegroundHeader);
			StyleSet->Set(TEXT("label"), LabelStyle);

			FTextBlockStyle GoodStyle(DefaultStyle);
			GoodStyle.SetColorAndOpacity(FStyleColors::Success);
			StyleSet->Set(TEXT("good"), GoodStyle);

			FTextBlockStyle WarnStyle(DefaultStyle);
			WarnStyle.SetColorAndOpacity(FStyleColors::Warning);
			StyleSet->Set(TEXT("warn"), WarnStyle);

			FTextBlockStyle BadStyle(DefaultStyle);
			BadStyle.SetColorAndOpacity(FStyleColors::Error);
			StyleSet->Set(TEXT("bad"), BadStyle);

			FTextBlockStyle InfoStyle(DefaultStyle);
			InfoStyle.SetColorAndOpacity(FStyleColors::Secondary);
			StyleSet->Set(TEXT("info"), InfoStyle);

			FTextBlockStyle MutedStyle(DefaultStyle);
			MutedStyle.SetColorAndOpacity(FStyleColors::Secondary);
			StyleSet->Set(TEXT("muted"), MutedStyle);

			FTextBlockStyle KeyStyle(DefaultStyle);
			KeyStyle.SetFont(FAppStyle::GetFontStyle(TEXT("SmallFontBold")));
			KeyStyle.SetColorAndOpacity(FStyleColors::ForegroundHeader);
			StyleSet->Set(TEXT("key"), KeyStyle);
		}
		return *StyleSet;
	}

	const FSliderStyle& GetDecalSlimSliderStyle()
	{
		static const FSliderStyle SliderStyle = []
		{
			FSliderStyle Style = FAppStyle::Get().GetWidgetStyle<FSliderStyle>(TEXT("Slider"));
			FSlateBrush NormalThumb = Style.NormalThumbImage;
			FSlateBrush HoveredThumb = Style.HoveredThumbImage;
			FSlateBrush DisabledThumb = Style.DisabledThumbImage;
			NormalThumb.ImageSize = FVector2D(8.f, 8.f);
			HoveredThumb.ImageSize = FVector2D(10.f, 10.f);
			DisabledThumb.ImageSize = FVector2D(8.f, 8.f);
			Style.SetNormalThumbImage(NormalThumb)
				.SetHoveredThumbImage(HoveredThumb)
				.SetDisabledThumbImage(DisabledThumb)
				.SetBarThickness(2.f);
			return Style;
		}();
		return SliderStyle;
	}

	TSharedRef<SWidget> MakeDecalSectionHeader(const FText& Title, const FText& Description)
	{
		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHeader)
				[
					SNew(STextBlock)
					.Text(Title)
					.Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(2.f, 3.f, 2.f, 0.f)
			[
				SNew(STextBlock)
				.Text(Description)
				.AutoWrapText(true)
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			];
	}

	TSharedRef<SWidget> MakeDecalSquareIconButton(
		const FName IconName,
		const FText& ToolTip,
		FOnClicked OnClicked)
	{
		return SNew(SBox)
			.WidthOverride(36.f)
			.HeightOverride(36.f)
			[
				SNew(SButton)
				.ButtonStyle(FAppStyle::Get(), TEXT("EditorUtilityButton"))
				.ToolTipText(ToolTip)
				.OnClicked(OnClicked)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush(IconName))
					.DesiredSizeOverride(FVector2D(16.f, 16.f))
				]
			];
	}

	TSharedRef<SWidget> MakeDecalFilterChip(
		const FText& Label,
		TAttribute<ECheckBoxState> CheckState,
		FOnCheckStateChanged OnCheckStateChanged,
		const float MinWidth = 76.f)
	{
		return SNew(SBox)
			.MinDesiredWidth(MinWidth)
			[
				SNew(SCheckBox)
				.Style(FAppStyle::Get(), TEXT("FilterBar.BasicFilterButton"))
				.IsChecked(CheckState)
				.OnCheckStateChanged(OnCheckStateChanged)
				.Padding(FMargin(7.f, 3.f))
				[
					SNew(STextBlock)
					.Text(Label)
					.Justification(ETextJustify::Center)
				]
			];
	}

	DECLARE_DELEGATE_RetVal_FourParams(bool, FOnPaletteAssetDragDropped, TWeakObjectPtr<UDevKitDecalAsset>, FLevelEditorViewportClient*, int32, int32);

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
	SelectedPaletteThumbnail = MakeShared<FAssetThumbnail>(static_cast<UObject*>(nullptr), 120, 120, ThumbnailPool);
	FAssetThumbnailConfig SelectedThumbnailConfig;
	SelectedThumbnailConfig.ThumbnailLabel = EThumbnailLabel::NoLabel;
	SelectedThumbnailConfig.bAllowHintText = false;
	SelectedThumbnailConfig.bAllowRealTimeOnHovered = false;
	SelectedThumbnailConfig.ShowAssetBorder = false;
	RefreshCollections();
	const FSlateStyleSet& RichTextStyle = GetDecalRichTextStyle();
	const TSharedRef<SScrollBar> PaletteScrollBar = SNew(SScrollBar)
		.AlwaysShowScrollbar(false)
		.AlwaysShowScrollbarTrack(false)
		.HideWhenNotInUse(true)
		.Orientation(Orient_Vertical)
		.Thickness(FVector2D(6.f, 6.f));
	ActionStatus.Reset();
	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Background")))
		.Padding(FMargin(6.f, 4.f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
				.Padding(FMargin(10.f, 8.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Title", "贴花与地表物件"))
							.Font(FAppStyle::GetFontStyle(TEXT("HeadingSmall")))
							.ColorAndOpacity(FStyleColors::ForegroundHeader)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 8.f, 0.f)
						[
							SNew(STextBlock)
							.Text(LOCTEXT("TitleDescription", "管理 RVT 网格贴花、场景网格贴花、地表物件与延迟投射。"))
							.AutoWrapText(true)
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f)
					[
						MakeDecalSquareIconButton(
							TEXT("Icons.Plus"),
							LOCTEXT("HeaderCreateCollectionTip", "新建当前 Level 的 Collection"),
							FOnClicked::CreateSP(this, &SDevKitDecalCollectionWidget::CreateCollection))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
					[
						MakeDecalSquareIconButton(
							TEXT("Icons.Edit"),
							LOCTEXT("HeaderEditCollectionTip", "编辑当前 Collection"),
							FOnClicked::CreateSP(this, &SDevKitDecalCollectionWidget::EnterSelectedCollection))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(4.f, 0.f, 0.f, 0.f)
					[
						MakeDecalSquareIconButton(
							TEXT("Icons.BrowseContent"),
							LOCTEXT("HeaderLegacyLibraryTip", "打开兼容 RVT 地表物件库"),
							FOnClicked::CreateSP(this, &SDevKitDecalCollectionWidget::OpenLegacyRVTLibrary))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)
			[
				SNew(SSegmentedControl<int32>)
				.Value_Lambda([this] { return ActiveSection; })
				.OnValueChanged_Lambda([this](int32 SectionIndex) { SelectSection(SectionIndex); })
				+ SSegmentedControl<int32>::Slot(0)
				.Text(LOCTEXT("ManageTab", "管理"))
				+ SSegmentedControl<int32>::Slot(1)
				.Text(LOCTEXT("PlaceTab", "放置"))
				+ SSegmentedControl<int32>::Slot(2)
				.Text(LOCTEXT("AssetTab", "资产"))
				+ SSegmentedControl<int32>::Slot(3)
				.Text(LOCTEXT("AuditTab", "审计"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
				.Padding(FMargin(8.f, 6.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 2.f, 7.f, 0.f)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush(TEXT("Icons.Info.Small")))
						.DesiredSizeOverride(FVector2D(14.f, 14.f))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SRichTextBlock)
						.Text_Lambda([this] { return GetSectionHelp(); })
						.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
						.DecoratorStyleSet(&RichTextStyle)
						.AutoWrapText(true)
					]
				]
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
						MakeDecalSectionHeader(
							LOCTEXT("CollectionsTitle", "当前关卡的 Collection"),
							LOCTEXT("CollectionsHelp", "每个 Level 或逻辑区域使用一个 Collection；编辑结果统一回写 Placement Record。"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
					[
						SAssignNew(CollectionRows, SVerticalBox)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 0.f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 4.f, 0.f)
						[
							SNew(SButton)
							.ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
							.Text(LOCTEXT("Edit", "编辑当前 Collection"))
							.HAlign(HAlign_Center)
							.OnClicked(this, &SDevKitDecalCollectionWidget::EnterSelectedCollection)
						]
						+ SHorizontalBox::Slot().FillWidth(1.f)
						[
							SNew(SButton)
							.Text(LOCTEXT("Create", "新建 Collection"))
							.HAlign(HAlign_Center)
							.OnClicked(this, &SDevKitDecalCollectionWidget::CreateCollection)
						]
						]
					]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SBorder)
				.Visibility_Lambda([this] { return ActionStatus.TrimStartAndEnd().IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
				.Padding(FMargin(8.f, 6.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Top).Padding(0.f, 2.f, 7.f, 0.f)
					[
						SNew(SImage)
						.Image(FAppStyle::GetBrush(TEXT("Icons.Edit")))
						.DesiredSizeOverride(FVector2D(14.f, 14.f))
					]
					+ SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SRichTextBlock)
						.Text_Lambda([this] { return GetActionStatus(); })
						.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
						.DecoratorStyleSet(&RichTextStyle)
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
						SNew(SRichTextBlock)
						.Text_Lambda([this] { return GetAuditSummary(); })
						.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
						.DecoratorStyleSet(&RichTextStyle)
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
						MakeDecalSectionHeader(
							LOCTEXT("PaletteTitle", "贴花资产库 / 放置"),
							LOCTEXT("PaletteHelp", "选择类型与状态后，从卡片拖入视口、单点放置或使用画笔。网格类按 Mesh+材质合批；Deferred 保持独立投射。"))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 5.f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
						.Padding(FMargin(6.f, 5.f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
							[
								SNew(SHeader)
								[
									SNew(STextBlock).Text(LOCTEXT("BackendFilterLabel", "类型与状态筛选"))
								]
							]
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SWrapBox)
								.UseAllottedSize(true)
								.InnerSlotPadding(FVector2D(4.f, 4.f))
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("BackendAll", "全部类型"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteBackendFilter == -1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteBackendFilter(-1); }), 88.f)]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("PaletteBackendRVTPlane", "RVT 平面"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::RVTPlane) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::RVTPlane)); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("PaletteBackendRVTObject", "RVT 物件"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::RVTVisibleMesh) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::RVTVisibleMesh)); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("PaletteBackendMesh", "网格贴花"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::MeshDecal) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::MeshDecal)); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("PaletteBackendOverlay", "地表物件"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::StaticMeshOverlay) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::StaticMeshOverlay)); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("PaletteBackendDeferred", "延迟投射"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteBackendFilter == static_cast<int32>(EDevKitDecalBackend::DeferredProjection) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteBackendFilter(static_cast<int32>(EDevKitDecalBackend::DeferredProjection)); }))]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SWrapBox)
								.UseAllottedSize(true)
								.InnerSlotPadding(FVector2D(4.f, 4.f))
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("ValidityAll", "全部状态"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteValidityFilter == -1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteValidityFilter(-1); }), 88.f)]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("ValidityReady", "可放置"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteValidityFilter == 1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteValidityFilter(1); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("ValidityIncomplete", "配置不完整"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteValidityFilter == 0 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteValidityFilter(0); }), 96.f)]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("UsageAll", "全部批次"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteUsageFilter == -1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteUsageFilter(-1); }), 88.f)]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("UsageUsed", "场景使用中"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteUsageFilter == 1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteUsageFilter(1); }), 88.f)]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("UsageEmpty", "空批次"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteUsageFilter == 0 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteUsageFilter(0); }))]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
							[
								SNew(SWrapBox)
								.UseAllottedSize(true)
								.InnerSlotPadding(FVector2D(4.f, 4.f))
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("SortCollection", "库顺序"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteSortMode == 0 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteSortMode(0); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("SortName", "按名称"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteSortMode == 1 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteSortMode(1); }))]
								+ SWrapBox::Slot()[MakeDecalFilterChip(LOCTEXT("SortInstances", "按实例数"), TAttribute<ECheckBoxState>::CreateLambda([this] { return PaletteSortMode == 2 ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; }), FOnCheckStateChanged::CreateLambda([this](ECheckBoxState) { SelectPaletteSortMode(2); }), 88.f)]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(2.f, 5.f, 0.f, 0.f)
							[
								SNew(SRichTextBlock)
								.Text_Lambda([this] { return GetPaletteFilterSummary(); })
								.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
								.DecoratorStyleSet(&RichTextStyle)
								.AutoWrapText(true)
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 3.f)
					[
						SNew(SHeader)
						[
							SNew(STextBlock).Text(LOCTEXT("PlacementSettingsHeader", "放置参数"))
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
							.Style(&GetDecalSlimSliderStyle())
							.IndentHandle(false)
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
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 4.f)
						[
							SNew(SHeader)
							[
								SNew(STextBlock).Text(LOCTEXT("PaletteCreateHeader", "新建与搜索"))
							]
						]
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(SWrapBox)
							.UseAllottedSize(true)
							.InnerSlotPadding(FVector2D(4.f, 4.f))
							+ SWrapBox::Slot()
							[
								SNew(SButton)
								.Text(LOCTEXT("NewDecalAsset", "+ RVT 贴花"))
								.ToolTipText(LOCTEXT("NewDecalAssetTip", "创建一个新的 RVT Plane 贴花资产，随后选择模型和材质。"))
								.OnClicked(this, &SDevKitDecalCollectionWidget::CreateNewDecalAsset)
							]
							+ SWrapBox::Slot()
							[
								SNew(SButton)
								.Text(LOCTEXT("NewModelAsset", "+ 模型 ISM"))
								.ToolTipText(LOCTEXT("NewModelAssetTip", "创建一个新的 RVT Ground Object 资产，并作为独立 ISM 批次加入当前 Collection。"))
								.OnClicked(this, &SDevKitDecalCollectionWidget::CreateNewModelAsset)
							]
							+ SWrapBox::Slot()
							[
								SNew(SButton)
								.Text(LOCTEXT("NewMeshDecalAsset", "+ 网格贴花"))
								.ToolTipText(LOCTEXT("NewMeshDecalAssetTip", "创建基于 Static Mesh 的普通网格贴花资产；它使用 ISM 合批，但不会写入 RVT。"))
								.OnClicked_Lambda([this] { return CreateNewAssetDefinition(EDevKitDecalBackend::MeshDecal); })
							]
							+ SWrapBox::Slot()
							[
								SNew(SButton)
								.Text(LOCTEXT("NewOverlayAsset", "+ 地表物件"))
								.ToolTipText(LOCTEXT("NewOverlayAssetTip", "创建保留模型的静态地表物件资产；可用于有 Z 轴起伏或遮挡的结构。"))
								.OnClicked_Lambda([this] { return CreateNewAssetDefinition(EDevKitDecalBackend::StaticMeshOverlay); })
							]
							+ SWrapBox::Slot()
							[
								SNew(SButton)
								.Text(LOCTEXT("NewDeferredAsset", "+ 延迟贴花"))
								.ToolTipText(LOCTEXT("NewDeferredAssetTip", "创建 Deferred Decal 资产；每条记录保留一个真实的投射代理，不伪装成 ISM 合批。"))
								.OnClicked_Lambda([this] { return CreateNewAssetDefinition(EDevKitDecalBackend::DeferredProjection); })
							]
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 4.f, 0.f, 0.f)
						[
							SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f).Padding(0.f, 0.f, 6.f, 0.f)
							[
								SNew(SSearchBox)
								.HintText(LOCTEXT("PaletteSearchHint", "搜索名称、模型、材质或贴花资产"))
								.OnTextChanged(this, &SDevKitDecalCollectionWidget::OnPaletteSearchChanged)
							]
							+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
							[
								SNew(STextBlock)
								.Text_Lambda([this] { return GetPaletteCountText(); })
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 3.f)
					[
						SNew(SHeader)
						[
							SNew(STextBlock).Text(LOCTEXT("PaletteTileHeader", "资源缩略图"))
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 0.f, 0.f, 6.f)
					[
						SNew(SBox)
						.HeightOverride(248.f)
						[
							SNew(SBorder)
							.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
							.Padding(3.f)
							[
								SNew(SHorizontalBox)
							+ SHorizontalBox::Slot().FillWidth(1.f)
							[
								SNew(SOverlay)
								+ SOverlay::Slot()
								[
									SAssignNew(PaletteTileView, STileView<FDevKitDecalPaletteItemPtr>)
									.ListItemsSource(&FilteredPaletteItems)
									.SelectionMode(ESelectionMode::Single)
									.OnGenerateTile(this, &SDevKitDecalCollectionWidget::GeneratePaletteTile)
									.OnSelectionChanged(this, &SDevKitDecalCollectionWidget::OnPaletteTileSelectionChanged)
									.ItemWidth(116.f)
									.ItemHeight(124.f)
									.ItemAlignment(EListItemAlignment::LeftAligned)
									.Orientation(Orient_Vertical)
									.AllowOverscroll(EAllowOverscroll::No)
									.ExternalScrollbar(PaletteScrollBar)
								]
								+ SOverlay::Slot()
								.HAlign(HAlign_Center)
								.VAlign(VAlign_Center)
								.Padding(FMargin(18.f))
								[
									SNew(STextBlock)
									.Text_Lambda([this] { return PaletteEmptyMessage; })
									.AutoWrapText(true)
									.Justification(ETextJustify::Center)
									.ColorAndOpacity(FSlateColor::UseSubduedForeground())
									.Visibility_Lambda([this] { return FilteredPaletteItems.IsEmpty() ? EVisibility::Visible : EVisibility::Collapsed; })
								]
							]
							+ SHorizontalBox::Slot().AutoWidth().Padding(3.f, 0.f, 0.f, 0.f)
							[
								PaletteScrollBar
							]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
						.Padding(FMargin(7.f, 6.f))
						[
							SNew(SVerticalBox)
							+ SVerticalBox::Slot().AutoHeight()
							[
								SNew(SHeader)
								[
									SNew(STextBlock)
									.Text(LOCTEXT("SelectedPaletteHeader", "已选择资源"))
									.Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 5.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().Padding(0.f, 0.f, 8.f, 0.f)
								[
									SNew(SBox)
									.WidthOverride(120.f)
									.HeightOverride(120.f)
									[
										SNew(SBorder)
										.BorderImage(FAppStyle::GetBrush(TEXT("ContentBrowser.ThumbnailShadow")))
										.Padding(2.f)
										[
											SelectedPaletteThumbnail->MakeThumbnailWidget(SelectedThumbnailConfig)
										]
									]
								]
								+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Top)
								[
									SNew(SRichTextBlock)
									.Text_Lambda([this] { return GetSelectedPaletteSummary(); })
									.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
									.DecoratorStyleSet(&RichTextStyle)
									.AutoWrapText(true)
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 6.f, 0.f, 3.f)
							[
								SNew(SWrapBox)
								.UseAllottedSize(true)
								.InnerSlotPadding(FVector2D(4.f, 4.f))
								+ SWrapBox::Slot()
								[
									SNew(SButton)
									.ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
									.Text(LOCTEXT("SelectedPalettePlace", "视口中心放置"))
									.IsEnabled_Lambda([this] { return SelectedPaletteAsset.IsValid(); })
									.OnClicked_Lambda([this] { return PlacePaletteAssetAtViewportCenter(SelectedPaletteAsset); })
								]
								+ SWrapBox::Slot()
								[
									SNew(SButton)
									.Text(LOCTEXT("SelectedPaletteVariant", "复制材质变体"))
									.IsEnabled_Lambda([this] { return SelectedPaletteAsset.IsValid(); })
									.OnClicked_Lambda([this] { return CreateMaterialVariant(SelectedPaletteAsset); })
								]
								+ SWrapBox::Slot()
								[
									SNew(SButton)
									.Text(LOCTEXT("SelectedPaletteISMVariant", "新增 ISM 批次"))
									.IsEnabled_Lambda([this] { return SelectedPaletteAsset.IsValid(); })
									.OnClicked_Lambda([this] { return CreateISMVariant(SelectedPaletteAsset); })
								]
								+ SWrapBox::Slot()
								[
									SNew(SButton)
									.Text_Lambda([this]
									{
										if (UDevKitDecalCollectionEdMode* Mode = Cast<UDevKitDecalCollectionEdMode>(GLevelEditorModeTools().GetActiveScriptableMode(UDevKitDecalCollectionEdMode::EM_DevKitDecalCollection)))
										{
											return Mode->IsBrushPlacementActiveFor(SelectedPaletteAsset.Get()) ? LOCTEXT("SelectedPaletteBrushActive", "● 画笔放置") : LOCTEXT("SelectedPaletteBrush", "画笔放置");
										}
										return LOCTEXT("SelectedPaletteBrush", "画笔放置");
									})
									.IsEnabled_Lambda([this] { return SelectedPaletteAsset.IsValid(); })
									.OnClicked_Lambda([this] { return TogglePaletteBrush(SelectedPaletteAsset); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
								[
									SNew(STextBlock).Text(LOCTEXT("SelectedPaletteMeshLabel", "模型"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SObjectPropertyEntryBox)
									.AllowedClass(UStaticMesh::StaticClass())
									.ObjectPath_Lambda([this]
									{
										const UDevKitDecalAsset* Asset = SelectedPaletteAsset.Get();
										UStaticMesh* Mesh = Asset ? (Asset->Mesh ? Asset->Mesh : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Mesh : nullptr)) : nullptr;
										return Mesh ? Mesh->GetPathName() : FString();
									})
									.AllowClear(true)
									.DisplayBrowse(true)
									.IsEnabled_Lambda([this] { return SelectedPaletteAsset.IsValid(); })
									.OnObjectChanged_Lambda([this](const FAssetData& AssetData) { OnPaletteMeshChanged(AssetData, SelectedPaletteAsset); })
								]
							]
							+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 3.f, 0.f, 0.f)
							[
								SNew(SHorizontalBox)
								+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.f, 0.f, 6.f, 0.f)
								[
									SNew(STextBlock).Text(LOCTEXT("SelectedPaletteMaterialLabel", "材质"))
								]
								+ SHorizontalBox::Slot().FillWidth(1.f)
								[
									SNew(SObjectPropertyEntryBox)
									.AllowedClass(UMaterialInterface::StaticClass())
									.ObjectPath_Lambda([this]
									{
										const UDevKitDecalAsset* Asset = SelectedPaletteAsset.Get();
										UMaterialInterface* Material = Asset ? (Asset->Material ? Asset->Material : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Material : nullptr)) : nullptr;
										return Material ? Material->GetPathName() : FString();
									})
									.AllowClear(true)
									.DisplayBrowse(true)
									.IsEnabled_Lambda([this] { return SelectedPaletteAsset.IsValid(); })
									.OnObjectChanged_Lambda([this](const FAssetData& AssetData) { OnPaletteMaterialChanged(AssetData, SelectedPaletteAsset); })
								]
							]
						]
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 8, 0, 0)
			[
				SNew(SExpandableArea)
				.Visibility_Lambda([this] { return ActiveSection == 0 || ActiveSection == 1 ? EVisibility::Visible : EVisibility::Collapsed; })
				.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
				.InitiallyCollapsed(false)
				.Padding(FMargin(8.f, 6.f))
				.HeaderContent()
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("SelectedInstanceTitle", "当前实例 / 直接调整"))
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text_Lambda([this]
						{
							FDevKitDecalPlacementRecord Record;
							return GetSelectedInstanceRecord(Record) ? LOCTEXT("SelectedInstanceReady", "1 个实例") : LOCTEXT("SelectedInstanceEmpty", "0 个实例");
						})
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
				.BodyContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 6.f)
					[
						SNew(SRichTextBlock)
						.Text_Lambda([this] { return GetSelectedInstanceSummary(); })
						.TextStyle(&RichTextStyle.GetWidgetStyle<FTextBlockStyle>(TEXT("Default")))
						.DecoratorStyleSet(&RichTextStyle)
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
							.ButtonStyle(FAppStyle::Get(), TEXT("PrimaryButton"))
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
		return LOCTEXT("NoSelectedInstance", "<muted>未选中当前 Collection 的实例。</> 进入 Edit 后点击单个网格或 Deferred 投射范围；使用 <key>W/E/R</> 调整，材质预览只影响当前记录。");
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
	return FText::FromString(FString::Printf(
		TEXT("<label>当前实例</> <muted>%s</>\n<label>类型</> %s  ·  <label>模型</> %s\n<label>材质</> %s\n<muted>材质预览不会改动其他记录；Mesh 类可 Bake 为独立批次，Deferred 保持独立投射代理。</>"),
		*EscapeDecalRichText(Record.InstanceGuid.ToString(EGuidFormats::DigitsWithHyphensInBraces)),
		*EscapeDecalRichText(BackendLabel.ToString()),
		*EscapeDecalRichText(Mesh ? Mesh->GetName() : TEXT("未配置模型")),
		*EscapeDecalRichText(Material ? Material->GetName() : TEXT("未配置材质"))));
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
	if (PaletteValidityFilter >= 0 && (Asset->IsValidDefinition() ? 1 : 0) != PaletteValidityFilter)
	{
		return false;
	}
	if (PaletteUsageFilter >= 0 && (GetPaletteAssetInstanceCount(Asset) > 0 ? 1 : 0) != PaletteUsageFilter)
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

int32 SDevKitDecalCollectionWidget::GetPaletteAssetInstanceCount(const UDevKitDecalAsset* Asset) const
{
	if (!Asset)
	{
		return 0;
	}
	if (const int32* Count = PaletteInstanceCounts.Find(Asset))
	{
		return *Count;
	}
	return 0;
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
		const FString DisplayName = Asset->DisplayName.IsEmpty() ? Asset->GetName() : Asset->DisplayName.ToString();
		return FText::FromString(FString::Printf(
			TEXT("<label>当前放置</> %s  ·  <muted>%s</>\n<key>拖入视口</> 按落点放置；<key>画笔</> 连续铺设；选中实例后使用 <key>W/E/R</>。"),
			*EscapeDecalRichText(DisplayName),
			*EscapeDecalRichText(BackendLabel.ToString())));
	}

	return FText::FromString(FString::Printf(
		TEXT("<warn>尚未选择放置资产。</> 当前类型：<label>%s</>。从下方资源卡选择，或直接拖动卡片到视口。"),
		*EscapeDecalRichText(GetPaletteBackendFilterLabel().ToString())));
}

FText SDevKitDecalCollectionWidget::GetPaletteFilterSummary() const
{
	FString StatusLabel = TEXT("全部状态");
	if (PaletteValidityFilter == 1)
	{
		StatusLabel = TEXT("可放置");
	}
	else if (PaletteValidityFilter == 0)
	{
		StatusLabel = TEXT("配置不完整");
	}

	FString UsageLabel = TEXT("全部批次");
	if (PaletteUsageFilter == 1)
	{
		UsageLabel = TEXT("场景使用中");
	}
	else if (PaletteUsageFilter == 0)
	{
		UsageLabel = TEXT("空批次");
	}

	const TCHAR* SortLabel = PaletteSortMode == 1 ? TEXT("名称") : (PaletteSortMode == 2 ? TEXT("实例数") : TEXT("库顺序"));
	return FText::FromString(FString::Printf(
		TEXT("<label>筛选</> %s  ·  %s  ·  %s  <muted>| 排序：%s | 结果：%d</>"),
		*EscapeDecalRichText(GetPaletteBackendFilterLabel().ToString()),
		*EscapeDecalRichText(StatusLabel),
		*EscapeDecalRichText(UsageLabel),
		SortLabel,
		GetFilteredPaletteCount()));
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

FText SDevKitDecalCollectionWidget::GetPaletteCountText() const
{
	return FText::Format(LOCTEXT("PaletteCountValue", "{0} 个资产"), FText::AsNumber(GetFilteredPaletteCount()));
}

void SDevKitDecalCollectionWidget::OnPaletteSearchChanged(const FText& SearchText)
{
	PaletteSearchText = SearchText.ToString();
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

		const bool bIsActive = Collection == ActiveCollection;
		const FText RecordCount = FText::Format(
			LOCTEXT("CollectionRecordCount", "{0} 条 Placement Record"),
			FText::AsNumber(Collection->Collection ? Collection->Collection->GetRecordCount() : 0));
		CollectionRows->AddSlot().AutoHeight().Padding(0.f, 0.f, 0.f, 3.f)
		[
			SNew(SButton)
			.ButtonStyle(FAppStyle::Get(), TEXT("SimpleButton"))
			.ContentPadding(0.f)
			.OnClicked_Lambda([this, WeakCollection]() { return SelectCollection(WeakCollection); })
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
				.Padding(FMargin(8.f, 6.f))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().FillWidth(1.f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
							.Text(FText::FromName(Collection->CollectionName))
							.Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.f, 2.f, 0.f, 0.f)
						[
							SNew(STextBlock)
							.Text(RecordCount)
							.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						]
					]
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8.f, 0.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(bIsActive ? LOCTEXT("CollectionRowActive", "当前") : FText::GetEmpty())
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
						.ColorAndOpacity(FStyleColors::Primary)
					]
				]
			]
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

TSharedRef<ITableRow> SDevKitDecalCollectionWidget::GeneratePaletteTile(
	FDevKitDecalPaletteItemPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	const TWeakObjectPtr<UDevKitDecalAsset> WeakAsset = Item.IsValid() ? Item->Asset : nullptr;
	UDevKitDecalAsset* Asset = WeakAsset.Get();
	if (!Asset)
	{
		return SNew(STableRow<FDevKitDecalPaletteItemPtr>, OwnerTable)
			[
				SNew(STextBlock).Text(LOCTEXT("InvalidPaletteTile", "失效"))
			];
	}

	UStaticMesh* PaletteMesh = Asset->Mesh ? Asset->Mesh : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Mesh : nullptr);
	UMaterialInterface* PaletteMaterial = Asset->Material ? Asset->Material : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Material : nullptr);
	UObject* PreviewAsset = PaletteMesh;
	if (!PreviewAsset)
	{
		PreviewAsset = PaletteMaterial;
	}

	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.ThumbnailLabel = EThumbnailLabel::NoLabel;
	ThumbnailConfig.bAllowHintText = false;
	ThumbnailConfig.bAllowRealTimeOnHovered = false;
	ThumbnailConfig.ShowAssetBorder = false;

	TSharedRef<SWidget> PrimaryThumbnail = SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush(TEXT("ContentBrowser.AssetTileItem.ThumbnailAreaBackground")))
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PaletteTileNoPreview", "无预览"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	if (PreviewAsset && ThumbnailPool.IsValid())
	{
		PrimaryThumbnail = MakeShared<FAssetThumbnail>(PreviewAsset, 104, 92, ThumbnailPool)->MakeThumbnailWidget(ThumbnailConfig);
	}

	TSharedRef<SWidget> MaterialBadge = SNullWidget::NullWidget;
	if (PaletteMesh && PaletteMaterial && ThumbnailPool.IsValid())
	{
		MaterialBadge = SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush(TEXT("ContentBrowser.AssetTileItem.AssetThumbnailOverlayBorder")))
			.Padding(1.f)
			[
				SNew(SBox)
				.WidthOverride(26.f)
				.HeightOverride(26.f)
				[
					MakeShared<FAssetThumbnail>(PaletteMaterial, 26, 26, ThumbnailPool)->MakeThumbnailWidget(ThumbnailConfig)
				]
			];
	}

	FText BackendBadgeText;
	switch (Asset->Backend)
	{
	case EDevKitDecalBackend::RVTPlane: BackendBadgeText = LOCTEXT("TileBadgeRVT", "RVT"); break;
	case EDevKitDecalBackend::RVTVisibleMesh: BackendBadgeText = LOCTEXT("TileBadgeRVTObject", "物件"); break;
	case EDevKitDecalBackend::MeshDecal: BackendBadgeText = LOCTEXT("TileBadgeMesh", "网格"); break;
	case EDevKitDecalBackend::StaticMeshOverlay: BackendBadgeText = LOCTEXT("TileBadgeOverlay", "地表"); break;
	case EDevKitDecalBackend::DeferredProjection: BackendBadgeText = LOCTEXT("TileBadgeDeferred", "投射"); break;
	default: BackendBadgeText = LOCTEXT("TileBadgeUnknown", "?"); break;
	}

	const int32 InstanceCount = GetPaletteAssetInstanceCount(Asset);
	const FText AssetDisplayName = Asset->DisplayName.IsEmpty() ? FText::FromString(Asset->GetName()) : Asset->DisplayName;
	const FText TileToolTip = FText::Format(
		LOCTEXT("PaletteTileToolTip", "{0}\n类型：{1}\n模型：{2}\n材质：{3}\n启用实例：{4}\n状态：{5}\n\n单击选择；拖入视口放置。"),
		AssetDisplayName,
		BackendBadgeText,
		PaletteMesh ? FText::FromString(PaletteMesh->GetPathName()) : LOCTEXT("TileNoMesh", "未配置"),
		PaletteMaterial ? FText::FromString(PaletteMaterial->GetPathName()) : LOCTEXT("TileNoMaterial", "未配置"),
		FText::AsNumber(InstanceCount),
		Asset->IsValidDefinition() ? LOCTEXT("TileReady", "可放置") : LOCTEXT("TileIncomplete", "配置不完整"));

	return SNew(STableRow<FDevKitDecalPaletteItemPtr>, OwnerTable)
		.Style(FAppStyle::Get(), TEXT("ContentBrowser.AssetListView.ColumnListTableRow"))
		.Padding(1.f)
		.OnDragDetected_Lambda([this, WeakAsset](const FGeometry&, const FPointerEvent& PointerEvent)
		{
			return PointerEvent.IsMouseButtonDown(EKeys::LeftMouseButton) && WeakAsset.IsValid()
				? BeginPaletteAssetDrag(WeakAsset)
				: FReply::Unhandled();
		})
		[
			SNew(SBox)
				.WidthOverride(112.f)
				.HeightOverride(120.f)
				.ToolTipText(TileToolTip)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.f)
					[
						SNew(SBorder)
						.BorderImage(FAppStyle::GetBrush(TEXT("ContentBrowser.ThumbnailShadow")))
						.Padding(2.f)
						[
							SNew(SOverlay)
							+ SOverlay::Slot()
							[
								PrimaryThumbnail
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Top)
							.Padding(4.f)
							[
								SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush(TEXT("ContentBrowser.AssetTileItem.AssetThumbnailOverlayBorder")))
								.Padding(FMargin(4.f, 1.f))
								[
									SNew(STextBlock)
									.Text(BackendBadgeText)
									.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
									.ShadowOffset(FVector2D(1.f, 1.f))
									.ColorAndOpacity(Asset->IsValidDefinition() ? FStyleColors::Foreground : FStyleColors::Warning)
								]
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Top)
							.Padding(4.f)
							[
								MaterialBadge
							]
							+ SOverlay::Slot()
							.HAlign(HAlign_Right)
							.VAlign(VAlign_Bottom)
							.Padding(FMargin(5.f, 5.f))
							[
								SNew(SBorder)
								.BorderImage(FAppStyle::GetBrush(TEXT("ContentBrowser.AssetTileItem.AssetThumbnailOverlayBorder")))
								.Padding(FMargin(4.f, 1.f))
								[
									SNew(STextBlock)
									.Text(FText::AsNumber(InstanceCount))
									.Font(FAppStyle::GetFontStyle(TEXT("SmallFontBold")))
									.ShadowOffset(FVector2D(1.f, 1.f))
									.ColorAndOpacity(FStyleColors::Foreground)
								]
							]
						]
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(3.f, 2.f, 3.f, 1.f)
					[
						SNew(STextBlock)
						.Text(AssetDisplayName)
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
						.OverflowPolicy(ETextOverflowPolicy::Ellipsis)
					]
				]
			];
}

void SDevKitDecalCollectionWidget::OnPaletteTileSelectionChanged(
	FDevKitDecalPaletteItemPtr Item,
	ESelectInfo::Type SelectInfo)
{
	const TWeakObjectPtr<UDevKitDecalAsset> Asset = Item.IsValid()
		? Item->Asset
		: TWeakObjectPtr<UDevKitDecalAsset>();

	// RefreshPaletteRows restores selection with ESelectInfo::Direct. Avoid
	// overwriting a more useful status message when the asset did not change.
	if (SelectedPaletteAsset != Asset)
	{
		SelectPaletteAsset(Asset);
	}
}

void SDevKitDecalCollectionWidget::UpdateSelectedPaletteThumbnail()
{
	UObject* PreviewAsset = nullptr;
	if (const UDevKitDecalAsset* Asset = SelectedPaletteAsset.Get())
	{
		PreviewAsset = Asset->Mesh ? Asset->Mesh.Get() : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Mesh.Get() : nullptr);
		if (!PreviewAsset)
		{
			PreviewAsset = Asset->Material ? Asset->Material.Get() : (Asset->LegacyRVTAsset ? Asset->LegacyRVTAsset->Material.Get() : nullptr);
		}
	}
	if (SelectedPaletteThumbnail.IsValid())
	{
		SelectedPaletteThumbnail->SetAsset(PreviewAsset);
	}
}

void SDevKitDecalCollectionWidget::RefreshPaletteRows()
{
	PaletteAssets.Reset();
	PaletteInstanceCounts.Reset();
	FilteredPaletteItems.Reset();
	PaletteEmptyMessage = FText::GetEmpty();
	UpdateSelectedPaletteThumbnail();

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
				if (Record.bEnabled)
				{
					PaletteInstanceCounts.FindOrAdd(Record.Asset)++;
				}
			}
		}
	}

	if (!Collection)
	{
		PaletteEmptyMessage = LOCTEXT("PaletteNoCollection", "请先选择并 Edit 一个 Collection。");
	}
	else if (PaletteAssets.IsEmpty())
	{
		PaletteEmptyMessage = LOCTEXT("PaletteEmpty", "当前 Collection 还没有贴花资产。请使用上方“新建”按钮，或从兼容库接管。");
	}
	else
	{
		TArray<TWeakObjectPtr<UDevKitDecalAsset>> FilteredAssets;
		FilteredAssets.Reserve(PaletteAssets.Num());
		for (const TWeakObjectPtr<UDevKitDecalAsset>& WeakAsset : PaletteAssets)
		{
			if (MatchesPaletteFilter(WeakAsset.Get()))
			{
				FilteredAssets.Add(WeakAsset);
			}
		}

		if (PaletteSortMode == 1)
		{
			FilteredAssets.StableSort([](const TWeakObjectPtr<UDevKitDecalAsset>& Left, const TWeakObjectPtr<UDevKitDecalAsset>& Right)
			{
				const UDevKitDecalAsset* LeftAsset = Left.Get();
				const UDevKitDecalAsset* RightAsset = Right.Get();
				const FString LeftName = LeftAsset && !LeftAsset->DisplayName.IsEmpty() ? LeftAsset->DisplayName.ToString() : (LeftAsset ? LeftAsset->GetName() : FString());
				const FString RightName = RightAsset && !RightAsset->DisplayName.IsEmpty() ? RightAsset->DisplayName.ToString() : (RightAsset ? RightAsset->GetName() : FString());
				return LeftName.Compare(RightName, ESearchCase::IgnoreCase) < 0;
			});
		}
		else if (PaletteSortMode == 2)
		{
			FilteredAssets.StableSort([this](const TWeakObjectPtr<UDevKitDecalAsset>& Left, const TWeakObjectPtr<UDevKitDecalAsset>& Right)
			{
				return GetPaletteAssetInstanceCount(Left.Get()) > GetPaletteAssetInstanceCount(Right.Get());
			});
		}

		for (const TWeakObjectPtr<UDevKitDecalAsset>& WeakAsset : FilteredAssets)
		{
			FilteredPaletteItems.Add(MakeShared<FDevKitDecalPaletteItem>(WeakAsset));
		}

		if (FilteredPaletteItems.IsEmpty())
		{
			PaletteEmptyMessage = LOCTEXT("PaletteNoSearchResult", "没有匹配当前类型、状态或搜索条件的资产。");
		}
	}

	if (PaletteTileView.IsValid())
	{
		FDevKitDecalPaletteItemPtr SelectedItem;
		for (const FDevKitDecalPaletteItemPtr& Item : FilteredPaletteItems)
		{
			if (Item.IsValid() && Item->Asset == SelectedPaletteAsset)
			{
				SelectedItem = Item;
				break;
			}
		}

		PaletteTileView->RequestListRefresh();
		if (SelectedItem.IsValid())
		{
			PaletteTileView->SetSelection(SelectedItem, ESelectInfo::Direct);
		}
		else
		{
			PaletteTileView->ClearSelection();
		}
	}
}
FReply SDevKitDecalCollectionWidget::SelectPaletteAsset(TWeakObjectPtr<UDevKitDecalAsset> Asset)
{
	SelectedPaletteAsset = Asset;
	UpdateSelectedPaletteThumbnail();
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
	RefreshPaletteRows();
	ActionStatus = FString::Printf(TEXT("已筛选“%s”资产。选择资源卡后可直接放置、拖放或刷地面。"), *GetPaletteBackendFilterLabel().ToString());
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::SelectPaletteValidityFilter(int32 ValidityFilter)
{
	PaletteValidityFilter = ValidityFilter;
	RefreshPaletteRows();
	ActionStatus = ValidityFilter == 1
		? TEXT("已只显示配置完整、可直接放置的资产。")
		: (ValidityFilter == 0 ? TEXT("已只显示配置不完整的资产，可直接补齐模型或材质。") : TEXT("已显示全部资产状态。"));
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::SelectPaletteUsageFilter(int32 UsageFilter)
{
	PaletteUsageFilter = UsageFilter;
	RefreshPaletteRows();
	ActionStatus = UsageFilter == 1
		? TEXT("已只显示当前 Collection 中存在启用实例的批次。")
		: (UsageFilter == 0 ? TEXT("已只显示尚未放置实例的空批次。") : TEXT("已显示全部使用状态。"));
	Invalidate(EInvalidateWidget::LayoutAndVolatility);
	return FReply::Handled();
}

FReply SDevKitDecalCollectionWidget::SelectPaletteSortMode(int32 SortMode)
{
	PaletteSortMode = FMath::Clamp(SortMode, 0, 2);
	RefreshPaletteRows();
	ActionStatus = PaletteSortMode == 1
		? TEXT("资产已按名称排序。")
		: (PaletteSortMode == 2 ? TEXT("资产已按启用实例数量排序。") : TEXT("资产已恢复 Collection 库顺序。"));
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
		return LOCTEXT("PlaceHelp", "<label>放置</> 选择 Active Collection 后，可从资产库拖放、刷地面或单点放置。<warn>没有 Active Collection 时不会创建匿名实例。</>");
	case 2:
		return LOCTEXT("AssetHelp", "<label>资产</> 统一记录 Mesh、Material、Backend、Placement、Quality/Platform Representation 与 RVT 校验；旧 RVT Surface Library 只作为兼容入口。");
	case 3:
		return LOCTEXT("AuditHelp", "<label>审计</> 检查当前 Level 的 Collection、记录、派生批次与遗漏旧贴花。<info>PCG 可作为批量生成输入，接管后仍可逐个编辑。</>");
	default:
		return LOCTEXT("ManageHelp", "<label>管理</> Collection 是轻量 Placement Record 容器；进入 Edit 后点击单个网格，使用 <key>W/E/R</> 编辑，变换会回写到记录。");
	}
}

FText SDevKitDecalCollectionWidget::GetActionStatus() const
{
	FString StatusTag = TEXT("text");
	if (ActionStatus.Contains(TEXT("失败")) || ActionStatus.Contains(TEXT("失效")) || ActionStatus.Contains(TEXT("拒绝")))
	{
		StatusTag = TEXT("bad");
	}
	else if (ActionStatus.Contains(TEXT("没有")) || ActionStatus.Contains(TEXT("未")) || ActionStatus.Contains(TEXT("不完整")))
	{
		StatusTag = TEXT("warn");
	}
	else if (ActionStatus.Contains(TEXT("成功")) || ActionStatus.Contains(TEXT("通过")) || ActionStatus.Contains(TEXT("完成")))
	{
		StatusTag = TEXT("good");
	}
	return FText::FromString(FString::Printf(TEXT("<%s>%s</>"), *StatusTag, *EscapeDecalRichText(ActionStatus.TrimStartAndEnd())));
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
		return FText::FromString(FString::Printf(
			TEXT("<label>当前 Level</> %d 个 Collection  ·  <warn>未接管 Deferred %d</>  ·  <warn>网格/RVT %d</>\n<muted>选择 Collection 后可校验记录和派生代理。</>"),
			Collections.Num(), UnownedDeferredDecals, LegacyMeshDecalCandidates));
	}

	int32 DisabledRecords = 0;
	int32 InvalidRecords = 0;
	for (const FDevKitDecalPlacementRecord& Record : Collection->Collection->Records)
	{
		DisabledRecords += Record.bEnabled ? 0 : 1;
		InvalidRecords += Record.IsValid() && Record.Asset && Record.Asset->IsValidDefinition() ? 0 : 1;
	}
	const TCHAR* IntegrityTag = InvalidRecords > 0 ? TEXT("bad") : (DisabledRecords > 0 ? TEXT("warn") : TEXT("good"));
	return FText::FromString(FString::Printf(
		TEXT("<label>Collection</> %s\n<label>记录</> %d  ·  <%s>禁用 %d / 无效 %d</>\n<label>派生</> ISM %d  ·  Deferred %d\n<label>未接管候选</> Deferred %d  ·  网格/RVT %d\n<muted>Placement Record 保持为唯一真值；派生组件可随时重建。</>"),
		*EscapeDecalRichText(Collection->CollectionName.ToString()),
		Collection->Collection->GetRecordCount(),
		IntegrityTag,
		DisabledRecords,
		InvalidRecords,
		Collection->DerivedRVTComponents.Num(),
		Collection->DerivedDeferredComponents.Num(),
		UnownedDeferredDecals,
		LegacyMeshDecalCandidates));
}

FText SDevKitDecalCollectionWidget::GetCollectionSummary() const
{
	if (!GEditor)
	{
		return LOCTEXT("NoEditor", "<bad>当前没有编辑器世界。</>");
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
		return FText::FromString(FString::Printf(
			TEXT("<label>当前 Collection</> %s  ·  <label>记录</> %d  ·  %s"),
			*EscapeDecalRichText(Collection->CollectionName.ToString()),
			Collection->Collection ? Collection->Collection->GetRecordCount() : 0,
			Collection->bEditSessionActive ? TEXT("<good>编辑中</>") : TEXT("<muted>未进入编辑</>")));
	}
	return FText::FromString(FString::Printf(
		TEXT("<warn>尚未选择 Collection。</> 已发现 <label>%d</> 个；请选择一个，或新建当前 Level 的 Default Collection。"),
		Collections.Num()));
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
