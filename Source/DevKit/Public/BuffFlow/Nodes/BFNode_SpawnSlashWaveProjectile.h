#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "GameplayTagContainer.h"
#include "BFNode_SpawnSlashWaveProjectile.generated.h"

class ASlashWaveProjectile;
class ACharacter;
class UGameplayEffect;
class UNiagaraSystem;

/**
 * Spawns a configurable slash-wave projectile. Used by combat cards such as Moonlight.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn Slash Wave Projectile", Category = "BuffFlow|Projectile"))
class DEVKIT_API UBFNode_SpawnSlashWaveProjectile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	TSubclassOf<ASlashWaveProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	float Damage = 10.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	FName DamageLogType = TEXT("Rune_SlashWave");

	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1.0"))
	float Speed = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0"))
	float MaxDistance = 800.f;

	/** <= 0 means unlimited targets. Each actor can receive DamageApplicationsPerTarget hits. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	int32 MaxHitCount = 2;

	/** Number of damage applications per target after the target first overlaps this wave. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "1", ClampMax = "20"))
	int32 DamageApplicationsPerTarget = 1;

	/** Delay between repeated damage applications on the same target. <= 0 applies repeats immediately. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave", meta = (ClampMin = "0.0"))
	float DamageApplicationInterval = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	FVector CollisionBoxExtent = FVector(30.f, 60.f, 35.f);

	/** When true, larger CollisionBoxExtent also scales the projectile actor/VFX relative to the projectile default extent. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	bool bScaleVisualWithCollisionExtent = true;

	/** Extra visual scale multiplier after collision-based scale is applied. Does not change the final configured collision extent. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	FVector VisualScaleMultiplier = FVector(1.f, 1.f, 1.f);

	/** Optional Niagara used as the projectile's main blade visual. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	TObjectPtr<UNiagaraSystem> ProjectileVisualNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	FVector ProjectileVisualNiagaraScale = FVector(1.f, 1.f, 1.f);

	/** Hide mesh/old BP visual components so only ProjectileVisualNiagaraSystem is visible. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	bool bHideDefaultProjectileVisuals = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	FVector HitNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Visual")
	FVector ExpireNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events")
	FGameplayTag HitGameplayEventTag;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Events")
	FGameplayTag ExpireGameplayEventTag;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "1"))
	int32 ProjectileCount = 1;

	/** Adds projectiles from the combat-card combo stacks when this Flow was started by CombatDeck. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (DisplayName = "Add Combo Stacks To Projectile Count"))
	bool bAddComboStacksToProjectileCount = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
	int32 ProjectilesPerComboStack = 1;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Combat Card", meta = (ClampMin = "0", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
	int32 MaxBonusProjectiles = 0;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float ProjectileConeAngleDegrees = 0.f;

	/** When true, extra projectiles are fired one after another along the same path instead of fanning out. */
	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (DisplayName = "Spawn Projectiles Sequentially"))
	bool bSpawnProjectilesSequentially = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Pattern", meta = (ClampMin = "0.0", EditCondition = "bSpawnProjectilesSequentially", EditConditionHides))
	float SequentialProjectileSpawnInterval = 0.12f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Collision")
	bool bDestroyOnWorldStaticHit = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
	bool bForcePureDamage = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
	float BonusArmorDamageMultiplier = 0.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
	bool bAddSourceArmorToDamage = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
	float SourceArmorToDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
	bool bConsumeSourceArmorOnSpawn = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage", meta = (ClampMin = "0.0"))
	float SourceArmorConsumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
	TSubclassOf<UGameplayEffect> AdditionalHitEffect;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
	FGameplayTag AdditionalHitSetByCallerTag;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Damage")
	float AdditionalHitSetByCallerValue = 0.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split")
	bool bSplitOnFirstHit = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0"))
	int32 MaxSplitGenerations = 1;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "1"))
	int32 SplitProjectileCount = 3;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float SplitConeAngleDegrees = 45.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0"))
	float SplitDamageMultiplier = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.01"))
	float SplitSpeedMultiplier = 2.f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split", meta = (ClampMin = "0.0"))
	float SplitMaxDistanceMultiplier = 0.6f;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|Split")
	FVector SplitCollisionBoxExtentMultiplier = FVector(0.5f, 0.5f, 0.5f);

	UPROPERTY(EditAnywhere, Category = "Slash Wave")
	FVector SpawnOffset = FVector(80.f, 0.f, 45.f);

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	TObjectPtr<UNiagaraSystem> LaunchNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	FName LaunchNiagaraEffectName = NAME_None;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	bool bAttachLaunchNiagaraToSource = false;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	FVector LaunchNiagaraOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	FRotator LaunchNiagaraRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	FVector LaunchNiagaraScale = FVector(1.f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, Category = "Slash Wave|VFX")
	bool bDestroyLaunchNiagaraWithFlow = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	float ResolveDamage(ACharacter* SourceCharacter) const;
	void ConsumeSourceArmor(ACharacter* SourceCharacter) const;
	void SpawnLaunchNiagara(ACharacter* SourceCharacter, const FVector& SpawnLocation, const FRotator& SpawnRotation) const;
};
