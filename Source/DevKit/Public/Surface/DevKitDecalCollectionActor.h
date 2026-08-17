#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Surface/DevKitDecalAsset.h"
#include "DevKitDecalCollectionActor.generated.h"

class UInstancedStaticMeshComponent;
class UDecalComponent;
class UMaterialInterface;

USTRUCT(BlueprintType)
struct DEVKIT_API FDevKitDecalPlacementRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	FGuid InstanceGuid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	TObjectPtr<UDevKitDecalAsset> Asset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	FTransform Transform;

	/** Per-placement projector size. Only consumed by DeferredProjection. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	FVector DecalSize = FVector(64.f, 64.f, 64.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	TArray<float> CustomData;

	/**
	 * A temporary per-instance material while the instance is being authored.
	 * It deliberately does not mutate the shared asset batch.  The editor's
	 * Bake action converts this override into a new UDevKitDecalAsset/ISM batch.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement|Material")
	TObjectPtr<UMaterialInterface> MaterialOverride = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	FGameplayTagContainer Tags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	bool bEnabled = true;

	/** Editor provenance for an explicitly adopted legacy component. Empty means authored directly in this Collection. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement|Source")
	FString SourceActorPath;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement|Source")
	FName SourceComponentName;

	/** The source was hidden by this collection on Adopt and can be restored without guessing ownership. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Placement|Source")
	bool bSourceHiddenForAdoption = false;

	bool IsValid() const { return InstanceGuid.IsValid() && Asset != nullptr; }
};

UCLASS(BlueprintType, ClassGroup = (DevKit), meta = (DisplayName = "Decal Collection Records"))
class DEVKIT_API UDevKitDecalCollectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDevKitDecalCollectionComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal Collection")
	TArray<FDevKitDecalPlacementRecord> Records;

	/**
	 * Palette entries exposed by the Collection Mode.  A palette entry is an
	 * authored mesh/material batch definition; records reference the entry and
	 * the derived ISM backend groups by that asset.  Keeping the list on the
	 * component lets a mesh/material variant exist before its first placement.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal Collection|Palette")
	TArray<TObjectPtr<UDevKitDecalAsset>> PaletteAssets;

	UFUNCTION(BlueprintCallable, Category = "Decal Collection")
	FGuid AddRecord(UDevKitDecalAsset* Asset, const FTransform& WorldTransform);

	UFUNCTION(BlueprintCallable, Category = "Decal Collection")
	bool AddPaletteAsset(UDevKitDecalAsset* Asset);

	UFUNCTION(BlueprintCallable, Category = "Decal Collection")
	bool RemoveRecord(const FGuid& InstanceGuid);

	UFUNCTION(BlueprintCallable, Category = "Decal Collection")
	bool UpdateRecordTransform(const FGuid& InstanceGuid, const FTransform& WorldTransform);

	UFUNCTION(BlueprintPure, Category = "Decal Collection")
	int32 GetRecordCount() const { return Records.Num(); }

	FDevKitDecalPlacementRecord* FindRecord(const FGuid& InstanceGuid);
	const FDevKitDecalPlacementRecord* FindRecord(const FGuid& InstanceGuid) const;
};

/** One level/region authoring owner. Render components are derived from Records. */
UCLASS(BlueprintType, meta = (DisplayName = "Decal Collection Actor"))
class DEVKIT_API ADevKitDecalCollectionActor : public AActor
{
	GENERATED_BODY()

public:
	ADevKitDecalCollectionActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Decal Collection")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Decal Collection")
	TObjectPtr<UDevKitDecalCollectionComponent> Collection;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal Collection", AssetRegistrySearchable)
	FGuid CollectionGuid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal Collection", AssetRegistrySearchable)
	FName CollectionName = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Decal Collection")
	FText Description;

	UPROPERTY(Transient)
	bool bEditSessionActive = false;

	/** Runtime/editor-derived RVT ISM components. Placement Records remain authoritative. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UInstancedStaticMeshComponent>> DerivedRVTComponents;

	/** Runtime/editor-derived Deferred Decal proxies. These are deliberately individual components, not fake ISM batches. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UDecalComponent>> DerivedDeferredComponents;

	/** Stable authoring-record identity for each transient derived ISM instance. */
	TArray<TArray<FGuid>> DerivedInstanceGuids;

	/** Stable authoring-record identity for each individual Deferred Decal component. */
	TArray<FGuid> DerivedDeferredGuids;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Decal Collection|Edit", meta = (DisplayName = "Rebuild Derived Rendering"))
	void RebuildDerivedRendering();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Decal Collection|Edit", meta = (DisplayName = "Validate Collection"))
	bool ValidateCollection();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Decal Collection|Edit", meta = (DisplayName = "Begin Edit Session"))
	void BeginEditSession();

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Decal Collection|Edit", meta = (DisplayName = "End Edit Session"))
	void EndEditSession(bool bApply = true);

	/** Updates the Collection-mode selection policy and refreshes per-instance hit proxies. */
	void SetDerivedInstanceEditingEnabled(bool bEnabled);

	/** Called by derived ISM components after UE moves an instance. */
	bool UpdateDerivedInstanceTransform(UInstancedStaticMeshComponent* Component, int32 InstanceIndex, const FTransform& WorldTransform);

	/** Resolve a transient derived ISM instance back to its authored record. */
	bool FindRecordForDerivedInstance(UInstancedStaticMeshComponent* Component, int32 InstanceIndex, FGuid& OutRecordGuid) const;

	/** Resolve one transient Deferred Decal proxy back to its authored record. */
	bool FindRecordForDerivedDeferred(UDecalComponent* Component, FGuid& OutRecordGuid) const;

	/** Updates both an authored Deferred Decal record and its live proxy transform. */
	bool UpdateDerivedDeferredTransform(UDecalComponent* Component, const FTransform& WorldTransform);

	/** Set a material preview override on one record without changing its shared asset. */
	bool SetRecordMaterialOverride(const FGuid& InstanceGuid, UMaterialInterface* Material);

	/** Rebind one record to a new asset (used by the editor Bake operation). */
	bool RebindRecordAsset(const FGuid& InstanceGuid, UDevKitDecalAsset* NewAsset);

	/** Explicitly adopts one existing Deferred Decal component; the source is hidden, never deleted. */
	bool AdoptDeferredDecal(class UDecalComponent* SourceComponent, UDevKitDecalAsset* DeferredAsset, FGuid& OutRecordGuid);

	/** Explicitly adopts one compatible ISM batch; every source instance becomes a world-space Placement Record. */
	bool AdoptInstancedMesh(class UInstancedStaticMeshComponent* SourceComponent, UDevKitDecalAsset* MeshAsset, int32& OutAdoptedCount);

	/** Restores the original source component for one adopted record and disables its derived copy. */
	bool RestoreAdoptedSource(const FGuid& InstanceGuid);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	void ClearDerivedRendering();
	void BuildDerivedRendering();
};

