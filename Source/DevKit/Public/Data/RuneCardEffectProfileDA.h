#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Actors/RuneGroundPathEffectActor.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "Data/RuneDataAsset.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "RuneCardEffectProfileDA.generated.h"

class ASlashWaveProjectile;
class UEffectDataAsset;
class UMaterialInterface;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class ERuneCardProfileDamageMode : uint8
{
	Fixed UMETA(DisplayName = "Fixed"),
	LastDamage UMETA(DisplayName = "Last Damage"),
	CombatCardAttackDamage UMETA(DisplayName = "Combat Card Attack Damage")
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneCardProfileSetByCaller
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SetByCaller")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SetByCaller")
	float Value = 0.0f;

	/** Optional. When set, the active RuneDataAsset tuning scalar overrides Value. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SetByCaller")
	FName TuningKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "SetByCaller")
	bool bUseCombatCardEffectMultiplier = false;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneCardProfileEffectConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> GameplayEffectClass;

	/** Optional data-driven GE builder. Used when GameplayEffectClass is empty. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TObjectPtr<UEffectDataAsset> EffectDataAsset = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect", meta = (ClampMin = "1"))
	int32 ApplicationCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	bool bRemoveEffectOnCleanup = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effect")
	TArray<FRuneCardProfileSetByCaller> SetByCallerValues;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneCardProfileProjectileConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<ASlashWaveProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1.0"))
	float Speed = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float MaxDistance = 800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	int32 MaxHitCount = 2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "1", ClampMax = "20"))
	int32 DamageApplicationsPerTarget = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageApplicationInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	bool bScaleVisualWithCollisionExtent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	FVector VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	TObjectPtr<UNiagaraSystem> ProjectileVisualNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	FVector ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	bool bHideDefaultProjectileVisuals = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	FVector HitNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Visual")
	FVector ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Events")
	FGameplayTag HitGameplayEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Events")
	FGameplayTag ExpireGameplayEventTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Pattern", meta = (ClampMin = "1"))
	int32 ProjectileCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Combat Card")
	bool bAddComboStacksToProjectileCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Combat Card", meta = (ClampMin = "0"))
	int32 ProjectilesPerComboStack = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Combat Card", meta = (ClampMin = "0"))
	int32 MaxBonusProjectiles = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Pattern", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ProjectileConeAngleDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Pattern")
	bool bSpawnProjectilesSequentially = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Pattern", meta = (ClampMin = "0.0"))
	float SequentialProjectileSpawnInterval = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Collision")
	bool bDestroyOnWorldStaticHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage")
	bool bForcePureDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0"))
	float BonusArmorDamageMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage")
	bool bAddSourceArmorToDamage = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0"))
	float SourceArmorToDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage")
	bool bConsumeSourceArmorOnSpawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage", meta = (ClampMin = "0.0"))
	float SourceArmorConsumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage")
	TSubclassOf<UGameplayEffect> AdditionalHitEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage")
	FGameplayTag AdditionalHitSetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Damage")
	float AdditionalHitSetByCallerValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split")
	bool bSplitOnFirstHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0"))
	int32 MaxSplitGenerations = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "1"))
	int32 SplitProjectileCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SplitConeAngleDegrees = 45.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split")
	bool bRandomizeSplitDirections = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SplitRandomYawJitterDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0.0", ClampMax = "45.0"))
	float SplitRandomPitchDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0.0"))
	float SplitDamageMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0.01"))
	float SplitSpeedMultiplier = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split", meta = (ClampMin = "0.0"))
	float SplitMaxDistanceMultiplier = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split")
	FVector SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split|Bounce")
	bool bBounceSplitChildrenOnEnemyHit = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Split|Bounce", meta = (ClampMin = "0"))
	int32 SplitChildMaxEnemyBounces = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector SpawnOffset = FVector(80.f, 0.f, 45.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	FName LaunchNiagaraEffectName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	bool bAttachLaunchNiagaraToSource = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	FVector LaunchNiagaraOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	FRotator LaunchNiagaraRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	FVector LaunchNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile|Launch VFX")
	bool bDestroyLaunchNiagaraWithFlow = false;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneCardProfileAreaConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	ERuneGroundPathTargetPolicy TargetPolicy = ERuneGroundPathTargetPolicy::EnemiesOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	ERuneGroundPathShape Shape = ERuneGroundPathShape::Rectangle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	ERuneGroundPathFacingMode FacingMode = ERuneGroundPathFacingMode::SourceActorForward;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	bool bCenterOnPathLength = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	float RotationYawOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0.01"))
	float Duration = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "0.01"))
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "1.0"))
	float Length = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "1.0"))
	float Width = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area", meta = (ClampMin = "1.0"))
	float Height = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Visual", meta = (ClampMin = "1.0"))
	float DecalProjectionDepth = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Visual")
	float DecalPlaneRotationDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	FVector SpawnOffset = FVector(45.0f, 0.0f, 6.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area")
	EBFTargetSelector Source = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Visual")
	TObjectPtr<UMaterialInterface> DecalMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Visual")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Visual")
	FVector NiagaraScale = FVector(0.5f, 0.5f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Visual", meta = (ClampMin = "1", ClampMax = "12"))
	int32 NiagaraInstanceCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Effect")
	FGameplayTag SetByCallerTag1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Effect")
	float SetByCallerValue1 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Effect")
	FGameplayTag SetByCallerTag2;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Effect")
	float SetByCallerValue2 = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Effect", meta = (ClampMin = "1"))
	int32 ApplicationCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Area|Effect")
	bool bApplyOncePerTarget = false;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneCardProfileVFXConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> NiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FName EffectName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FName AttachSocketName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	TArray<FName> AttachSocketFallbackNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	EBFTargetSelector AttachTarget = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	bool bAttachToTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	FVector Scale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX", meta = (ClampMin = "0.0"))
	float Lifetime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
	bool bDestroyWithFlow = false;
};

UCLASS(BlueprintType)
class DEVKIT_API URuneCardEffectProfileDA : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity")
	FGameplayTag EffectIdTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity")
	FName DebugName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "1. Identity")
	FLinearColor DebugColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Damage")
	ERuneCardProfileDamageMode DamageMode = ERuneCardProfileDamageMode::Fixed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Damage")
	float DamageValue = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Damage")
	FName DamageTuningKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "2. Damage")
	FName DamageLogType = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "3. Effect")
	FRuneCardProfileEffectConfig Effect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "4. Projectile")
	FRuneCardProfileProjectileConfig Projectile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "5. Area")
	FRuneCardProfileAreaConfig Area;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "6. VFX")
	FRuneCardProfileVFXConfig VFX;

	UFUNCTION(BlueprintPure, Category = "Rune Card Profile")
	FName GetTraceName() const;
};
