#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "RVT/DevKitRVTSurfaceAsset.h"
#include "Tools/RVTMeshDecal/DevKitRVTMeshDecalService.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"

class ADevKitRVTSurfaceInstanceActor;
class SEditableTextBox;
class SSearchBox;
class SWidgetSwitcher;
class UDevKitRVTSurfaceAsset;
class UWorld;

/**
 * RVT surface library.
 *
 * The normal entry point is the two-column use view.  Asset authoring lives in
 * a separate three-column view so mesh/material selection cannot be confused
 * with the reusable, finished surface assets shown to level artists.
 */
class SDevKitRVTMeshDecalWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDevKitRVTMeshDecalWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

#if WITH_DEV_AUTOMATION_TESTS
	bool IsUseViewActiveForAutomation() const;
#endif

	// Called by the draggable asset cards declared in the implementation file.
	void SelectSurfaceAssetFromCard(const TSharedPtr<FAssetData>& Item);
	FReply BeginSurfaceAssetDrag(const TSharedPtr<FAssetData>& Item);

private:
	enum class EViewMode : uint8
	{
		Use,
		Edit
	};

	enum class ELibraryFilter : uint8
	{
		All,
		PlaneDecal,
		VisibleObject
	};

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildUseView();
	TSharedRef<SWidget> BuildLibraryPanel();
	TSharedRef<SWidget> BuildUsePanel();
	TSharedRef<SWidget> BuildEditView();
	TSharedRef<SWidget> BuildMeshPickerPanel();
	TSharedRef<SWidget> BuildMaterialPickerPanel();
	TSharedRef<SWidget> BuildAssetSettingsPanel();
	TSharedRef<SWidget> CreateMeshPicker();
	TSharedRef<SWidget> CreateMaterialPicker();

	UWorld* GetEditorWorld() const;
	FString GetCurrentWorldPackagePath() const;
	void RefreshWorldContext(bool bResetDefaultFolder);
	void RefreshAutomaticRVTBindings();

	void RefreshSurfaceAssetLibrary();
	void ApplyLibraryFilter();
	bool DoesAssetPassLibraryFilter(const FAssetData& AssetData) const;
	TSharedRef<ITableRow> GenerateSurfaceAssetRow(
		TSharedPtr<FAssetData> Item,
		const TSharedRef<STableViewBase>& OwnerTable);
	void OnSurfaceAssetSelectionChanged(TSharedPtr<FAssetData> Item, ESelectInfo::Type SelectInfo);
	void OnSurfaceAssetDoubleClicked(TSharedPtr<FAssetData> Item);
	void OnLibrarySearchChanged(const FText& NewText);
	ECheckBoxState IsLibraryFilterChecked(ELibraryFilter Filter) const;
	void OnLibraryFilterChanged(ECheckBoxState NewState, ELibraryFilter Filter);

	UDevKitRVTSurfaceAsset* GetSelectedSurfaceAsset() const;
	ADevKitRVTSurfaceInstanceActor* FindSelectedSurfaceInstanceActor() const;
	FText GetSelectedAssetTitle() const;
	FText GetSelectedAssetSummary() const;
	FText GetAutomaticRVTText() const;
	FText GetControllerSummary() const;
	bool HasSelectedSurfaceAsset() const;
	FReply RefreshLibrary();
	FReply EditSelectedSurfaceAsset();
	FReply CreateNewSurfaceAsset();
	FReply CreateOrSelectControllerActor();
	FReply PlaceSingleInstanceAtViewportCenter();
	void HandleControllerResult(const FDevKitRVTSurfaceControllerResult& Result);

	ECheckBoxState IsViewModeChecked(EViewMode Mode) const;
	void OnViewModeChanged(ECheckBoxState NewState, EViewMode Mode);
	void SetViewMode(EViewMode Mode);
	void ResetEditForm();
	void LoadEditFormFromAsset(UDevKitRVTSurfaceAsset* Asset);
	void UpdateSuggestedAssetName();
	FDevKitRVTSurfaceAssetRequest BuildSurfaceAssetRequest() const;
	bool CanSaveSurfaceAsset() const;
	FReply SaveSurfaceAsset();
	FReply ReturnToUseView();

	void OnMeshSelected(const FAssetData& AssetData);
	void OnMaterialSelected(const FAssetData& AssetData);
	FText GetSelectedMeshText() const;
	FText GetSelectedMaterialText() const;
	ECheckBoxState IsEditAssetTypeChecked(EDevKitRVTSurfaceAssetType Type) const;
	void OnEditAssetTypeChanged(ECheckBoxState NewState, EDevKitRVTSurfaceAssetType Type);
	ECheckBoxState IsEditGeometryPolicyChecked(EDevKitRVTSurfaceGeometryPolicy Policy) const;
	void OnEditGeometryPolicyChanged(ECheckBoxState NewState, EDevKitRVTSurfaceGeometryPolicy Policy);
	ECheckBoxState IsEditFlagChecked(bool SDevKitRVTMeshDecalWidget::* Flag) const;
	void OnEditFlagChanged(ECheckBoxState NewState, bool SDevKitRVTMeshDecalWidget::* Flag);

	FText GetStatusText() const;
	FSlateColor GetStatusColor() const;
	void SetStatus(const FText& InStatus, bool bIsError);

	EViewMode ViewMode = EViewMode::Use;
	ELibraryFilter LibraryFilter = ELibraryFilter::All;
	TSharedPtr<SWidgetSwitcher> ViewSwitcher;
	TSharedPtr<SListView<TSharedPtr<FAssetData>>> SurfaceAssetListView;
	TSharedPtr<SSearchBox> LibrarySearchBox;
	TArray<TSharedPtr<FAssetData>> AllSurfaceAssets;
	TArray<TSharedPtr<FAssetData>> FilteredSurfaceAssets;
	TSharedPtr<FAssetData> SelectedSurfaceAssetItem;
	FString LibrarySearchText;

	TSharedPtr<SEditableTextBox> AssetFolderTextBox;
	TSharedPtr<SEditableTextBox> AssetNameTextBox;
	TSharedPtr<SEditableTextBox> DisplayNameTextBox;
	TWeakObjectPtr<UDevKitRVTSurfaceAsset> EditingSurfaceAsset;
	FString EditMeshObjectPath;
	FString EditMaterialObjectPath;
	FString EditAssetFolder;
	FString EditAssetName;
	FText EditDisplayName;
	FText EditDescription;
	EDevKitRVTSurfaceAssetType EditAssetType = EDevKitRVTSurfaceAssetType::PlaneDecal;
	EDevKitRVTSurfaceGeometryPolicy EditGeometryPolicy = EDevKitRVTSurfaceGeometryPolicy::RVTOnly;
	int32 EditPriority = 0;
	FVector EditDefaultScale = FVector::OneVector;
	float EditZOffset = 0.5f;
	bool bEditAlignToNormal = true;
	bool bEditRandomYaw = true;
	bool bEditBindWorldHeight = false;
	bool bEditEnableCollision = false;
	bool bEditCastShadow = false;

	FString LastWorldPackagePath;
	FString AutomaticSurfaceRVTPath;
	FString AutomaticHeightRVTPath;
	FText AutomaticRVTMessage;
	FText StatusText;
	bool bStatusIsError = false;
};