/**
 * Transient ISM backend used by ADevKitDecalCollectionActor.
 * It keeps UE's native typed-element W/E/R editing while synchronizing the
 * authoritative Placement Record transform instead of losing edits on rebuild.
 */
UCLASS(ClassGroup = (DevKit), Transient)
class DEVKIT_API UDevKitDecalCollectionISMComponent : public UInstancedStaticMeshComponent
{
	GENERATED_BODY()

public:
	/** Keep instance editing gated by the active collection edit session. */
	virtual bool CanEditSMInstance(const FSMInstanceId& InstanceId) const override;
	virtual bool CanMoveSMInstance(const FSMInstanceId& InstanceId, const ETypedElementWorldType WorldType) const override;
	virtual bool SetSMInstanceTransform(const FSMInstanceId& InstanceId, const FTransform& InstanceTransform, bool bWorldSpace = false, bool bMarkRenderStateDirty = false, bool bTeleport = false) override;
	virtual void NotifySMInstanceMovementStarted(const FSMInstanceId& InstanceId) override;
	virtual void NotifySMInstanceMovementOngoing(const FSMInstanceId& InstanceId) override;
	virtual void NotifySMInstanceMovementEnded(const FSMInstanceId& InstanceId) override;
	virtual bool CanDeleteSMInstance(const FSMInstanceId& InstanceId) const override { return false; }
	virtual bool CanDuplicateSMInstance(const FSMInstanceId& InstanceId) const override { return false; }
};
