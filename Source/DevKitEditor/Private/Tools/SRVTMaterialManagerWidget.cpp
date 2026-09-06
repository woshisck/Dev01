#include "Tools/SRVTMaterialManagerWidget.h"

#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetThumbnail.h"
#include "AssetToolsModule.h"
#include "Components/PrimitiveComponent.h"
#include "DesktopPlatformModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureDefines.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "IAssetTools.h"
#include "IDesktopPlatform.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialFunctionInstance.h"
#include "Materials/MaterialFunctionMaterialLayer.h"
#include "Materials/MaterialFunctionMaterialLayerBlend.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialLayersFunctions.h"
#include "Misc/MessageDialog.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "SourceControlHelpers.h"
#include "Styling/AppStyle.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "Tools/DevKitArtToolUI.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"

#define LOCTEXT_NAMESPACE "DevKitRVTMaterialManager"

// Kept as a standalone translation unit so Live Coding and normal DevKitEditor builds share the same tool implementation.

namespace
{
	const FString LibraryRoot(TEXT("/Game/Art/Texture/CommonTex"));
	const FString LayerTemplatePath(TEXT("/YogArt_Material/MaterialInstance/RVT_Ground/BaseMI/MLI_BasicMat_Base"));
	const FString BlendTemplatePath(TEXT("/YogArt_Material/MaterialInstance/RVT_Ground/BaseMI/MLBI_BasicMatMask_Base"));
	const FString GroundBaseMaterialPath(TEXT("/YogArt_Material/MaterialInstance/MI_Yog_Ground_RVT_Base"));
	const FString TestMaterialPath(TEXT("/Game/Developers/g/L1_Corridor_01a_RVT_Test/LevelMaterial/MI_L1_Corridor_01a_Ground_RVT_Test"));

	const FLinearColor BaseCardColor(0.035f, 0.16f, 0.34f, 1.0f);
	const FLinearColor VariantCardColor(0.42f, 0.34f, 0.10f, 1.0f);

	DECLARE_DELEGATE_OneParam(FOnRVTMaterialCardSelected, TSharedPtr<FRVTMaterialLibraryEntry>);
	DECLARE_DELEGATE_RetVal_OneParam(FReply, FOnRVTMaterialCardDragged, TSharedPtr<FRVTMaterialLibraryEntry>);
	DECLARE_DELEGATE_RetVal_OneParam(bool, FOnCanAcceptRVTPresetAsset, const FAssetData&);
	DECLARE_DELEGATE_OneParam(FOnRVTPresetAssetDropped, const FAssetData&);

	class SRVTPresetLayerDropTarget : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRVTPresetLayerDropTarget) {}
			SLATE_DEFAULT_SLOT(FArguments, Content)
			SLATE_EVENT(FOnCanAcceptRVTPresetAsset, OnCanAcceptAsset)
			SLATE_EVENT(FOnRVTPresetAssetDropped, OnAssetDropped)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			OnCanAcceptAsset = InArgs._OnCanAcceptAsset;
			OnAssetDropped = InArgs._OnAssetDropped;
			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
				.BorderBackgroundColor_Lambda([this]()
				{
					if (!bDragActive)
					{
						return FSlateColor(FLinearColor(0.045f, 0.05f, 0.06f, 1.0f));
					}
					return FSlateColor(bAcceptsDrop
						? FLinearColor(0.02f, 0.34f, 0.48f, 1.0f)
						: FLinearColor(0.42f, 0.08f, 0.06f, 1.0f));
				})
				[
					InArgs._Content.Widget
				]
			];
		}

		virtual void OnDragEnter(const FGeometry&, const FDragDropEvent& DragDropEvent) override
		{
			FAssetData AssetData;
			bDragActive = true;
			bAcceptsDrop = TryGetAcceptableAsset(DragDropEvent, AssetData);
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		virtual void OnDragLeave(const FDragDropEvent&) override
		{
			bDragActive = false;
			bAcceptsDrop = false;
			Invalidate(EInvalidateWidgetReason::Paint);
		}

		virtual FReply OnDragOver(const FGeometry&, const FDragDropEvent& DragDropEvent) override
		{
			FAssetData AssetData;
			bDragActive = true;
			bAcceptsDrop = TryGetAcceptableAsset(DragDropEvent, AssetData);
			Invalidate(EInvalidateWidgetReason::Paint);
			return bAcceptsDrop ? FReply::Handled() : FReply::Unhandled();
		}

		virtual FReply OnDrop(const FGeometry&, const FDragDropEvent& DragDropEvent) override
		{
			FAssetData AssetData;
			const bool bAccepted = TryGetAcceptableAsset(DragDropEvent, AssetData);
			bDragActive = false;
			bAcceptsDrop = false;
			Invalidate(EInvalidateWidgetReason::Paint);
			if (bAccepted && OnAssetDropped.IsBound())
			{
				OnAssetDropped.Execute(AssetData);
				return FReply::Handled();
			}
			return FReply::Unhandled();
		}

	private:
		bool TryGetAcceptableAsset(const FDragDropEvent& DragDropEvent, FAssetData& OutAssetData) const
		{
			const TSharedPtr<FAssetDragDropOp> AssetOperation = DragDropEvent.GetOperationAs<FAssetDragDropOp>();
			if (!AssetOperation.IsValid() || AssetOperation->GetAssets().Num() != 1)
			{
				return false;
			}

			OutAssetData = AssetOperation->GetAssets()[0];
			return OnCanAcceptAsset.IsBound() && OnCanAcceptAsset.Execute(OutAssetData);
		}

		FOnCanAcceptRVTPresetAsset OnCanAcceptAsset;
		FOnRVTPresetAssetDropped OnAssetDropped;
		bool bDragActive = false;
		bool bAcceptsDrop = false;
	};

	class SRVTMaterialShelfCard : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SRVTMaterialShelfCard) {}
			SLATE_ARGUMENT(TSharedPtr<FRVTMaterialLibraryEntry>, Entry)
			SLATE_EVENT(FOnRVTMaterialCardSelected, OnSelected)
			SLATE_EVENT(FOnRVTMaterialCardDragged, OnDragged)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			Entry = InArgs._Entry;
			OnSelected = InArgs._OnSelected;
			OnDragged = InArgs._OnDragged;
			const bool bHighlighted = Entry.IsValid() && (Entry->bIsVariant || Entry->Variants.Num() > 0);

			ChildSlot
			[
				SNew(SBorder)
				.Padding(5.0f)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
				.BorderBackgroundColor(bHighlighted ? VariantCardColor : BaseCardColor)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						Entry.IsValid() && Entry->Thumbnail.IsValid()
							? Entry->Thumbnail->MakeThumbnailWidget()
							: SNullWidget::NullWidget
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, 5.0f, 1.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(Entry.IsValid() ? FText::FromString(Entry->AssetName) : FText::GetEmpty())
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
						.AutoWrapText(true)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, 2.0f, 1.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(Entry.IsValid()
							? FText::Format(LOCTEXT("CardMeta", "{0} · {1} 个变体"), FText::FromString(Entry->Category), FText::AsNumber(Entry->Variants.Num()))
							: FText::GetEmpty())
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
					]
				]
			];
		}

		virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& MouseEvent) override
		{
			if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && Entry.IsValid())
			{
				OnSelected.ExecuteIfBound(Entry);
				return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
			}
			return FReply::Unhandled();
		}

		virtual FReply OnDragDetected(const FGeometry&, const FPointerEvent&) override
		{
			return Entry.IsValid() && OnDragged.IsBound() ? OnDragged.Execute(Entry) : FReply::Unhandled();
		}

	private:
		TSharedPtr<FRVTMaterialLibraryEntry> Entry;
		FOnRVTMaterialCardSelected OnSelected;
		FOnRVTMaterialCardDragged OnDragged;
	};

	FString GetParameterTexturePath(UMaterialFunctionMaterialLayerInstance* Layer, const FName ParameterName)
	{
		if (UTexture* Texture = SRVTMaterialManagerWidget::ResolveTextureParameter(Layer, ParameterName))
		{
			return Texture->GetPathName();
		}
		return TEXT("未设置");
	}
}

