#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_AttachedNiagara.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;
class USkeletalMeshComponent;
class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class AWeaponInstance;

UENUM(BlueprintType)
enum class EGCNNiagaraParamType : uint8
{
	Float  UMETA(DisplayName = "Float"),
	Vector UMETA(DisplayName = "Vector"),
	Color  UMETA(DisplayName = "Color"),
	Bool   UMETA(DisplayName = "Bool"),
	Int    UMETA(DisplayName = "Int"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FGCNNiagaraParamOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter")
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter")
	EGCNNiagaraParamType ParamType = EGCNNiagaraParamType::Float;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter",
		meta = (EditCondition = "ParamType == EGCNNiagaraParamType::Float", EditConditionHides))
	float FloatValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter",
		meta = (EditCondition = "ParamType == EGCNNiagaraParamType::Vector", EditConditionHides))
	FVector VectorValue = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter",
		meta = (EditCondition = "ParamType == EGCNNiagaraParamType::Color", EditConditionHides))
	FLinearColor ColorValue = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter",
		meta = (EditCondition = "ParamType == EGCNNiagaraParamType::Bool", EditConditionHides))
	bool bBoolValue = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Niagara|Parameter",
		meta = (EditCondition = "ParamType == EGCNNiagaraParamType::Int", EditConditionHides))
	int32 IntValue = 0;
};

UENUM(BlueprintType)
enum class EGCNMaterialParamType : uint8
{
	Scalar UMETA(DisplayName = "Scalar"),
	Vector UMETA(DisplayName = "Vector"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FGCNMaterialParamOverride
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material|Parameter")
	FName ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material|Parameter")
	EGCNMaterialParamType ParamType = EGCNMaterialParamType::Scalar;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material|Parameter",
		meta = (EditCondition = "ParamType == EGCNMaterialParamType::Scalar", EditConditionHides))
	float ScalarValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material|Parameter",
		meta = (EditCondition = "ParamType == EGCNMaterialParamType::Vector", EditConditionHides))
	FLinearColor VectorValue = FLinearColor::White;
};

UENUM(BlueprintType)
enum class EGCNAttachedNiagaraAttachTarget : uint8
{
	TargetActor UMETA(DisplayName = "Target Actor"),
	EquippedWeapon UMETA(DisplayName = "Equipped Weapon"),
};

/**
 * GameplayCue notify actor that keeps a Niagara system attached to the target actor's mesh
 * for the lifetime of an active cue. Intended for persistent status VFX such as burn or bleed.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AGCN_AttachedNiagara : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_AttachedNiagara();

	virtual bool OnActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool WhileActive_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual bool OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara|Attachment")
	EGCNAttachedNiagaraAttachTarget AttachTarget = EGCNAttachedNiagaraAttachTarget::TargetActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	bool bAttachToSkeletalMesh = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FName AttachSocketName = TEXT("spine_03");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	TArray<FName> AttachSocketFallbackNames = {
		TEXT("spine_02"),
		TEXT("pelvis"),
		TEXT("root")
	};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara|Weapon")
	FName WeaponAttachSocketName = TEXT("VFX");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara|Weapon")
	TArray<FName> WeaponAttachSocketFallbackNames = {
		TEXT("Muzzle"),
		TEXT("FX")
	};

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara|Weapon")
	bool bFallbackToTargetActorIfWeaponMissing = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	FVector Scale = FVector(1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	bool bSpawnOnExecute = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara")
	bool bSkipDedicatedServer = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Niagara|Parameters")
	TArray<FGCNNiagaraParamOverride> NiagaraParameterOverrides;

	// ── Weapon Material Override ───────────────────────────────────────────────

	/** Material applied to the equipped weapon's first material slot on active. Null = no override. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponMaterial")
	TObjectPtr<UMaterialInterface> WeaponMaterialOverride;

	/** Scalar/Vector parameters set on the dynamic material instance created from WeaponMaterialOverride. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WeaponMaterial|Parameters")
	TArray<FGCNMaterialParamOverride> WeaponMaterialParameterOverrides;

private:
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> ActiveNiagaraComponent;

	// Original material at slot 0 before the override was applied.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> OriginalWeaponMaterial;

	// Dynamic instance created from WeaponMaterialOverride; null when inactive.
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ActiveWeaponMaterialDynamic;

	// Weak ref to the mesh we patched — avoids holding a strong ref to the weapon.
	TWeakObjectPtr<UMeshComponent> CachedWeaponMesh;

	UNiagaraComponent* SpawnNiagara(AActor* Target, bool bAutoDestroy);
	void StopNiagara();
	void ApplyNiagaraParameterOverrides(UNiagaraComponent* Component) const;
	void ApplyWeaponMaterial(AActor* Target);
	void RestoreWeaponMaterial();
	void ApplyWeaponMaterialParameters(UMaterialInstanceDynamic* DynMat) const;
	USceneComponent* ResolveAttachComponent(AActor* Target, FName& OutSocketName) const;
	USceneComponent* ResolveTargetActorAttachComponent(AActor* Target, FName& OutSocketName) const;
	USceneComponent* ResolveWeaponAttachComponent(AActor* Target, FName& OutSocketName) const;
	USceneComponent* FindNamedSceneComponent(AActor* OwnerActor, const FName ComponentName) const;
	AWeaponInstance* ResolveEquippedWeapon(AActor* Target) const;
	bool TryResolveSocketOrBone(USceneComponent* Component, const FName SocketOrBoneName, FName& OutSocketName) const;
};
