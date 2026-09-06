#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STileView.h"

class ADevKitDecalCollectionActor;
class UDevKitDecalAsset;
class UMaterialInterface;
class FLevelEditorViewportClient;
class FAssetThumbnail;
class FAssetThumbnailPool;
enum class EDevKitDecalBackend : uint8;
struct FAssetData;
struct FDevKitDecalPlacementRecord;
struct FDevKitDecalPaletteItem;
class SVerticalBox;

using FDevKitDecalPaletteItemPtr = TSharedPtr<FDevKitDecalPaletteItem>;

class SDevKitDecalCollectionWidget final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitDecalCollectionWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	void RefreshCollections();
	void RefreshCollectionRows();
	FReply CreateCollection();
	FReply SelectCollection(TWeakObjectPtr<ADevKitDecalCollectionActor> Collection);
	FReply EnterSelectedCollection();
	FReply AdoptSelectedDeferredDecal();
	FReply AdoptSelectedInstancedMesh();
	FReply RestoreSelectedRecordSource();
	FReply OpenLegacyRVTLibrary();
	FReply SelectSection(int32 SectionIndex);
	FReply SelectPaletteBackendFilter(int32 BackendFilter);
	FReply SelectPaletteValidityFilter(int32 ValidityFilter);
	FReply SelectPaletteUsageFilter(int32 UsageFilter);
	FReply SelectPaletteSortMode(int32 SortMode);
	FReply SelectPaletteAsset(TWeakObjectPtr<UDevKitDecalAsset> Asset);
	FReply BeginPaletteAssetDrag(TWeakObjectPtr<UDevKitDecalAsset> Asset);
	FReply CreateMaterialVariant(TWeakObjectPtr<UDevKitDecalAsset> SourceAsset);
	FReply CreateNewDecalAsset();
	FReply CreateNewModelAsset();
	FReply CreateISMVariant(TWeakObjectPtr<UDevKitDecalAsset> SourceAsset);
	FReply PlacePaletteAssetAtViewportCenter(TWeakObjectPtr<UDevKitDecalAsset> Asset);
	FReply TogglePaletteBrush(TWeakObjectPtr<UDevKitDecalAsset> Asset);
	bool HandlePaletteAssetDrop(TWeakObjectPtr<UDevKitDecalAsset> Asset, FLevelEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY);
	void OnPaletteMeshChanged(const FAssetData& AssetData, TWeakObjectPtr<UDevKitDecalAsset> TargetAsset);
	void OnPaletteMaterialChanged(const FAssetData& AssetData, TWeakObjectPtr<UDevKitDecalAsset> TargetAsset);
	void OnSelectedInstanceMaterialChanged(const FAssetData& AssetData);
	void OnPaletteSearchChanged(const FText& SearchText);
	FReply BakeSelectedInstance();
	FText GetSelectedInstanceSummary() const;
	FString GetSelectedInstanceMaterialPath() const;
	FText GetPaletteCountText() const;
	FText GetSelectedPaletteSummary() const;
	FText GetPaletteBackendFilterLabel() const;
	FText GetPaletteFilterSummary() const;
	int32 GetFilteredPaletteCount() const;
	int32 GetPaletteAssetInstanceCount(const UDevKitDecalAsset* Asset) const;
	bool MatchesPaletteFilter(const UDevKitDecalAsset* Asset) const;
	bool GetSelectedInstanceRecord(FDevKitDecalPlacementRecord& OutRecord) const;
	FReply CreateNewAssetDefinition(EDevKitDecalBackend Backend);
	TSharedRef<ITableRow> GeneratePaletteTile(FDevKitDecalPaletteItemPtr Item, const TSharedRef<STableViewBase>& OwnerTable);
	void OnPaletteTileSelectionChanged(FDevKitDecalPaletteItemPtr Item, ESelectInfo::Type SelectInfo);
	void RefreshPaletteRows();
	void UpdateSelectedPaletteThumbnail();
	ADevKitDecalCollectionActor* GetTargetCollection() const;
	FText GetCollectionSummary() const;
	FText GetSectionTitle() const;
	FText GetSectionHelp() const;
	FText GetActionStatus() const;
	FText GetAuditSummary() const;

	TArray<TWeakObjectPtr<ADevKitDecalCollectionActor>> Collections;
	TWeakObjectPtr<ADevKitDecalCollectionActor> SelectedCollection;
	TArray<TWeakObjectPtr<UDevKitDecalAsset>> PaletteAssets;
	TMap<const UDevKitDecalAsset*, int32> PaletteInstanceCounts;
	TWeakObjectPtr<UDevKitDecalAsset> SelectedPaletteAsset;
	TWeakObjectPtr<UMaterialInterface> SelectedInstanceMaterial;
	FGuid LastSelectedInstanceGuid;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<FAssetThumbnail> SelectedPaletteThumbnail;
	TArray<FDevKitDecalPaletteItemPtr> FilteredPaletteItems;
	TSharedPtr<STileView<FDevKitDecalPaletteItemPtr>> PaletteTileView;
	FText PaletteEmptyMessage;
	TSharedPtr<SVerticalBox> CollectionRows;
	FString PaletteSearchText;
	/** -1 is every backend; otherwise the numeric EDevKitDecalBackend value. */
	int32 PaletteBackendFilter = -1;
	/** -1 is every asset, 1 is ready, 0 is incomplete. */
	int32 PaletteValidityFilter = -1;
	/** -1 is every asset, 1 is used by enabled records, 0 is an empty batch. */
	int32 PaletteUsageFilter = -1;
	/** 0 keeps Collection order, 1 sorts by display name, 2 sorts by instance count. */
	int32 PaletteSortMode = 0;
	/** Open directly into the artist placement workflow rather than administration. */
	int32 ActiveSection = 1;
	FString ActionStatus;
};