void SRVTMaterialManagerWidget::Construct(const FArguments& InArgs)
{
	ThumbnailPool = MakeShared<FAssetThumbnailPool>(256);
	PresetLayerPaths = {
		FSoftObjectPath(TEXT("/Game/Art/Texture/CommonTex/Brick/MLI_CommonBrick_Floor_01.MLI_CommonBrick_Floor_01")),
		FSoftObjectPath(TEXT("/Game/Art/Texture/CommonTex/Brick/MLI_CommonBrick_Floor_02.MLI_CommonBrick_Floor_02")),
		FSoftObjectPath(TEXT("/Game/Art/Texture/CommonTex/Brick/MLI_CommonBrick_Floor_03.MLI_CommonBrick_Floor_03")),
		FSoftObjectPath(TEXT("/Game/Art/Texture/CommonTex/Ground/MLI_CommonGround_Floor_01.MLI_CommonGround_Floor_01"))
	};
	StatusColor = FSlateColor::UseForeground();

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 6.0f, 8.0f, 5.0f)
		[
			SNew(SBorder)
			.Padding(FMargin(5.0f, 3.0f))
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 5.0f, 0.0f)
				[
					SNew(SButton).Text(LOCTEXT("LibraryMode", "材质库")).OnClicked(this, &SRVTMaterialManagerWidget::ShowLibraryView)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(LOCTEXT("PresetMode", "层材质预设")).OnClicked(this, &SRVTMaterialManagerWidget::ShowPresetView)
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 2.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("InteractionHint", "双击打开  ·  Ctrl+B 定位  ·  拖拽到 Material Layers"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.0f).Padding(8.0f, 0.0f, 8.0f, 5.0f)
		[
			SAssignNew(ViewSwitcher, SWidgetSwitcher)
			.WidgetIndex(0)
			+ SWidgetSwitcher::Slot()[BuildLibraryView()]
			+ SWidgetSwitcher::Slot()[BuildPresetView()]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(12.0f, 0.0f, 12.0f, 12.0f)
		[
			SNew(SBorder)
			.Padding(8.0f)
			.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
			[
				SAssignNew(StatusTextBlock, STextBlock)
				.Text(LOCTEXT("Ready", "就绪。材质库来源：/Game/Art/Texture/CommonTex"))
				.ColorAndOpacity_Lambda([this]() { return StatusColor; })
				.AutoWrapText(true)
			]
		]
	];

	RebuildPresetLayerRows();
	ScanLibrary();
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildLibraryView()
{
	return SNew(SVerticalBox)
	+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)[BuildLibraryToolbar()]
	+ SVerticalBox::Slot().FillHeight(1.0f)
	[
		SNew(SSplitter)
		+ SSplitter::Slot().Value(0.58f)[BuildLibraryShelf()]
		+ SSplitter::Slot().Value(0.42f)[BuildDetailPanel()]
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildLibraryToolbar()
{
	return SNew(SBorder)
	.Padding(7.0f)
	.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SButton).Text(LOCTEXT("Refresh", "刷新材质库")).OnClicked(this, &SRVTMaterialManagerWidget::RefreshLibrary)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
		[
			SNew(SButton).Text(LOCTEXT("Import", "导入/更新贴图并生成 MLI")).OnClicked(this, &SRVTMaterialManagerWidget::ImportOrUpdateTextures)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("CategoryLabel", "导入分类"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 8.0f, 0.0f)
		[
			SAssignNew(CategoryTextBox, SEditableTextBox).Text(FText::FromString(TEXT("Brick"))).MinDesiredWidth(90.0f)
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 4.0f, 0.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(LOCTEXT("TargetNameLabel", "MLI 名称"))
		]
		+ SHorizontalBox::Slot().FillWidth(1.0f)
		[
			SAssignNew(TargetLayerNameTextBox, SEditableTextBox)
			.HintText(LOCTEXT("TargetNameHint", "留空：更新当前选择；没有选择时按贴图名推断"))
		]
		+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f).VAlign(VAlign_Center)
		[
			SNew(STextBlock).Text(this, &SRVTMaterialManagerWidget::GetSummaryText)
		]
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildLibraryShelf()
{
	return SNew(SBorder)
	.Padding(8.0f)
	.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("LibraryClassificationTitle", "材质分类"))
				.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 8.0f, 0.0f, 6.0f)
		[
			SNew(SSearchBox)
				.HintText(LOCTEXT("LibrarySearchHint", "搜索 MLI、分类或变体"))
				.OnTextChanged(this, &SRVTMaterialManagerWidget::OnLibrarySearchTextChanged)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 8.0f)
		[
			BuildLibraryCategoryFilters()
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SAssignNew(LibraryTreeView, STreeView<FTreeNodePtr>)
				.TreeItemsSource(&LibraryTreeRootNodes)
				.SelectionMode(ESelectionMode::Single)
				.OnGenerateRow(this, &SRVTMaterialManagerWidget::GenerateLibraryTreeRow)
				.OnGetChildren(this, &SRVTMaterialManagerWidget::GetLibraryTreeChildren)
				.OnSelectionChanged(this, &SRVTMaterialManagerWidget::OnLibraryTreeSelectionChanged)
				.OnMouseButtonDoubleClick(this, &SRVTMaterialManagerWidget::OnLibraryTreeDoubleClicked)
				.OnKeyDownHandler(this, &SRVTMaterialManagerWidget::HandleMaterialLibraryKeyDown)
		]
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildLibraryCategoryFilters()
{
	return SAssignNew(LibraryCategoryWrapBox, SWrapBox)
		.UseAllottedSize(true)
		.InnerSlotPadding(FVector2D(4.0f, 4.0f));
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildLibraryCategoryFilterButton(const FString& Category)
{
	return SNew(SCheckBox)
		.Style(FAppStyle::Get(), TEXT("ToggleButtonCheckbox"))
		.IsChecked(this, &SRVTMaterialManagerWidget::IsLibraryCategoryFilterChecked, Category)
		.OnCheckStateChanged(this, &SRVTMaterialManagerWidget::OnLibraryCategoryFilterChanged, Category)
		.ToolTipText(FText::Format(LOCTEXT("CategoryFilterTooltip", "显示或隐藏 {0} 分类。"), FText::FromString(Category)))
		[
			SNew(STextBlock).Text(FText::FromString(Category))
		];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildDetailPanel()
{
	return SNew(SBorder)
	.Padding(8.0f)
	.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
	[
		SAssignNew(DetailContentBox, SBox)[BuildSelectedDetailContent()]
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildSelectedDetailContent()
{
	if (!SelectedEntry.IsValid())
	{
		return SNew(STextBlock)
		.Text(LOCTEXT("NoSelection", "从左侧分类列表选择一个 MLI。可直接拖到 Material Layers；双击打开，Ctrl+B 在 Content Browser 中定位。"))
		.AutoWrapText(true)
		.ColorAndOpacity(FSlateColor::UseSubduedForeground());
	}

	return SNew(SScrollBox)
	+ SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock).Text(this, &SRVTMaterialManagerWidget::GetSelectedTitle).Font(FAppStyle::GetFontStyle(TEXT("DetailsView.CategoryFontStyle")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
		[
			SNew(STextBlock).Text(this, &SRVTMaterialManagerWidget::GetSelectedKindText).ColorAndOpacity(FSlateColor::UseSubduedForeground())
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
		[
			SNew(STextBlock).Text(this, &SRVTMaterialManagerWidget::GetSelectedPath).AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
		[
			BuildTextureResourceRow(
				FName(TEXT("T BaseColor")),
				LOCTEXT("BaseColorTextureLabel", "Base Color"),
				LOCTEXT("BaseColorTextureDescription", "颜色 · sRGB"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f)
		[
			BuildTextureResourceRow(
				FName(TEXT("T MRAH")),
				LOCTEXT("MRAHTextureLabel", "MRAH"),
				LOCTEXT("MRAHTextureDescription", "Metallic · Roughness · AO · Height"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 8.0f)
		[
			BuildTextureResourceRow(
				FName(TEXT("T Normal")),
				LOCTEXT("NormalLightTextureLabel", "Normal Light"),
				LOCTEXT("NormalLightTextureDescription", "RG 法线 · B LightMask"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("OpenSelected", "打开 MLI")).OnClicked(this, &SRVTMaterialManagerWidget::OpenSelectedAsset)
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(VariantSuffixTextBox, SEditableTextBox).Text(FText::FromString(TEXT("Damage"))).HintText(LOCTEXT("VariantSuffixHint", "Damage 或 a"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("CreateVariant", "创建变体")).OnClicked(this, &SRVTMaterialManagerWidget::CreateVariant)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 5.0f)
		[
			SNew(STextBlock).Text(LOCTEXT("Variants", "同材质变体（浅黄色）")).Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(VariantView, STileView<FEntryPtr>)
			.ListItemsSource(&CurrentVariants)
			.ItemWidth(132.0f)
			.ItemHeight(164.0f)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateTile(this, &SRVTMaterialManagerWidget::GenerateVariantTile)
			.OnMouseButtonDoubleClick(this, &SRVTMaterialManagerWidget::OnVariantDoubleClicked)
		]
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildTextureResourceRow(
	const FName ParameterName,
	const FText& DisplayName,
	const FText& Description)
{
	return SNew(SBorder)
		.Padding(FMargin(9.0f, 6.0f))
		.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.42f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
						.Text(DisplayName)
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(FText::FromName(ParameterName))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
						.Text(Description)
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
						.AutoWrapText(true)
				]
			]
			+ SHorizontalBox::Slot().FillWidth(0.58f).VAlign(VAlign_Center).Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SObjectPropertyEntryBox)
					.AllowedClass(UTexture2D::StaticClass())
					.ObjectPath_Lambda([this, ParameterName]() { return GetSelectedTextureObjectPath(ParameterName); })
					.AllowClear(false)
					.DisplayUseSelected(false)
					.DisplayBrowse(true)
					.EnableContentPicker(false)
					.DisplayThumbnail(true)
					.ThumbnailPool(ThumbnailPool)
			]
		];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildPresetPalette()
{
	return SNew(SBorder)
	.Padding(8.0f)
	.BorderImage(FAppStyle::GetBrush(TEXT("ToolPanel.GroupBorder")))
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PresetPaletteTitle", "可用 RVT 层材质"))
			.Font(FAppStyle::GetFontStyle(TEXT("HeadingExtraSmall")))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 7.0f)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PresetPaletteHint", "抓住材质球拖到右侧槽位；基础材质与变体都可以直接使用。"))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			.AutoWrapText(true)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 5.0f)
		[
			SNew(SSearchBox)
			.HintText(LOCTEXT("PresetPaletteSearchHint", "搜索名称或分类"))
			.OnTextChanged(this, &SRVTMaterialManagerWidget::OnPresetPaletteSearchTextChanged)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(1.0f, 0.0f, 1.0f, 6.0f)
		[
			SNew(STextBlock)
			.Text(this, &SRVTMaterialManagerWidget::GetPresetPaletteSummaryText)
			.ColorAndOpacity(FLinearColor(0.35f, 0.78f, 0.95f, 1.0f))
		]
		+ SVerticalBox::Slot().FillHeight(1.0f)
		[
			SAssignNew(PresetPaletteView, STileView<FEntryPtr>)
			.ListItemsSource(&PresetPaletteEntries)
			.ItemWidth(122.0f)
			.ItemHeight(154.0f)
			.SelectionMode(ESelectionMode::Single)
			.OnGenerateTile(this, &SRVTMaterialManagerWidget::GenerateShelfTile)
		]
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildPresetView()
{
	const TSharedRef<SWidget> PresetEditor = SNew(SScrollBox)
	+ SScrollBox::Slot()
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 7.0f)
		[
			DevKitArtToolUI::MakeSectionHeader(1, LOCTEXT("PresetStack", "配置 RVT 层材质堆栈"), LOCTEXT("PresetStackDesc", "从左侧材质球或 Content Browser 将 MLI 拖入可视化槽位。拖入指定槽会替换；拖入列表末尾会自动补空槽或新增层。"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 2.0f, 8.0f, 6.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("DynamicLayerHint", "Background 固定为第一项；其余层可自由添加、删除和排序。空槽不会写入最终预设。"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground()).AutoWrapText(true)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("AddLayer", "+ 添加层")).OnClicked(this, &SRVTMaterialManagerWidget::AddPresetLayer)
			]
		]
		+ SVerticalBox::Slot().AutoHeight()
		[
			SAssignNew(PresetLayersBox, SVerticalBox)
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 7.0f)
		[
			DevKitArtToolUI::MakeSectionHeader(2, LOCTEXT("PresetOutput", "创建预设并赋予场景对象"), LOCTEXT("PresetOutputDesc", "创建基于 MI_Yog_Ground_RVT_Base 的材质实例；可只创建，也可创建后立即赋予当前选中 Actor 的全部材质槽。赋予操作只标脏关卡，不自动保存地图。"))
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 2.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("PresetName", "名称"))]
			+ SHorizontalBox::Slot().FillWidth(0.4f).Padding(0.0f, 0.0f, 12.0f, 0.0f)
			[
				SAssignNew(PresetNameTextBox, SEditableTextBox).Text(FText::FromString(TEXT("MI_RVT_LayerPreset_01")))
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 6.0f, 0.0f)[SNew(STextBlock).Text(LOCTEXT("PresetFolder", "目录"))]
			+ SHorizontalBox::Slot().FillWidth(0.6f)
			[
				SAssignNew(PresetFolderTextBox, SEditableTextBox).Text(FText::FromString(TEXT("/Game/Art/Material/RVT/Presets")))
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 8.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("CreateOnly", "创建预设")).OnClicked_Lambda([this]() { return CreatePreset(false); })
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 6.0f, 0.0f)
			[
				SNew(SButton).Text(LOCTEXT("CreateAssign", "创建并赋予选中物件")).OnClicked_Lambda([this]() { return CreatePreset(true); })
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SButton).Text(LOCTEXT("AssignLast", "赋予最近创建的预设")).OnClicked(this, &SRVTMaterialManagerWidget::AssignLastPresetToSelection)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(8.0f, 12.0f, 8.0f, 8.0f)
		[
			SNew(SButton)
			.Text(LOCTEXT("ReplaceTest", "将当前材质库实例写入 RVT 测试材质"))
			.ToolTipText(FText::FromString(TestMaterialPath))
			.OnClicked(this, &SRVTMaterialManagerWidget::ReplaceTestMaterialLayers)
		]
	];

	return SNew(SSplitter)
	.PhysicalSplitterHandleSize(3.0f)
	+ SSplitter::Slot().Value(0.31f).MinSize(300.0f)
	[
		BuildPresetPalette()
	]
	+ SSplitter::Slot().Value(0.69f).MinSize(620.0f)
	[
		PresetEditor
	];
}

TSharedRef<SWidget> SRVTMaterialManagerWidget::BuildPresetLayerSlot(const int32 SlotIndex, const FText& Label)
{
	const bool bIsBackground = SlotIndex == 0;
	const FEntryPtr SlotEntry = PresetLayerPaths.IsValidIndex(SlotIndex) ? FindEntryByPath(PresetLayerPaths[SlotIndex]) : nullptr;
	const bool bHasLayer = SlotEntry.IsValid();
	const FText LayerName = bHasLayer ? FText::FromString(SlotEntry->AssetName) : LOCTEXT("EmptyPresetLayer", "空槽");
	const FText LayerPath = bHasLayer
		? FText::FromName(SlotEntry->AssetData.PackageName)
		: LOCTEXT("PresetLayerDropPrompt", "将左侧材质球拖到这里");
	const FText DropAction = bHasLayer
		? LOCTEXT("ReplacePresetLayerHint", "拖入新的 MLI 可直接替换")
		: LOCTEXT("FillPresetLayerHint", "拖入 MLI 即刻填充");

	TSharedRef<SWidget> ThumbnailWidget = SNew(SBorder)
		.Padding(14.0f)
		.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Recessed")))
		[
			SNew(SImage)
			.Image(FAppStyle::GetBrush(TEXT("Icons.Plus")))
			.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		];
	if (bHasLayer && SlotEntry->Thumbnail.IsValid())
	{
		ThumbnailWidget = SlotEntry->Thumbnail->MakeThumbnailWidget();
	}

	return SNew(SRVTPresetLayerDropTarget)
	.OnCanAcceptAsset(FOnCanAcceptRVTPresetAsset::CreateLambda([this](const FAssetData& AssetData)
	{
		return IsLayerAssetAllowed(AssetData);
	}))
	.OnAssetDropped(FOnRVTPresetAssetDropped::CreateLambda([this, SlotIndex](const FAssetData& AssetData)
	{
		OnPresetLayerDropped(AssetData, SlotIndex);
	}))
	[
		SNew(SBorder)
		.Padding(FMargin(8.0f, 6.0f))
		.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
			[
				SNew(SBox).WidthOverride(88.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(Label).Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(bIsBackground ? LOCTEXT("BackgroundRequired", "必填 · 无 Blend") : LOCTEXT("LayerUsesBlend", "高度混合"))
						.ColorAndOpacity(bIsBackground
							? FLinearColor(0.35f, 0.82f, 1.0f, 1.0f)
							: FLinearColor(0.62f, 0.72f, 0.86f, 1.0f))
						.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 9.0f, 0.0f)
			[
				SNew(SBox).WidthOverride(64.0f).HeightOverride(64.0f)
				[
					ThumbnailWidget
				]
			]
			+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(LayerName).Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(LayerPath)
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Clipping(EWidgetClipping::ClipToBounds)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 3.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(DropAction)
					.ColorAndOpacity(FLinearColor(0.25f, 0.78f, 0.95f, 1.0f))
					.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FallbackPicker", "备用选择"))
					.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					.Font(FAppStyle::GetFontStyle(TEXT("SmallFont")))
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(SBox).WidthOverride(230.0f)
					[
						SNew(SObjectPropertyEntryBox)
						.AllowedClass(UMaterialFunctionMaterialLayerInstance::StaticClass())
						.ObjectPath_Lambda([this, SlotIndex]() { return GetPresetLayerPath(SlotIndex); })
						.OnObjectChanged_Lambda([this, SlotIndex](const FAssetData& AssetData) { OnPresetLayerChanged(AssetData, SlotIndex); })
						.OnShouldFilterAsset_Lambda([this](const FAssetData& AssetData) { return !IsLayerAssetAllowed(AssetData); })
						.AllowClear(!bIsBackground)
						.DisplayThumbnail(false)
						.DisplayBrowse(true)
						.DisplayUseSelected(true)
					]
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(9.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(STextBlock)
				.Text(bIsBackground ? FText::GetEmpty() : LOCTEXT("DefaultBlend", "MLBI_BasicMatMask_Base"))
				.ColorAndOpacity(FSlateColor::UseSubduedForeground())
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(8.0f, 0.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				.Visibility(bIsBackground ? EVisibility::Collapsed : EVisibility::Visible)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("↑"))).ToolTipText(LOCTEXT("MoveLayerUp", "上移一层"))
					.IsEnabled(SlotIndex > 1).OnClicked(this, &SRVTMaterialManagerWidget::MovePresetLayer, SlotIndex, -1)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(3.0f, 0.0f)
				[
					SNew(SButton).Text(FText::FromString(TEXT("↓"))).ToolTipText(LOCTEXT("MoveLayerDown", "下移一层"))
					.IsEnabled(SlotIndex < PresetLayerPaths.Num() - 1).OnClicked(this, &SRVTMaterialManagerWidget::MovePresetLayer, SlotIndex, 1)
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SButton).Text(FText::FromString(TEXT("×"))).ToolTipText(LOCTEXT("RemoveLayer", "删除该层"))
					.OnClicked(this, &SRVTMaterialManagerWidget::RemovePresetLayer, SlotIndex)
				]
			]
		]
	];
}

void SRVTMaterialManagerWidget::RebuildPresetLayerRows()
{
	if (!PresetLayersBox.IsValid())
	{
		return;
	}
	PresetLayersBox->ClearChildren();
	for (int32 SlotIndex = 0; SlotIndex < PresetLayerPaths.Num(); ++SlotIndex)
	{
		const FText Label = SlotIndex == 0
			? LOCTEXT("Background", "Background")
			: FText::Format(LOCTEXT("DynamicLayerLabel", "Layer {0}"), FText::AsNumber(SlotIndex));
		PresetLayersBox->AddSlot().AutoHeight().Padding(8.0f, 0.0f, 8.0f, 5.0f)[BuildPresetLayerSlot(SlotIndex, Label)];
	}

	PresetLayersBox->AddSlot().AutoHeight().Padding(8.0f, 3.0f, 8.0f, 0.0f)
	[
		SNew(SRVTPresetLayerDropTarget)
		.OnCanAcceptAsset(FOnCanAcceptRVTPresetAsset::CreateLambda([this](const FAssetData& AssetData)
		{
			return IsLayerAssetAllowed(AssetData);
		}))
		.OnAssetDropped(FOnRVTPresetAssetDropped::CreateSP(this, &SRVTMaterialManagerWidget::AutoPlacePresetLayer))
		[
			SNew(SBorder)
			.Padding(FMargin(10.0f, 8.0f))
			.BorderImage(FAppStyle::GetBrush(TEXT("NoBorder")))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
				[
					SNew(SImage)
					.Image(FAppStyle::GetBrush(TEXT("Icons.Plus")))
					.DesiredSizeOverride(FVector2D(16.0f, 16.0f))
					.ColorAndOpacity(FLinearColor(0.25f, 0.78f, 0.95f, 1.0f))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AutoPlaceDropTitle", "拖到这里自动放入层堆栈"))
						.Font(FAppStyle::GetFontStyle(TEXT("NormalFontBold")))
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.Text(LOCTEXT("AutoPlaceDropHint", "优先填充 Background 或已有空槽；没有空槽时自动新增 Layer。"))
						.ColorAndOpacity(FSlateColor::UseSubduedForeground())
					]
				]
			]
		]
	];
}

FReply SRVTMaterialManagerWidget::AddPresetLayer()
{
	PresetLayerPaths.Add(FSoftObjectPath());
	RebuildPresetLayerRows();
	SetStatus(FText::Format(LOCTEXT("LayerAdded", "已添加 Layer {0}。拖入 MLI 后即可参与预设。"), FText::AsNumber(PresetLayerPaths.Num() - 1)));
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::RemovePresetLayer(const int32 SlotIndex)
{
	if (SlotIndex > 0 && PresetLayerPaths.IsValidIndex(SlotIndex))
	{
		PresetLayerPaths.RemoveAt(SlotIndex);
		RebuildPresetLayerRows();
		SetStatus(LOCTEXT("LayerRemoved", "已删除层并重新编号。"));
	}
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::MovePresetLayer(const int32 SlotIndex, const int32 Direction)
{
	const int32 DestinationIndex = SlotIndex + Direction;
	if (SlotIndex > 0 && DestinationIndex > 0 && PresetLayerPaths.IsValidIndex(SlotIndex) && PresetLayerPaths.IsValidIndex(DestinationIndex))
	{
		PresetLayerPaths.Swap(SlotIndex, DestinationIndex);
		RebuildPresetLayerRows();
		SetStatus(LOCTEXT("LayerMoved", "已调整层顺序。"));
	}
	return FReply::Handled();
}

void SRVTMaterialManagerWidget::OnPresetPaletteSearchTextChanged(const FText& InSearchText)
{
	PresetPaletteSearchText = InSearchText.ToString().TrimStartAndEnd();
	RebuildPresetPalette();
}

void SRVTMaterialManagerWidget::RebuildPresetPalette()
{
	PresetPaletteEntries.Reset();
	for (const FEntryPtr& Entry : AllEntries)
	{
		if (!Entry.IsValid())
		{
			continue;
		}

		const bool bMatchesSearch = PresetPaletteSearchText.IsEmpty()
			|| Entry->AssetName.Contains(PresetPaletteSearchText, ESearchCase::IgnoreCase)
			|| Entry->Category.Contains(PresetPaletteSearchText, ESearchCase::IgnoreCase)
			|| Entry->AssetData.PackageName.ToString().Contains(PresetPaletteSearchText, ESearchCase::IgnoreCase);
		if (bMatchesSearch)
		{
			PresetPaletteEntries.Add(Entry);
		}
	}

	if (PresetPaletteView.IsValid())
	{
		PresetPaletteView->RequestListRefresh();
	}
}

void SRVTMaterialManagerWidget::OnPresetLayerDropped(const FAssetData& AssetData, const int32 SlotIndex)
{
	OnPresetLayerChanged(AssetData, SlotIndex);
}

void SRVTMaterialManagerWidget::AutoPlacePresetLayer(const FAssetData& AssetData)
{
	if (!IsLayerAssetAllowed(AssetData))
	{
		SetStatus(LOCTEXT("PresetDropRejected", "只能放入 CommonTex 材质库中的 Material Layer Instance。"), true);
		return;
	}

	int32 TargetIndex = INDEX_NONE;
	for (int32 SlotIndex = 0; SlotIndex < PresetLayerPaths.Num(); ++SlotIndex)
	{
		if (!PresetLayerPaths[SlotIndex].IsValid())
		{
			TargetIndex = SlotIndex;
			break;
		}
	}

	if (TargetIndex == INDEX_NONE)
	{
		TargetIndex = PresetLayerPaths.Add(AssetData.ToSoftObjectPath());
	}
	else
	{
		PresetLayerPaths[TargetIndex] = AssetData.ToSoftObjectPath();
	}

	RebuildPresetLayerRows();
	const FText SlotLabel = TargetIndex == 0
		? LOCTEXT("AutoPlacedBackground", "Background")
		: FText::Format(LOCTEXT("AutoPlacedLayer", "Layer {0}"), FText::AsNumber(TargetIndex));
	SetStatus(FText::Format(
		LOCTEXT("PresetAutoPlaced", "已将 {0} 自动放入 {1}。"),
		FText::FromName(AssetData.AssetName),
		SlotLabel));
}

void SRVTMaterialManagerWidget::RebuildLibraryCategoryButtons()
{
	if (!LibraryCategoryWrapBox.IsValid())
	{
		return;
	}

	LibraryCategoryWrapBox->ClearChildren();
	for (const FString& Category : LibraryCategories)
	{
		LibraryCategoryWrapBox->AddSlot()
		[
			BuildLibraryCategoryFilterButton(Category)
		];
	}
}

void SRVTMaterialManagerWidget::RebuildLibraryTree()
{
	const FName SelectedPackageName = SelectedEntry.IsValid() ? SelectedEntry->AssetData.PackageName : NAME_None;
	if (LibraryTreeView.IsValid())
	{
		LibraryTreeView->ClearSelection();
		LibraryTreeView->ClearExpandedItems();
	}

	LibraryTreeRootNodes.Reset();
	TMap<FString, FTreeNodePtr> CategoryNodes;
	FTreeNodePtr RestoredSelection;

	for (const FEntryPtr& Entry : BaseEntries)
	{
		if (!PassesLibraryFilter(Entry))
		{
			continue;
		}

		FTreeNodePtr* ExistingCategoryNode = CategoryNodes.Find(Entry->Category);
		if (!ExistingCategoryNode)
		{
			FTreeNodePtr CategoryNode = MakeShared<FRVTMaterialLibraryTreeNode>();
			CategoryNode->DisplayName = Entry->Category;
			CategoryNode->Category = Entry->Category;
			CategoryNode->bIsCategory = true;
			LibraryTreeRootNodes.Add(CategoryNode);
			CategoryNodes.Add(Entry->Category, CategoryNode);
			ExistingCategoryNode = CategoryNodes.Find(Entry->Category);
		}

		FTreeNodePtr AssetNode = MakeShared<FRVTMaterialLibraryTreeNode>();
		AssetNode->DisplayName = Entry->AssetName;
		AssetNode->Category = Entry->Category;
		AssetNode->Entry = Entry;
		(*ExistingCategoryNode)->Children.Add(AssetNode);
		if (!SelectedPackageName.IsNone() && Entry->AssetData.PackageName == SelectedPackageName)
		{
			RestoredSelection = AssetNode;
		}
	}

	LibraryTreeRootNodes.Sort([](const FTreeNodePtr& Left, const FTreeNodePtr& Right)
	{
		return Left.IsValid() && Right.IsValid() && Left->DisplayName < Right->DisplayName;
	});
	for (const FTreeNodePtr& RootNode : LibraryTreeRootNodes)
	{
		if (RootNode.IsValid())
		{
			RootNode->Children.Sort([](const FTreeNodePtr& Left, const FTreeNodePtr& Right)
			{
				return Left.IsValid() && Right.IsValid() && Left->DisplayName < Right->DisplayName;
			});
		}
	}

	if (LibraryTreeView.IsValid())
	{
		LibraryTreeView->RequestTreeRefresh();
		for (const FTreeNodePtr& RootNode : LibraryTreeRootNodes)
		{
			LibraryTreeView->SetItemExpansion(RootNode, true);
		}
		if (RestoredSelection.IsValid())
		{
			LibraryTreeView->SetSelection(RestoredSelection, ESelectInfo::Direct);
		}
		else if (SelectedEntry.IsValid())
		{
			SelectEntry(nullptr);
		}
	}
}

TSharedRef<ITableRow> SRVTMaterialManagerWidget::GenerateLibraryTreeRow(
	FTreeNodePtr Node,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (!Node.IsValid() || Node->bIsCategory)
	{
		const FText CategoryLabel = Node.IsValid()
			? FText::Format(LOCTEXT("CategoryTreeLabel", "{0}  ·  {1} 组"), FText::FromString(Node->DisplayName), FText::AsNumber(Node->Children.Num()))
			: LOCTEXT("InvalidCategory", "未分类");
		return SNew(STableRow<FTreeNodePtr>, OwnerTable)
			.Padding(FMargin(5.0f, 4.0f))
			[
				SNew(STextBlock)
					.Text(CategoryLabel)
					.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
			];
	}

	const FEntryPtr Entry = Node->Entry;
	TSharedRef<SWidget> ThumbnailWidget = SNew(SBox);
	if (Entry.IsValid() && Entry->Thumbnail.IsValid())
	{
		ThumbnailWidget = Entry->Thumbnail->MakeThumbnailWidget();
	}
	const bool bHasVariants = Entry.IsValid() && !Entry->Variants.IsEmpty();
	const FLinearColor CardColor = bHasVariants ? VariantCardColor : BaseCardColor;
	const FSlateColor VariantTextColor = bHasVariants
		? FSlateColor(FLinearColor(1.0f, 0.80f, 0.36f))
		: FSlateColor(FLinearColor(0.35f, 0.72f, 1.0f));

	return SNew(STableRow<FTreeNodePtr>, OwnerTable)
		.Padding(2.0f)
		.OnDragDetected_Lambda([this, Entry](const FGeometry&, const FPointerEvent&)
		{
			return BeginEntryDrag(Entry);
		})
		[
			SNew(SBorder)
				.Padding(5.0f)
				.BorderImage(FAppStyle::GetBrush(TEXT("Brushes.Panel")))
				.BorderBackgroundColor(CardColor)
				.ToolTipText(LOCTEXT("LibraryRowTooltip", "单击查看详情；双击打开 MLI；Ctrl+B 在 Content Browser 中定位；也可拖入 Material Layers。"))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0.0f, 0.0f, 8.0f, 0.0f)
					[
						SNew(SBox).WidthOverride(58.0f).HeightOverride(58.0f)
						[
							ThumbnailWidget
						]
					]
					+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot().AutoHeight()
						[
							SNew(STextBlock)
								.Text(Entry.IsValid() ? FText::FromString(Entry->AssetName) : LOCTEXT("InvalidEntry", "无效材质"))
								.Font(FAppStyle::GetFontStyle(TEXT("PropertyWindow.BoldFont")))
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(Entry.IsValid() ? FText::FromName(Entry->AssetData.PackageName) : FText::GetEmpty())
								.ColorAndOpacity(FSlateColor::UseSubduedForeground())
								.Clipping(EWidgetClipping::ClipToBounds)
						]
						+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 2.0f, 0.0f, 0.0f)
						[
							SNew(STextBlock)
								.Text(Entry.IsValid()
									? FText::Format(LOCTEXT("LibraryRowMeta", "基础材质 · {0} 个变体 · {1}"), FText::AsNumber(Entry->Variants.Num()), FText::FromString(Entry->Category))
									: FText::GetEmpty())
								.ColorAndOpacity(VariantTextColor)
						]
					]
				]
		];
}

void SRVTMaterialManagerWidget::GetLibraryTreeChildren(FTreeNodePtr Node, TArray<FTreeNodePtr>& OutChildren) const
{
	if (Node.IsValid())
	{
		OutChildren.Append(Node->Children);
	}
}

void SRVTMaterialManagerWidget::OnLibraryTreeSelectionChanged(FTreeNodePtr Node, ESelectInfo::Type)
{
	if (Node.IsValid() && !Node->bIsCategory && Node->Entry.IsValid())
	{
		SelectEntry(Node->Entry);
	}
	else if (Node.IsValid() && Node->bIsCategory)
	{
		SelectEntry(nullptr);
	}
}

void SRVTMaterialManagerWidget::OnLibraryTreeDoubleClicked(FTreeNodePtr Node)
{
	if (!Node.IsValid())
	{
		return;
	}
	if (Node->bIsCategory)
	{
		if (LibraryTreeView.IsValid())
		{
			LibraryTreeView->SetItemExpansion(Node, !LibraryTreeView->IsItemExpanded(Node));
		}
		return;
	}

	SelectEntry(Node->Entry);
	OpenSelectedAsset();
}

void SRVTMaterialManagerWidget::OnVariantDoubleClicked(FEntryPtr Entry)
{
	SelectEntry(Entry);
	OpenSelectedAsset();
}

FReply SRVTMaterialManagerWidget::HandleMaterialLibraryKeyDown(const FGeometry&, const FKeyEvent& KeyEvent)
{
	if (KeyEvent.IsControlDown() && !KeyEvent.IsAltDown() && KeyEvent.GetKey() == EKeys::B)
	{
		if (LibraryTreeView.IsValid())
		{
			const TArray<FTreeNodePtr> SelectedNodes = LibraryTreeView->GetSelectedItems();
			if (SelectedNodes.Num() == 1 && SelectedNodes[0].IsValid() && !SelectedNodes[0]->bIsCategory && SelectedNodes[0]->Entry.IsValid())
			{
				SelectedEntry = SelectedNodes[0]->Entry;
				return SyncSelectedAssetToContentBrowser();
			}
		}
		return FReply::Unhandled();
	}
	return FReply::Unhandled();
}

TSharedRef<ITableRow> SRVTMaterialManagerWidget::GenerateShelfTile(FEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<FEntryPtr>, OwnerTable)
	.Padding(3.0f)
	[
		SNew(SRVTMaterialShelfCard)
		.Entry(Entry)
		.OnSelected(FOnRVTMaterialCardSelected::CreateSP(this, &SRVTMaterialManagerWidget::SelectEntry))
		.OnDragged(FOnRVTMaterialCardDragged::CreateSP(this, &SRVTMaterialManagerWidget::BeginEntryDrag))
	];
}

TSharedRef<ITableRow> SRVTMaterialManagerWidget::GenerateVariantTile(FEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable)
{
	return GenerateShelfTile(Entry, OwnerTable);
}

void SRVTMaterialManagerWidget::OnLibrarySearchTextChanged(const FText& InSearchText)
{
	LibrarySearchText = InSearchText.ToString().TrimStartAndEnd();
	RebuildLibraryTree();
}

void SRVTMaterialManagerWidget::OnLibraryCategoryFilterChanged(const ECheckBoxState NewState, FString Category)
{
	if (NewState == ECheckBoxState::Checked)
	{
		EnabledLibraryCategories.Add(Category);
	}
	else
	{
		EnabledLibraryCategories.Remove(Category);
	}
	RebuildLibraryTree();
}

ECheckBoxState SRVTMaterialManagerWidget::IsLibraryCategoryFilterChecked(FString Category) const
{
	return EnabledLibraryCategories.Contains(Category) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool SRVTMaterialManagerWidget::PassesLibraryFilter(const FEntryPtr& Entry) const
{
	if (!Entry.IsValid() || !EnabledLibraryCategories.Contains(Entry->Category))
	{
		return false;
	}
	if (LibrarySearchText.IsEmpty())
	{
		return true;
	}

	FString SearchableText = FString::Printf(
		TEXT("%s %s %s"),
		*Entry->AssetName,
		*Entry->Category,
		*Entry->AssetData.PackageName.ToString());
	for (const FEntryPtr& Variant : Entry->Variants)
	{
		if (Variant.IsValid())
		{
			SearchableText += TEXT(" ") + Variant->AssetName;
		}
	}
	return SearchableText.Contains(LibrarySearchText, ESearchCase::IgnoreCase);
}

void SRVTMaterialManagerWidget::ScanLibrary()
{
	TSet<FString> PreviousCategories;
	for (const FString& Category : LibraryCategories)
	{
		PreviousCategories.Add(Category);
	}
	AllEntries.Reset();
	BaseEntries.Reset();
	CurrentVariants.Reset();
	SelectedEntry.Reset();

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.ScanPathsSynchronous({LibraryRoot}, true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(FName(*LibraryRoot), Assets, true, false);

	for (const FAssetData& AssetData : Assets)
	{
		UMaterialFunctionMaterialLayerInstance* Layer = Cast<UMaterialFunctionMaterialLayerInstance>(AssetData.GetAsset());
		if (!Layer || !AssetData.AssetName.ToString().StartsWith(TEXT("MLI_")))
		{
			continue;
		}
		FEntryPtr Entry = MakeShared<FRVTMaterialLibraryEntry>();
		Entry->AssetData = AssetData;
		Entry->AssetName = AssetData.AssetName.ToString();
		Entry->Category = NormalizeCategory(FPackageName::GetLongPackagePath(AssetData.PackageName.ToString()).RightChop(LibraryRoot.Len() + 1));
		Entry->Thumbnail = MakeShared<FAssetThumbnail>(AssetData, 112, 112, ThumbnailPool);
		AllEntries.Add(Entry);
	}

	AllEntries.Sort([](const FEntryPtr& A, const FEntryPtr& B)
	{
		if (A->Category != B->Category)
		{
			return A->Category < B->Category;
		}
		return A->AssetName < B->AssetName;
	});

	for (const FEntryPtr& Entry : AllEntries)
	{
		FEntryPtr BestBase;
		for (const FEntryPtr& Candidate : AllEntries)
		{
			if (Candidate == Entry || Candidate->AssetName.Len() >= Entry->AssetName.Len())
			{
				continue;
			}
			if (Candidate->Category != Entry->Category)
			{
				continue;
			}

			const FString VariantPrefix = Candidate->AssetName + TEXT("_");
			if (!Entry->AssetName.StartsWith(VariantPrefix))
			{
				continue;
			}
			const FString VariantSuffix = Entry->AssetName.RightChop(VariantPrefix.Len());
			const bool bRecognizedVariant = VariantSuffix.Equals(TEXT("a"), ESearchCase::IgnoreCase)
				|| VariantSuffix.Equals(TEXT("b"), ESearchCase::IgnoreCase)
				|| VariantSuffix.Equals(TEXT("c"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Damage"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Dirt"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Dirty"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Color"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Tint"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Broken"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Worn"), ESearchCase::IgnoreCase)
				|| VariantSuffix.StartsWith(TEXT("Variant"), ESearchCase::IgnoreCase);
			if (bRecognizedVariant
				&& (!BestBase.IsValid() || Candidate->AssetName.Len() > BestBase->AssetName.Len()))
			{
				BestBase = Candidate;
			}
		}
		if (BestBase.IsValid())
		{
			Entry->bIsVariant = true;
			Entry->BaseAssetName = BestBase->AssetName;
			BestBase->Variants.Add(Entry);
		}
		else
		{
			Entry->BaseAssetName = Entry->AssetName;
			BaseEntries.Add(Entry);
		}
	}

	LibraryCategories.Reset();
	for (const FEntryPtr& Entry : BaseEntries)
	{
		if (Entry.IsValid())
		{
			LibraryCategories.AddUnique(Entry->Category);
		}
	}
	LibraryCategories.Sort();
	for (const FString& Category : LibraryCategories)
	{
		if (!PreviousCategories.Contains(Category))
		{
			EnabledLibraryCategories.Add(Category);
		}
	}
	TArray<FString> CategoriesToRemove;
	for (const FString& Category : EnabledLibraryCategories)
	{
		if (!LibraryCategories.Contains(Category))
		{
			CategoriesToRemove.Add(Category);
		}
	}
	for (const FString& Category : CategoriesToRemove)
	{
		EnabledLibraryCategories.Remove(Category);
	}

	RebuildLibraryCategoryButtons();
	RebuildLibraryTree();
	RebuildPresetPalette();
	RebuildPresetLayerRows();
	RefreshDetailPanel();
	SetStatus(FText::Format(LOCTEXT("ScanDone", "已扫描 {0} 个基础 MLI、{1} 个变体。材质库：{2}"), FText::AsNumber(BaseEntries.Num()), FText::AsNumber(AllEntries.Num() - BaseEntries.Num()), FText::FromString(LibraryRoot)));
}

void SRVTMaterialManagerWidget::SelectEntry(FEntryPtr Entry)
{
	SelectedEntry = Entry;
	CurrentVariants.Reset();
	if (Entry.IsValid())
	{
		if (!Entry->bIsVariant)
		{
			CurrentVariants = Entry->Variants;
		}
		else
		{
			for (const FEntryPtr& BaseEntry : BaseEntries)
			{
				if (BaseEntry.IsValid() && BaseEntry->Category == Entry->Category && BaseEntry->AssetName == Entry->BaseAssetName)
				{
					CurrentVariants = BaseEntry->Variants;
					break;
				}
			}
		}
	}
	if (TargetLayerNameTextBox.IsValid() && Entry.IsValid())
	{
		TargetLayerNameTextBox->SetText(FText::FromString(Entry->AssetName));
	}
	if (CategoryTextBox.IsValid() && Entry.IsValid())
	{
		CategoryTextBox->SetText(FText::FromString(Entry->Category));
	}
	RefreshDetailPanel();
}

FReply SRVTMaterialManagerWidget::BeginEntryDrag(FEntryPtr Entry) const
{
	return Entry.IsValid() ? FReply::Handled().BeginDragDrop(FAssetDragDropOp::New(Entry->AssetData)) : FReply::Unhandled();
}

void SRVTMaterialManagerWidget::RefreshDetailPanel()
{
	if (DetailContentBox.IsValid())
	{
		DetailContentBox->SetContent(BuildSelectedDetailContent());
	}
}

FReply SRVTMaterialManagerWidget::RefreshLibrary()
{
	ScanLibrary();
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::OpenSelectedAsset()
{
	if (SelectedEntry.IsValid() && GEditor)
	{
		if (UObject* Asset = SelectedEntry->AssetData.GetAsset())
		{
			if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
			{
				AssetEditorSubsystem->OpenEditorForAsset(Asset);
			}
		}
	}
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::SyncSelectedAssetToContentBrowser()
{
	if (SelectedEntry.IsValid() && SelectedEntry->AssetData.IsValid() && GEditor)
	{
		TArray<FAssetData> AssetsToSync;
		AssetsToSync.Add(SelectedEntry->AssetData);
		GEditor->SyncBrowserToObjects(AssetsToSync, true);
		SetStatus(FText::Format(
			LOCTEXT("SyncedSelectedMaterial", "已在 Content Browser 中定位：{0}"),
			FText::FromString(SelectedEntry->AssetName)));
	}
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::CreateVariant()
{
	if (!SelectedEntry.IsValid())
	{
		SetStatus(LOCTEXT("SelectBeforeVariant", "请先选择一个基础 MLI。"), true);
		return FReply::Handled();
	}
	UMaterialFunctionMaterialLayerInstance* Source = Cast<UMaterialFunctionMaterialLayerInstance>(SelectedEntry->AssetData.GetAsset());
	FString Suffix = VariantSuffixTextBox.IsValid() ? VariantSuffixTextBox->GetText().ToString().TrimStartAndEnd() : TEXT("Damage");
	Suffix.RemoveFromStart(TEXT("_"));
	if (!Source || Suffix.IsEmpty())
	{
		SetStatus(LOCTEXT("InvalidVariant", "变体后缀不能为空。"), true);
		return FReply::Handled();
	}
	const FString BaseName = SelectedEntry->bIsVariant ? SelectedEntry->BaseAssetName : SelectedEntry->AssetName;
	const FString AssetName = BaseName + TEXT("_") + Suffix;
	const FString DestinationPath = FPackageName::GetLongPackagePath(Source->GetOutermost()->GetName());
	if (FindObject<UObject>(nullptr, *(DestinationPath + TEXT("/") + AssetName)) || FPackageName::DoesPackageExist(DestinationPath + TEXT("/") + AssetName))
	{
		SetStatus(FText::Format(LOCTEXT("VariantExists", "变体已存在：{0}"), FText::FromString(AssetName)), true);
		return FReply::Handled();
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateVariantTransaction", "创建 RVT 材质变体"));
	UMaterialFunctionMaterialLayerInstance* Variant = DuplicateLayer(Source, DestinationPath, AssetName);
	if (!Variant || !SaveAsset(Variant, true))
	{
		SetStatus(LOCTEXT("VariantFailed", "创建变体失败，请检查日志和源控制状态。"), true);
		return FReply::Handled();
	}
	ScanLibrary();
	SetStatus(FText::Format(LOCTEXT("VariantCreated", "已创建变体 {0}。它继承当前三张贴图，可通过导入按钮只替换需要变化的贴图。"), FText::FromString(Variant->GetPathName())));
	GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Variant);
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::ImportOrUpdateTextures()
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		SetStatus(LOCTEXT("NoDesktopPlatform", "无法打开文件选择窗口。"), true);
		return FReply::Handled();
	}
	TArray<FString> Filenames;
	const void* ParentWindow = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);
	if (!DesktopPlatform->OpenFileDialog(ParentWindow, TEXT("导入 RVT 材质贴图"), TEXT(""), TEXT(""), TEXT("Images (*.png;*.tga;*.tif;*.tiff;*.exr)|*.png;*.tga;*.tif;*.tiff;*.exr"), EFileDialogFlags::Multiple, Filenames))
	{
		return FReply::Handled();
	}

	TMap<FName, FString> SourceFiles;
	FString TextureCore;
	for (const FString& Filename : Filenames)
	{
		const FString Base = FPaths::GetBaseFilename(Filename);
		struct FSuffix { const TCHAR* Text; FName Parameter; };
		const FSuffix Suffixes[] = {{TEXT("_BaseColor"), TEXT("T BaseColor")}, {TEXT("_MRAH"), TEXT("T MRAH")}, {TEXT("_NormalLight"), TEXT("T Normal")}};
		bool bMatched = false;
		for (const FSuffix& Suffix : Suffixes)
		{
			if (Base.EndsWith(Suffix.Text))
			{
				SourceFiles.Add(Suffix.Parameter, Filename);
				if (TextureCore.IsEmpty())
				{
					TextureCore = Base.LeftChop(FCString::Strlen(Suffix.Text));
				}
				bMatched = true;
				break;
			}
		}
		if (!bMatched)
		{
			SetStatus(FText::Format(LOCTEXT("BadTextureName", "无法识别贴图后缀：{0}。需要 _BaseColor、_MRAH 或 _NormalLight。"), FText::FromString(Base)), true);
			return FReply::Handled();
		}
	}
	if (SourceFiles.IsEmpty())
	{
		return FReply::Handled();
	}

	const FString Category = NormalizeCategory(CategoryTextBox.IsValid() ? CategoryTextBox->GetText().ToString() : TEXT("Ground"));
	const FString DestinationPath = LibraryRoot + TEXT("/") + Category;
	FString TargetName = TargetLayerNameTextBox.IsValid() ? TargetLayerNameTextBox->GetText().ToString().TrimStartAndEnd() : FString();
	if (TargetName.IsEmpty() && SelectedEntry.IsValid())
	{
		TargetName = SelectedEntry->AssetName;
	}
	if (TargetName.IsEmpty())
	{
		TargetName = MakeInferredLayerName(TextureCore);
	}
	if (!TargetName.StartsWith(TEXT("MLI_")))
	{
		TargetName = TEXT("MLI_") + TargetName;
	}

	TArray<UAssetImportTask*> Tasks;
	TMap<FName, UAssetImportTask*> TaskByParameter;
	for (const TPair<FName, FString>& Pair : SourceFiles)
	{
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = Pair.Value;
		Task->DestinationPath = DestinationPath;
		Task->DestinationName = FPaths::GetBaseFilename(Pair.Value);
		Task->bAutomated = true;
		Task->bReplaceExisting = true;
		Task->bReplaceExistingSettings = true;
		Task->bSave = false;
		Task->bAsync = false;
		Tasks.Add(Task);
		TaskByParameter.Add(Pair.Key, Task);
	}
	FAssetToolsModule::GetModule().Get().ImportAssetTasks(Tasks);

	TMap<FName, UTexture*> ImportedTextures;
	for (const TPair<FName, UAssetImportTask*>& Pair : TaskByParameter)
	{
		UTexture2D* Texture = Pair.Value && Pair.Value->ImportedObjectPaths.Num() > 0
			? LoadObject<UTexture2D>(nullptr, *Pair.Value->ImportedObjectPaths[0]) : nullptr;
		if (!Texture)
		{
			SetStatus(FText::Format(LOCTEXT("ImportFailed", "贴图导入失败：{0}"), FText::FromName(Pair.Key)), true);
			return FReply::Handled();
		}

		bool bExpectedSRGB = false;
		TextureCompressionSettings ExpectedCompression = TC_Default;
		if (Pair.Key == FName(TEXT("T BaseColor")))
		{
			bExpectedSRGB = true;
			ExpectedCompression = TC_Default;
		}
		else if (Pair.Key == FName(TEXT("T MRAH")))
		{
			ExpectedCompression = TC_Masks;
		}
		else if (Pair.Key == FName(TEXT("T Normal")))
		{
			// NormalLight stores tangent-space normal XY in RG and MaterialLightMask in B.
			// BC5 / TC_Normalmap would discard the authored B channel.
			ExpectedCompression = TC_BC7;
		}
		else
		{
			SetStatus(FText::Format(LOCTEXT("UnsupportedTextureParameter", "不支持的贴图参数：{0}"), FText::FromName(Pair.Key)), true);
			return FReply::Handled();
		}

		const auto ValidateTextureContract = [bExpectedSRGB, ExpectedCompression](const UTexture2D* TextureToValidate)
		{
			return TextureToValidate
				&& !TextureToValidate->VirtualTextureStreaming
				&& TextureToValidate->SRGB == bExpectedSRGB
				&& TextureToValidate->CompressionSettings == ExpectedCompression;
		};

		Texture->Modify();
		Texture->VirtualTextureStreaming = false;
		Texture->SRGB = bExpectedSRGB;
		Texture->CompressionSettings = ExpectedCompression;
		Texture->PostEditChange();
		if (!ValidateTextureContract(Texture))
		{
			SetStatus(FText::Format(
				LOCTEXT("TextureContractFailed", "贴图格式校验失败：{0}。BaseColor 必须 sRGB/Default，MRAH 必须 Linear/Masks，NormalLight 必须 Linear/BC7。"),
				FText::FromString(Texture->GetPathName())), true);
			return FReply::Handled();
		}
		if (!SaveAsset(Texture, true))
		{
			SetStatus(FText::Format(LOCTEXT("TextureSaveFailed", "保存贴图失败：{0}"), FText::FromString(Texture->GetPathName())), true);
			return FReply::Handled();
		}
		if (!ValidateTextureContract(Texture))
		{
			SetStatus(FText::Format(
				LOCTEXT("SavedTextureContractFailed", "贴图保存后的格式校验失败：{0}"),
				FText::FromString(Texture->GetPathName())), true);
			return FReply::Handled();
		}
		ImportedTextures.Add(Pair.Key, Texture);
	}

	const FString LayerObjectPath = DestinationPath + TEXT("/") + TargetName + TEXT(".") + TargetName;
	UMaterialFunctionMaterialLayerInstance* Layer = LoadObject<UMaterialFunctionMaterialLayerInstance>(nullptr, *LayerObjectPath);
	if (!Layer)
	{
		UMaterialFunctionMaterialLayerInstance* Source = SelectedEntry.IsValid()
			? Cast<UMaterialFunctionMaterialLayerInstance>(SelectedEntry->AssetData.GetAsset())
			: LoadObject<UMaterialFunctionMaterialLayerInstance>(nullptr, *LayerTemplatePath);
		Layer = DuplicateLayer(Source, DestinationPath, TargetName);
	}
	if (!Layer)
	{
		SetStatus(LOCTEXT("LayerCreateFailed", "无法创建目标 MLI。"), true);
		return FReply::Handled();
	}

	Layer->Modify();
	Layer->UpdateParameterSet();
	for (FTextureParameterValue& Parameter : Layer->TextureParameterValues)
	{
		if (UTexture** Imported = ImportedTextures.Find(Parameter.ParameterInfo.Name))
		{
			Parameter.ParameterValue = *Imported;
		}
	}
	Layer->PostEditChange();
	if (!SaveAsset(Layer, true))
	{
		SetStatus(LOCTEXT("LayerSaveFailed", "MLI 已修改但保存失败。"), true);
		return FReply::Handled();
	}

	ScanLibrary();
	SetStatus(FText::Format(LOCTEXT("ImportSuccess", "已导入 {0} 张贴图并更新 {1}。未提供的贴图参数保持原值。"), FText::AsNumber(ImportedTextures.Num()), FText::FromString(Layer->GetPathName())));
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::ShowLibraryView()
{
	ViewSwitcher->SetActiveWidgetIndex(0);
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::ShowPresetView()
{
	ViewSwitcher->SetActiveWidgetIndex(1);
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::ReplaceTestMaterialLayers()
{
	UMaterialInstanceConstant* TestMaterial = LoadObject<UMaterialInstanceConstant>(nullptr, *TestMaterialPath);
	UMaterialFunctionMaterialLayerBlendInstance* DefaultBlend = LoadObject<UMaterialFunctionMaterialLayerBlendInstance>(nullptr, *BlendTemplatePath);
	if (!TestMaterial || !DefaultBlend)
	{
		SetStatus(LOCTEXT("TestDependenciesMissing", "测试材质或默认 Blend 缺失。"), true);
		return FReply::Handled();
	}

	TArray<UMaterialFunctionMaterialLayerInstance*> SelectedLayers;
	UMaterialFunctionMaterialLayerInstance* SelectedBackground = PresetLayerPaths.Num() > 0 && PresetLayerPaths[0].IsValid()
		? Cast<UMaterialFunctionMaterialLayerInstance>(PresetLayerPaths[0].TryLoad()) : nullptr;
	if (SelectedBackground)
	{
		SelectedLayers.Add(SelectedBackground);
	}
	for (int32 SlotIndex = 1; SlotIndex < PresetLayerPaths.Num(); ++SlotIndex)
	{
		const FSoftObjectPath& LayerPath = PresetLayerPaths[SlotIndex];
		if (LayerPath.IsValid())
		{
			if (UMaterialFunctionMaterialLayerInstance* Layer = Cast<UMaterialFunctionMaterialLayerInstance>(LayerPath.TryLoad()))
			{
				SelectedLayers.Add(Layer);
			}
		}
	}
	if (!SelectedBackground)
	{
		SetStatus(LOCTEXT("TestNeedsBackground", "请先在动态层列表中设置 Background；空层会被忽略。"), true);
		return FReply::Handled();
	}

	FMaterialLayersFunctions Layers;
	Layers.AddDefaultBackgroundLayer();
	Layers.Layers[0] = SelectedLayers[0];
	Layers.EditorOnly.LayerNames[0] = FText::FromString(SelectedLayers[0]->GetName());
	for (int32 LayerIndex = 1; LayerIndex < SelectedLayers.Num(); ++LayerIndex)
	{
		const int32 NewLayerIndex = Layers.AppendBlendedLayer();
		Layers.Layers[NewLayerIndex] = SelectedLayers[LayerIndex];
		Layers.Blends[NewLayerIndex - 1] = DefaultBlend;
		Layers.EditorOnly.LayerNames[NewLayerIndex] = FText::FromString(SelectedLayers[LayerIndex]->GetName());
	}
	Layers.Validate();

	const FScopedTransaction Transaction(LOCTEXT("ReplaceTestLayersTransaction", "替换 RVT 测试材质层"));
	TestMaterial->Modify();
	TestMaterial->SetMaterialLayers(Layers);
	TestMaterial->PostEditChange();
	if (!SaveAsset(TestMaterial, false))
	{
		SetStatus(LOCTEXT("TestSaveFailed", "测试材质已修改，但保存失败。"), true);
		return FReply::Handled();
	}
	SetStatus(FText::Format(LOCTEXT("TestReplaced", "已按当前动态列表写入 RVT 测试材质，共 {0} 个有效 Layer。"), FText::AsNumber(SelectedLayers.Num())));
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::CreatePreset(const bool bAssignToSelection)
{
	UMaterialFunctionMaterialLayerBlendInstance* DefaultBlend = LoadObject<UMaterialFunctionMaterialLayerBlendInstance>(nullptr, *BlendTemplatePath);
	UMaterialFunctionMaterialLayerInstance* Background = PresetLayerPaths[0].IsValid()
		? Cast<UMaterialFunctionMaterialLayerInstance>(PresetLayerPaths[0].TryLoad()) : nullptr;
	if (!Background || !DefaultBlend)
	{
		SetStatus(LOCTEXT("PresetMissingBackground", "Background MLI 必填，且默认 MLBI_BasicMatMask_Base 必须存在。"), true);
		return FReply::Handled();
	}

	FString AssetName = PresetNameTextBox->GetText().ToString().TrimStartAndEnd();
	FString DestinationPath = PresetFolderTextBox->GetText().ToString().TrimStartAndEnd();
	if (!AssetName.StartsWith(TEXT("MI_")))
	{
		AssetName = TEXT("MI_") + AssetName;
	}
	if (!FPackageName::IsValidLongPackageName(DestinationPath) || !DestinationPath.StartsWith(TEXT("/Game/")))
	{
		SetStatus(LOCTEXT("BadPresetFolder", "预设目录必须是有效的 /Game/... 路径。"), true);
		return FReply::Handled();
	}
	if (FPackageName::DoesPackageExist(DestinationPath + TEXT("/") + AssetName))
	{
		SetStatus(LOCTEXT("PresetExists", "目标预设已经存在，请换一个名称。"), true);
		return FReply::Handled();
	}

	FMaterialLayersFunctions Layers;
	Layers.AddDefaultBackgroundLayer();
	Layers.Layers[0] = Background;
	Layers.EditorOnly.LayerNames[0] = FText::FromString(Background->GetName());
	for (int32 SlotIndex = 1; SlotIndex < PresetLayerPaths.Num(); ++SlotIndex)
	{
		UMaterialFunctionMaterialLayerInstance* Layer = PresetLayerPaths[SlotIndex].IsValid()
			? Cast<UMaterialFunctionMaterialLayerInstance>(PresetLayerPaths[SlotIndex].TryLoad()) : nullptr;
		if (!Layer)
		{
			continue;
		}
		const int32 LayerIndex = Layers.AppendBlendedLayer();
		Layers.Layers[LayerIndex] = Layer;
		Layers.Blends[LayerIndex - 1] = DefaultBlend;
		Layers.EditorOnly.LayerNames[LayerIndex] = FText::FromString(Layer->GetName());
	}
	Layers.Validate();

	const FScopedTransaction Transaction(LOCTEXT("CreatePresetTransaction", "创建 RVT 层材质预设"));
	UMaterialInstanceConstant* Preset = DuplicateGroundMaterialInstance(DestinationPath, AssetName);
	if (!Preset)
	{
		SetStatus(LOCTEXT("PresetCreateFailed", "创建材质实例失败。"), true);
		return FReply::Handled();
	}
	Preset->Modify();
	Preset->SetMaterialLayers(Layers);
	Preset->PostEditChange();
	if (!SaveAsset(Preset, true))
	{
		SetStatus(LOCTEXT("PresetSaveFailed", "预设已创建但保存失败。"), true);
		return FReply::Handled();
	}
	LastCreatedPreset = Preset;

	int32 ActorCount = 0;
	int32 ComponentCount = 0;
	if (bAssignToSelection)
	{
		ApplyMaterialToSelectedActors(Preset, ActorCount, ComponentCount);
	}
	SetStatus(FText::Format(
		bAssignToSelection ? LOCTEXT("PresetCreatedAssigned", "已创建 {0}，并赋予 {1} 个 Actor、{2} 个 Primitive Component。地图仅标脏，未自动保存。") : LOCTEXT("PresetCreated", "已创建 RVT 层材质预设：{0}"),
		FText::FromString(Preset->GetPathName()), FText::AsNumber(ActorCount), FText::AsNumber(ComponentCount)));
	return FReply::Handled();
}

FReply SRVTMaterialManagerWidget::AssignLastPresetToSelection()
{
	if (!LastCreatedPreset.IsValid())
	{
		SetStatus(LOCTEXT("NoLastPreset", "当前会话还没有创建预设。"), true);
		return FReply::Handled();
	}
	int32 ActorCount = 0;
	int32 ComponentCount = 0;
	ApplyMaterialToSelectedActors(LastCreatedPreset.Get(), ActorCount, ComponentCount);
	SetStatus(FText::Format(LOCTEXT("AssignedLast", "已将 {0} 赋予 {1} 个 Actor、{2} 个 Primitive Component；未自动保存地图。"), FText::FromString(LastCreatedPreset->GetName()), FText::AsNumber(ActorCount), FText::AsNumber(ComponentCount)));
	return FReply::Handled();
}

bool SRVTMaterialManagerWidget::ApplyMaterialToSelectedActors(UMaterialInstanceConstant* Material, int32& OutActorCount, int32& OutComponentCount) const
{
	OutActorCount = 0;
	OutComponentCount = 0;
	if (!Material || !GEditor)
	{
		return false;
	}
	const FScopedTransaction Transaction(LOCTEXT("AssignPresetTransaction", "赋予 RVT 层材质预设"));
	for (FSelectionIterator It(*GEditor->GetSelectedActors()); It; ++It)
	{
		AActor* Actor = Cast<AActor>(*It);
		if (!Actor)
		{
			continue;
		}
		bool bChangedActor = false;
		TInlineComponentArray<UPrimitiveComponent*> Components(Actor);
		for (UPrimitiveComponent* Component : Components)
		{
			if (!Component || Component->GetNumMaterials() <= 0)
			{
				continue;
			}
			Component->Modify();
			for (int32 MaterialIndex = 0; MaterialIndex < Component->GetNumMaterials(); ++MaterialIndex)
			{
				Component->SetMaterial(MaterialIndex, Material);
			}
			Component->MarkRenderStateDirty();
			++OutComponentCount;
			bChangedActor = true;
		}
		if (bChangedActor)
		{
			Actor->MarkPackageDirty();
			++OutActorCount;
		}
	}
	return OutActorCount > 0;
}

FText SRVTMaterialManagerWidget::GetSummaryText() const
{
	return FText::Format(
		LOCTEXT("Summary", "{0} 基础 · {1} 变体 · {2} 分类"),
		FText::AsNumber(BaseEntries.Num()),
		FText::AsNumber(AllEntries.Num() - BaseEntries.Num()),
		FText::AsNumber(LibraryCategories.Num()));
}

FText SRVTMaterialManagerWidget::GetSelectedTitle() const
{
	return SelectedEntry.IsValid() ? FText::FromString(SelectedEntry->AssetName) : FText::GetEmpty();
}

FText SRVTMaterialManagerWidget::GetSelectedPath() const
{
	return SelectedEntry.IsValid() ? FText::FromName(SelectedEntry->AssetData.PackageName) : FText::GetEmpty();
}

FText SRVTMaterialManagerWidget::GetSelectedKindText() const
{
	if (!SelectedEntry.IsValid())
	{
		return FText::GetEmpty();
	}
	return SelectedEntry->bIsVariant
		? FText::Format(LOCTEXT("VariantKind", "变体材质 · 基础：{0}"), FText::FromString(SelectedEntry->BaseAssetName))
		: FText::Format(LOCTEXT("BaseKind", "基础材质 · {0} 个变体 · 分类：{1}"), FText::AsNumber(SelectedEntry->Variants.Num()), FText::FromString(SelectedEntry->Category));
}

FString SRVTMaterialManagerWidget::GetSelectedTextureObjectPath(const FName ParameterName) const
{
	UMaterialFunctionMaterialLayerInstance* Layer = SelectedEntry.IsValid() ? Cast<UMaterialFunctionMaterialLayerInstance>(SelectedEntry->AssetData.GetAsset()) : nullptr;
	if (UTexture* Texture = ResolveTextureParameter(Layer, ParameterName))
	{
		return Texture->GetPathName();
	}
	return FString();
}

FString SRVTMaterialManagerWidget::GetPresetLayerPath(const int32 SlotIndex) const
{
	return PresetLayerPaths.IsValidIndex(SlotIndex) ? PresetLayerPaths[SlotIndex].ToString() : FString();
}

SRVTMaterialManagerWidget::FEntryPtr SRVTMaterialManagerWidget::FindEntryByPath(const FSoftObjectPath& ObjectPath) const
{
	if (!ObjectPath.IsValid())
	{
		return nullptr;
	}

	for (const FEntryPtr& Entry : AllEntries)
	{
		if (Entry.IsValid() && Entry->AssetData.ToSoftObjectPath() == ObjectPath)
		{
			return Entry;
		}
	}
	return nullptr;
}

FText SRVTMaterialManagerWidget::GetPresetPaletteSummaryText() const
{
	return FText::Format(
		LOCTEXT("PresetPaletteSummary", "显示 {0} / {1} 个可拖拽 MLI"),
		FText::AsNumber(PresetPaletteEntries.Num()),
		FText::AsNumber(AllEntries.Num()));
}

void SRVTMaterialManagerWidget::OnPresetLayerChanged(const FAssetData& AssetData, const int32 SlotIndex)
{
	if (!PresetLayerPaths.IsValidIndex(SlotIndex))
	{
		return;
	}
	if (AssetData.IsValid() && !IsLayerAssetAllowed(AssetData))
	{
		SetStatus(LOCTEXT("PresetSlotRejected", "该资产不是 CommonTex 材质库中的 Material Layer Instance。"), true);
		return;
	}
	if (SlotIndex == 0 && !AssetData.IsValid())
	{
		SetStatus(LOCTEXT("BackgroundCannotClear", "Background 为必填层，不能清空。"), true);
		return;
	}

	PresetLayerPaths[SlotIndex] = AssetData.IsValid() ? AssetData.ToSoftObjectPath() : FSoftObjectPath();
	RebuildPresetLayerRows();
	const FText SlotLabel = SlotIndex == 0
		? LOCTEXT("ChangedBackground", "Background")
		: FText::Format(LOCTEXT("ChangedLayer", "Layer {0}"), FText::AsNumber(SlotIndex));
	SetStatus(AssetData.IsValid()
		? FText::Format(LOCTEXT("PresetSlotFilled", "已将 {0} 放入 {1}。"), FText::FromName(AssetData.AssetName), SlotLabel)
		: FText::Format(LOCTEXT("PresetSlotCleared", "已清空 {0}。"), SlotLabel));
}

bool SRVTMaterialManagerWidget::IsLayerAssetAllowed(const FAssetData& AssetData) const
{
	return AssetData.IsValid()
		&& AssetData.PackageName.ToString().StartsWith(LibraryRoot + TEXT("/"))
		&& AssetData.IsInstanceOf(UMaterialFunctionMaterialLayerInstance::StaticClass(), EResolveClass::Yes);
}

FString SRVTMaterialManagerWidget::NormalizeCategory(const FString& Category)
{
	FString Result = Category.TrimStartAndEnd();
	Result.ReplaceInline(TEXT("\\"), TEXT("/"));
	Result.RemoveFromStart(TEXT("/"));
	Result.RemoveFromEnd(TEXT("/"));
	return Result.IsEmpty() ? TEXT("Ground") : Result;
}

FString SRVTMaterialManagerWidget::MakeInferredLayerName(const FString& TextureCore)
{
	FString Core = TextureCore;
	Core.RemoveFromStart(TEXT("T_Yog_"));
	Core.RemoveFromStart(TEXT("T_"));
	return TEXT("MLI_") + Core;
}

UTexture* SRVTMaterialManagerWidget::ResolveTextureParameter(UMaterialFunctionMaterialLayerInstance* Layer, const FName ParameterName)
{
	for (UMaterialFunctionInstance* Current = Layer; Current; Current = Cast<UMaterialFunctionInstance>(Current->Parent))
	{
		for (const FTextureParameterValue& Parameter : Current->TextureParameterValues)
		{
			if (Parameter.ParameterInfo.Name == ParameterName && Parameter.ParameterValue)
			{
				return Parameter.ParameterValue;
			}
		}
	}
	return nullptr;
}

bool SRVTMaterialManagerWidget::SaveAsset(UObject* Asset, const bool bAddToSourceControl)
{
	if (!Asset || !GEditor)
	{
		return false;
	}
	UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	const bool bSaved = AssetSubsystem && AssetSubsystem->SaveLoadedAsset(Asset, false);
	if (bSaved && bAddToSourceControl)
	{
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		if (FPaths::FileExists(Filename))
		{
			SourceControlHelpers::MarkFileForAdd(Filename, false);
		}
	}
	return bSaved;
}

UMaterialFunctionMaterialLayerInstance* SRVTMaterialManagerWidget::DuplicateLayer(
	UMaterialFunctionMaterialLayerInstance* Source,
	const FString& DestinationPath,
	const FString& AssetName)
{
	return Source ? Cast<UMaterialFunctionMaterialLayerInstance>(FAssetToolsModule::GetModule().Get().DuplicateAsset(AssetName, DestinationPath, Source)) : nullptr;
}

UMaterialInstanceConstant* SRVTMaterialManagerWidget::DuplicateGroundMaterialInstance(const FString& DestinationPath, const FString& AssetName)
{
	UMaterialInstanceConstant* Base = LoadObject<UMaterialInstanceConstant>(nullptr, *GroundBaseMaterialPath);
	return Base ? Cast<UMaterialInstanceConstant>(FAssetToolsModule::GetModule().Get().DuplicateAsset(AssetName, DestinationPath, Base)) : nullptr;
}

void SRVTMaterialManagerWidget::SetStatus(const FText& Text, const bool bError)
{
	StatusColor = bError ? FSlateColor(FLinearColor(1.0f, 0.3f, 0.2f)) : FSlateColor(FLinearColor(0.3f, 0.8f, 1.0f));
	if (StatusTextBlock.IsValid())
	{
		StatusTextBlock->SetText(Text);
	}
}

#undef LOCTEXT_NAMESPACE
