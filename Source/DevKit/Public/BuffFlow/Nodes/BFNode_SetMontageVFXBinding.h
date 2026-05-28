#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Component/MontageVFXBindingComponent.h"
#include "BFNode_SetMontageVFXBinding.generated.h"

class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;

UCLASS(NotBlueprintable, meta = (DisplayName = "Set Montage VFX Binding", Category = "BuffFlow|Combat"))
class DEVKIT_API UBFNode_SetMontageVFXBinding : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Binding")
	FName SlotName;

	// ── Niagara ──────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara|Attachment")
	EGCNAttachedNiagaraAttachTarget AttachTarget = EGCNAttachedNiagaraAttachTarget::TargetActor;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara",
		meta = (EditCondition = "AttachTarget == EGCNAttachedNiagaraAttachTarget::TargetActor", EditConditionHides))
	bool bAttachToSkeletalMesh = true;

	// Actor attach
	UPROPERTY(EditAnywhere, Category = "VFX|Niagara",
		meta = (EditCondition = "AttachTarget == EGCNAttachedNiagaraAttachTarget::TargetActor", EditConditionHides))
	FName AttachSocketName;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara",
		meta = (EditCondition = "AttachTarget == EGCNAttachedNiagaraAttachTarget::TargetActor", EditConditionHides))
	TArray<FName> AttachSocketFallbackNames;

	// Weapon attach
	UPROPERTY(EditAnywhere, Category = "VFX|Niagara|Weapon",
		meta = (EditCondition = "AttachTarget == EGCNAttachedNiagaraAttachTarget::EquippedWeapon", EditConditionHides))
	FName WeaponAttachSocketName;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara|Weapon",
		meta = (EditCondition = "AttachTarget == EGCNAttachedNiagaraAttachTarget::EquippedWeapon", EditConditionHides))
	TArray<FName> WeaponAttachSocketFallbackNames;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara|Weapon",
		meta = (EditCondition = "AttachTarget == EGCNAttachedNiagaraAttachTarget::EquippedWeapon", EditConditionHides))
	bool bFallbackToTargetActorIfWeaponMissing = true;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara")
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditAnywhere, Category = "VFX|Niagara|Parameters")
	TArray<FGCNNiagaraParamOverride> NiagaraParameterOverrides;

	// ── Sound ────────────────────────────────────────────────────────────────

	UPROPERTY(EditAnywhere, Category = "VFX|Sound")
	TObjectPtr<USoundBase> Sound;

	// ── Weapon material override ──────────────────────────────────────────────

	UPROPERTY(EditAnywhere, Category = "VFX|Material",
		meta = (ToolTip = "Applied to the weapon mesh on NotifyBegin, restored on NotifyEnd."))
	TObjectPtr<UMaterialInterface> WeaponMaterialOverride;

	UPROPERTY(EditAnywhere, Category = "VFX|Material|Parameters")
	TArray<FGCNMaterialParamOverride> WeaponMaterialParameterOverrides;

	UPROPERTY(EditAnywhere, Category = "VFX|Material")
	int32 WeaponMaterialSlot = 0;

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
