#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"
#include "Widgets/Views/STreeView.h"

class FAssetThumbnail;
class FAssetThumbnailPool;
class SBox;
class SEditableTextBox;
class STextBlock;
class SVerticalBox;
class SWidgetSwitcher;
class SWrapBox;
class UMaterialFunctionMaterialLayerInstance;
class UMaterialInstanceConstant;
class UTexture;

struct FRVTMaterialLibraryEntry
{
	FAssetData AssetData;
	FString AssetName;
	FString Category;
	FString BaseAssetName;
	bool bIsVariant = false;
	TArray<TSharedPtr<FRVTMaterialLibraryEntry>> Variants;
	TSharedPtr<FAssetThumbnail> Thumbnail;
};

struct FRVTMaterialLibraryTreeNode
{
	FString DisplayName;
	FString Category;
	bool bIsCategory = false;
	TSharedPtr<FRVTMaterialLibraryEntry> Entry;
	TArray<TSharedPtr<FRVTMaterialLibraryTreeNode>> Children;
};

class SRVTMaterialManagerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SRVTMaterialManagerWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	void SelectEntry(TSharedPtr<FRVTMaterialLibraryEntry> Entry);
	FReply BeginEntryDrag(TSharedPtr<FRVTMaterialLibraryEntry> Entry) const;
	static UTexture* ResolveTextureParameter(UMaterialFunctionMaterialLayerInstance* Layer, FName ParameterName);

