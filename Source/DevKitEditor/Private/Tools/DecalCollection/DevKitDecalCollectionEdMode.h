#pragma once

#include "ScopedTransaction.h"
#include "Tools/LegacyEdModeWidgetHelpers.h"
#include "Surface/DevKitDecalCollectionActor.h"
#include "DevKitDecalCollectionEdMode.generated.h"

class ADevKitDecalCollectionActor;
class UDevKitDecalAsset;
class UMaterialInterface;
class FLevelEditorViewportClient;
class HHitProxy;
struct FViewportClick;

UCLASS(Transient)
class DEVKITEDITOR_API UDevKitDecalCollectionEdMode : public UBaseLegacyWidgetEdMode
{
	GENERATED_BODY()

public:
	static const FEditorModeID EM_DevKitDecalCollection;

	UDevKitDecalCollectionEdMode();

	virtual void Enter() override;
	virtual void Exit() override;
	virtual void ModeTick(float DeltaTime) override;
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override;
	virtual bool IsEditingDisallowed(AActor* InActor) const override;
	virtual bool ProcessEditDuplicate() override;
	virtual bool ProcessEditDelete() override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click) override;
	virtual bool InputKey(FEditorViewportClient* InViewportClient, FViewport* InViewport, FKey Key, EInputEvent Event) override;
	virtual bool InputAxis(FEditorViewportClient* InViewportClient, FViewport* InViewport, int32 ControllerId, FKey Key, float Delta, float DeltaTime) override;
	virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag, FRotator& InRot, FVector& InScale) override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	/**
	 * The modern typed-element selection path does not automatically opt a
	 * UEdMode into the legacy viewport widget contract.  The collection uses
	 * UBaseLegacyWidgetEdMode as a small compatibility bridge so the native
	 * editor gizmo can drive the selected ISM instance.  These overrides keep
	 * the widget tied to the typed-element selection, not to actor selection.
	 */
	virtual bool AllowWidgetMove() override;
	virtual EAxisList::Type GetWidgetAxisToDraw(UE::Widget::EWidgetMode InWidgetMode) const override;
	virtual FVector GetWidgetLocation() const override;
	virtual bool ShouldDrawWidget() const override;
	virtual bool UsesTransformWidget() const override;
	virtual bool UsesTransformWidget(UE::Widget::EWidgetMode CheckMode) const override;

	ADevKitDecalCollectionActor* GetActiveCollection() const;
	/** Queues an explicit target before ActivateMode so Enter cannot race selection propagation. */
	static void RequestCollectionForActivation(ADevKitDecalCollectionActor* Collection);
	/** Selects a Collection as the active edit target even when the Mode was already active. */
	bool BeginEditingCollection(ADevKitDecalCollectionActor* Collection);
	/** Places one palette asset at the active perspective viewport center. */
	bool PlaceAssetAtViewportCenter(UDevKitDecalAsset* Asset);
	/** Places one palette asset at a viewport-local cursor position (drag/drop). */
	bool PlaceAssetAtViewportCursor(UDevKitDecalAsset* Asset, FLevelEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY);
	/** Enables/disables surface brush placement for one selected palette asset. */
	bool ToggleBrushPlacement(UDevKitDecalAsset* Asset);
	bool IsBrushPlacementActiveFor(const UDevKitDecalAsset* Asset) const;
	void SetBrushSpacing(float InSpacing);
	float GetBrushSpacing() const { return BrushSpacing; }
	/** Returns the authored record behind the current ISM selection. */
	bool GetSelectedInstanceDetails(FDevKitDecalPlacementRecord& OutRecord) const;
	/** Applies a material preview to only the selected record/batch. */
	bool SetSelectedInstanceMaterialOverride(UMaterialInterface* Material);
	/** Rebinds the selected record to a baked material-variant asset. */
	bool BakeSelectedInstanceAsset(UDevKitDecalAsset* NewAsset);
	void ApplyAndExit();
	void CancelAndExit();

protected:
	virtual void CreateToolkit() override;

private:
	TWeakObjectPtr<ADevKitDecalCollectionActor> SessionCollection;
	TArray<FDevKitDecalPlacementRecord> SessionRecords;
	bool bSessionSnapshotValid = false;
	bool bAcceptOnExit = true;
	FGuid SelectedDeferredRecordGuid;
	TWeakObjectPtr<class UDecalComponent> SelectedDeferredComponent;
	TUniquePtr<FScopedTransaction> DeferredTransformTransaction;
	TWeakObjectPtr<UDevKitDecalAsset> BrushPlacementAsset;
	TUniquePtr<FScopedTransaction> BrushPlacementTransaction;
	FVector BrushLastPlacement = FVector::ZeroVector;
	float BrushSpacing = 100.f;
	bool bBrushPlacementEnabled = false;
	bool bBrushStrokeActive = false;
	bool bHasBrushLastPlacement = false;

	bool GetSelectedInstanceTransform(FTransform& OutTransform) const;
	bool GetSelectedInstanceRecord(FGuid& OutRecordGuid, FDevKitDecalPlacementRecord& OutRecord) const;
	bool GetSelectedDeferredRecord(FGuid& OutRecordGuid, FDevKitDecalPlacementRecord& OutRecord) const;
	bool TrySelectDeferredRecord(const FViewportClick& Click);
	void RestoreSessionAdoptionSourceVisibility(ADevKitDecalCollectionActor* Collection) const;
	bool ResolvePlacementTransform(UDevKitDecalAsset* Asset, FEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY, bool bRequireSurfaceHit, FTransform& OutTransform) const;
	bool AddPlacementRecord(UDevKitDecalAsset* Asset, const FTransform& PlacementTransform);
	bool PaintBrushAtViewportCoordinates(FEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY);
	bool PlaceAssetAtViewportCoordinates(UDevKitDecalAsset* Asset, FLevelEditorViewportClient* ViewportClient, int32 ViewportX, int32 ViewportY, bool bRequireSurfaceHit);
};
