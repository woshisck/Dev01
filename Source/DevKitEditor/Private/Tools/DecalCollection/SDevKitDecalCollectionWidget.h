#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class ADevKitDecalCollectionActor;
class UDevKitDecalAsset;
class UMaterialInterface;
class FLevelEditorViewportClient;
class FAssetThumbnailPool;
class SWrapBox;
enum class EDevKitDecalBackend : uint8;
struct FAssetData;
struct FDevKitDecalPlacementRecord;
class SVerticalBox;

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
	void OnPaletteDisplayLimitChanged(float SliderValue);
	void OnPaletteBrowseOffsetChanged(float SliderValue);
	FReply BakeSelectedInstance();
	FText GetSelectedInstanceSummary() const;
	FString GetSelectedInstanceMaterialPath() const;
	FText GetPaletteCountText() const;
	FText GetSelectedPaletteSummary() const;
	FText GetPaletteBackendFilterLabel() const;
	FText GetPaletteDisplayLimitText() const;
	float GetPaletteDisplayLimitSliderValue() const;
	int32 GetPaletteDisplayLimitUpperBound() const;
	FText GetPaletteBrowseRangeText() const;
	float GetPaletteBrowseSliderValue() const;
	int32 GetPaletteBrowseOffsetUpperBound() const;
	int32 GetFilteredPaletteCount() const;
	bool MatchesPaletteFilter(const UDevKitDecalAsset* Asset) const;
	bool GetSelectedInstanceRecord(FDevKitDecalPlacementRecord& OutRecord) const;
	FReply CreateNewAssetDefinition(EDevKitDecalBackend Backend);
	void RefreshPaletteRows();
	ADevKitDecalCollectionActor* GetTargetCollection() const;
	FText GetCollectionSummary() const;
	FText GetSectionTitle() const;
	FText GetSectionHelp() const;
	FText GetActionStatus() const;
	FText GetAuditSummary() const;

	TArray<TWeakObjectPtr<ADevKitDecalCollectionActor>> Collections;
	TWeakObjectPtr<ADevKitDecalCollectionActor> SelectedCollection;
	TArray<TWeakObjectPtr<UDevKitDecalAsset>> PaletteAssets;
	TWeakObjectPtr<UDevKitDecalAsset> SelectedPaletteAsset;
	TWeakObjectPtr<UMaterialInterface> SelectedInstanceMaterial;
	FGuid LastSelectedInstanceGuid;
	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<SWrapBox> PaletteRows;
	TSharedPtr<SVerticalBox> CollectionRows;
	FString PaletteSearchText;
	/** -1 is every backend; otherwise the numeric EDevKitDecalBackend value. */
	int32 PaletteBackendFilter = -1;
	int32 PaletteDisplayLimit = 12;
	int32 PaletteBrowseOffset = 0;
	/** Open directly into the artist placement workflow rather than administration. */
	int32 ActiveSection = 1;
	FString ActionStatus;
};