private:
	using FEntryPtr = TSharedPtr<FRVTMaterialLibraryEntry>;
	using FTreeNodePtr = TSharedPtr<FRVTMaterialLibraryTreeNode>;

	TSharedRef<SWidget> BuildLibraryView();
	TSharedRef<SWidget> BuildLibraryToolbar();
	TSharedRef<SWidget> BuildLibraryShelf();
	TSharedRef<SWidget> BuildLibraryCategoryFilters();
	TSharedRef<SWidget> BuildLibraryCategoryFilterButton(const FString& Category);
	TSharedRef<SWidget> BuildDetailPanel();
	TSharedRef<SWidget> BuildSelectedDetailContent();
	TSharedRef<SWidget> BuildTextureResourceRow(FName ParameterName, const FText& DisplayName, const FText& Description);
	TSharedRef<SWidget> BuildPresetView();
	TSharedRef<SWidget> BuildPresetPalette();
	TSharedRef<SWidget> BuildPresetLayerSlot(int32 SlotIndex, const FText& Label);
	void RebuildPresetLayerRows();
	void RebuildPresetPalette();
	void RebuildLibraryCategoryButtons();
	void RebuildLibraryTree();
	TSharedRef<ITableRow> GenerateLibraryTreeRow(FTreeNodePtr Node, const TSharedRef<STableViewBase>& OwnerTable);
	void GetLibraryTreeChildren(FTreeNodePtr Node, TArray<FTreeNodePtr>& OutChildren) const;
	void OnLibraryTreeSelectionChanged(FTreeNodePtr Node, ESelectInfo::Type SelectInfo);
	void OnLibraryTreeDoubleClicked(FTreeNodePtr Node);
	void OnVariantDoubleClicked(FEntryPtr Entry);
	FReply HandleMaterialLibraryKeyDown(const FGeometry& Geometry, const FKeyEvent& KeyEvent);
	TSharedRef<ITableRow> GenerateShelfTile(FEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateVariantTile(FEntryPtr Entry, const TSharedRef<STableViewBase>& OwnerTable);

	FReply RefreshLibrary();
	FReply OpenSelectedAsset();
	FReply SyncSelectedAssetToContentBrowser();
	FReply CreateVariant();
	FReply ImportOrUpdateTextures();
	FReply ShowLibraryView();
	FReply ShowPresetView();
	FReply ReplaceTestMaterialLayers();
	FReply CreatePreset(bool bAssignToSelection);
	FReply AssignLastPresetToSelection();
	FReply AddPresetLayer();
	FReply RemovePresetLayer(int32 SlotIndex);
	FReply MovePresetLayer(int32 SlotIndex, int32 Direction);
	void OnPresetPaletteSearchTextChanged(const FText& InSearchText);
	void OnPresetLayerDropped(const FAssetData& AssetData, int32 SlotIndex);
	void AutoPlacePresetLayer(const FAssetData& AssetData);

	void ScanLibrary();
	void OnLibrarySearchTextChanged(const FText& InSearchText);
	void OnLibraryCategoryFilterChanged(ECheckBoxState NewState, FString Category);
	ECheckBoxState IsLibraryCategoryFilterChecked(FString Category) const;
	bool PassesLibraryFilter(const FEntryPtr& Entry) const;
	void RefreshDetailPanel();
	void SetStatus(const FText& Text, bool bError = false);
	FText GetSummaryText() const;
	FText GetSelectedTitle() const;
	FText GetSelectedPath() const;
	FText GetSelectedKindText() const;
	FString GetSelectedTextureObjectPath(FName ParameterName) const;
	FString GetPresetLayerPath(int32 SlotIndex) const;
	FEntryPtr FindEntryByPath(const FSoftObjectPath& ObjectPath) const;
	FText GetPresetPaletteSummaryText() const;
	void OnPresetLayerChanged(const FAssetData& AssetData, int32 SlotIndex);
	bool IsLayerAssetAllowed(const FAssetData& AssetData) const;

	static FString NormalizeCategory(const FString& Category);
	static FString MakeInferredLayerName(const FString& TextureCore);
	static bool SaveAsset(UObject* Asset, bool bAddToSourceControl);
	static UMaterialFunctionMaterialLayerInstance* DuplicateLayer(
		UMaterialFunctionMaterialLayerInstance* Source,
		const FString& DestinationPath,
		const FString& AssetName);
	static UMaterialInstanceConstant* DuplicateGroundMaterialInstance(
		const FString& DestinationPath,
		const FString& AssetName);
	bool ApplyMaterialToSelectedActors(UMaterialInstanceConstant* Material, int32& OutActorCount, int32& OutComponentCount) const;

	TArray<FEntryPtr> AllEntries;
	TArray<FEntryPtr> BaseEntries;
	TArray<FEntryPtr> CurrentVariants;
	TArray<FEntryPtr> PresetPaletteEntries;
	TArray<FTreeNodePtr> LibraryTreeRootNodes;
	TArray<FString> LibraryCategories;
	TSet<FString> EnabledLibraryCategories;
	TSharedPtr<STreeView<FTreeNodePtr>> LibraryTreeView;
	TSharedPtr<STileView<FEntryPtr>> VariantView;
	TSharedPtr<STileView<FEntryPtr>> PresetPaletteView;
	TSharedPtr<SWrapBox> LibraryCategoryWrapBox;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<SWidgetSwitcher> ViewSwitcher;
	TSharedPtr<SBox> DetailContentBox;
	TSharedPtr<STextBlock> StatusTextBlock;
	TSharedPtr<SEditableTextBox> CategoryTextBox;
	TSharedPtr<SEditableTextBox> TargetLayerNameTextBox;
	TSharedPtr<SEditableTextBox> VariantSuffixTextBox;
	TSharedPtr<SEditableTextBox> PresetNameTextBox;
	TSharedPtr<SEditableTextBox> PresetFolderTextBox;
	TSharedPtr<SVerticalBox> PresetLayersBox;
	FEntryPtr SelectedEntry;
	TArray<FSoftObjectPath> PresetLayerPaths;
	TWeakObjectPtr<UMaterialInstanceConstant> LastCreatedPreset;
	FString LibrarySearchText;
	FString PresetPaletteSearchText;
	FSlateColor StatusColor = FSlateColor::UseForeground();
};
