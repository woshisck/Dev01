#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilitySystem/GameplayCue/GCN_AttachedNiagara.h"
#include "Data/AbilityData.h"
#include "MontageVFXBindingComponent.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UMeshComponent;
class UStaticMesh;
class UStaticMeshComponent;
class AWeaponInstance;

// ─── Binding config (written by BFNode_SetMontageVFXBinding in PreCommit flow) ─

USTRUCT(BlueprintType)
struct DEVKIT_API FMontageVFXBindingConfig
{
	GENERATED_BODY()

	// Niagara
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara|Attachment")
	EGCNAttachedNiagaraAttachTarget AttachTarget = EGCNAttachedNiagaraAttachTarget::TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	bool bAttachToSkeletalMesh = true;

	// Actor attach sockets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	FName AttachSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	TArray<FName> AttachSocketFallbackNames;

	// Weapon attach sockets
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara|Weapon")
	FName WeaponAttachSocketName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara|Weapon")
	TArray<FName> WeaponAttachSocketFallbackNames;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara|Weapon")
	bool bFallbackToTargetActorIfWeaponMissing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Niagara|Parameters")
	TArray<FGCNNiagaraParamOverride> NiagaraParameterOverrides;

	// Sound (fire-and-forget on NotifyBegin)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Sound")
	TObjectPtr<USoundBase> Sound;

	// Weapon material override (restored on NotifyEnd / ClearAllBindings)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Material",
		meta = (ToolTip = "Applied to the weapon mesh on NotifyBegin, restored on NotifyEnd."))
	TObjectPtr<UMaterialInterface> WeaponMaterialOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Material|Parameters")
	TArray<FGCNMaterialParamOverride> WeaponMaterialParameterOverrides;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Material")
	int32 WeaponMaterialSlot = 0;

	// Annulus plane (spawned on NotifyBegin, destroyed on NotifyEnd)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane")
	bool bSpawnAnnulusPlane = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane",
		meta = (EditCondition = "bSpawnAnnulusPlane", EditConditionHides))
	TObjectPtr<UStaticMesh> AnnulusPlaneMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane",
		meta = (EditCondition = "bSpawnAnnulusPlane", EditConditionHides))
	TObjectPtr<UMaterialInterface> AnnulusPlaneMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane",
		meta = (EditCondition = "bSpawnAnnulusPlane", EditConditionHides, ClampMin = "0.0"))
	float AnnulusPlaneZOffset = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane",
		meta = (EditCondition = "bSpawnAnnulusPlane", EditConditionHides, ClampMin = "1.0"))
	float AnnulusPlaneMeshSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane",
		meta = (EditCondition = "bSpawnAnnulusPlane", EditConditionHides))
	FLinearColor AnnulusPlaneTint = FLinearColor(0.f, 0.7f, 1.f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX|Annulus Plane|Parameters",
		meta = (EditCondition = "bSpawnAnnulusPlane", EditConditionHides))
	TArray<FGCNMaterialParamOverride> AnnulusPlaneMaterialParameterOverrides;
};

// ─── Active-state tracking (internal, per slot) ──────────────────────────────

USTRUCT()
struct DEVKIT_API FMontageVFXActiveState
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> NiagaraComp;

	UPROPERTY()
	TObjectPtr<UMeshComponent> WeaponMesh;

	UPROPERTY()
	TObjectPtr<UMaterialInterface> OriginalMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ActiveDynamicMaterial;

	UPROPERTY()
	int32 MaterialSlot = 0;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> AnnulusPlaneComponents;
};

// ─── Component ───────────────────────────────────────────────────────────────

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class DEVKIT_API UMontageVFXBindingComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMontageVFXBindingComponent();

	void RegisterBinding(FName SlotName, const FMontageVFXBindingConfig& Config);
	void ActivateSlot(FName SlotName, const FActionData* ActionData = nullptr);
	void DeactivateSlot(FName SlotName);
	void ClearAllBindings();

private:
	UPROPERTY()
	TMap<FName, FMontageVFXBindingConfig> PendingBindings;

	UPROPERTY()
	TMap<FName, FMontageVFXActiveState> ActiveStates;

	USceneComponent* ResolveAttachTarget(const FMontageVFXBindingConfig& Config, FName& OutSocket) const;
	USceneComponent* ResolveWeaponAttachComponent(const FMontageVFXBindingConfig& Config, FName& OutSocket) const;
	USceneComponent* ResolveOwnerAttachComponent(const FMontageVFXBindingConfig& Config, FName& OutSocket) const;
	static USceneComponent* FindNamedSceneComponent(AActor* OwnerActor, FName ComponentName);
	static bool TryResolveSocketOrBone(UMeshComponent* Mesh, FName SocketOrBoneName, FName& OutSocket);
	AWeaponInstance* ResolveEquippedWeapon() const;
	void ApplyNiagaraParameterOverrides(UNiagaraComponent* Comp, const TArray<FGCNNiagaraParamOverride>& Overrides) const;
	void ApplyMaterialParams(UMaterialInstanceDynamic* DynMat, const TArray<FGCNMaterialParamOverride>& Params) const;
	void SpawnAnnulusPlanes(const FMontageVFXBindingConfig& Config, const FActionData& ActionData, FMontageVFXActiveState& State) const;
	void TearDownActiveState(FMontageVFXActiveState& State);
};
