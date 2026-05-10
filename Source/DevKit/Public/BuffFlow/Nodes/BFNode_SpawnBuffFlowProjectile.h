#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "Projectile/BuffFlowProjectile.h"
#include "BFNode_SpawnBuffFlowProjectile.generated.h"

class UCurveFloat;
class UGameplayEffect;
class UBuffFlowComponent;

UCLASS(NotBlueprintable, meta = (DisplayName = "Spawn BuffFlow Projectile", Category = "BuffFlow|Projectile"))
class DEVKIT_API UBFNode_SpawnBuffFlowProjectile : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile", meta = (DisplayName = "Projectile Class"))
	TSubclassOf<ABuffFlowProjectile> ProjectileClass;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile", meta = (DisplayName = "Creator Source"))
	EBFTargetSelector SourceSelector = EBFTargetSelector::BuffOwner;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile", meta = (DisplayName = "Spawn Offset"))
	FVector SpawnOffset = FVector(80.f, 0.f, 45.f);

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Pattern", meta = (ClampMin = "1", DisplayName = "Projectile Count"))
	int32 ProjectileCount = 1;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Combat Card", meta = (DisplayName = "Add Combo Stacks To Projectile Count"))
	bool bAddComboStacksToProjectileCount = false;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Combat Card", meta = (ClampMin = "0", DisplayName = "Projectiles Per Combo Stack", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
	int32 ProjectilesPerComboStack = 1;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Combat Card", meta = (ClampMin = "0", DisplayName = "Max Bonus Projectiles", EditCondition = "bAddComboStacksToProjectileCount", EditConditionHides))
	int32 MaxBonusProjectiles = 0;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile", meta = (DisplayName = "Trigger Mode"))
	EBuffFlowProjectileTriggerMode TriggerMode = EBuffFlowProjectileTriggerMode::HitOnce;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile", meta = (ClampMin = "0.01", DisplayName = "Trigger Interval", EditCondition = "TriggerMode == EBuffFlowProjectileTriggerMode::PeriodicOverlap", EditConditionHides))
	float TriggerInterval = 0.2f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Lifetime", meta = (ClampMin = "0.01", DisplayName = "Lifetime"))
	float Lifetime = 1.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Lifetime", meta = (DisplayName = "Lifetime Curve"))
	TObjectPtr<UCurveFloat> LifetimeCurve = nullptr;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Lifetime", meta = (DisplayName = "Lifetime Curve Input", EditCondition = "LifetimeCurve != nullptr", EditConditionHides))
	EBuffFlowProjectileCurveInput LifetimeCurveInput = EBuffFlowProjectileCurveInput::Constant;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Lifetime", meta = (DisplayName = "Lifetime Curve Constant Input", EditCondition = "LifetimeCurve != nullptr", EditConditionHides))
	float LifetimeCurveConstantInput = 0.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Movement", meta = (ClampMin = "0.0", DisplayName = "Speed"))
	float Speed = 1200.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Movement", meta = (DisplayName = "Speed Over Life Curve"))
	TObjectPtr<UCurveFloat> SpeedOverLifeCurve = nullptr;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Movement", meta = (DisplayName = "Speed Curve Mode", EditCondition = "SpeedOverLifeCurve != nullptr", EditConditionHides))
	EBuffFlowProjectileSpeedCurveMode SpeedCurveMode = EBuffFlowProjectileSpeedCurveMode::AbsoluteSpeed;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Collision", meta = (ClampMin = "1.0", DisplayName = "Collision Capsule Radius"))
	float CollisionCapsuleRadius = 24.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Collision", meta = (ClampMin = "1.0", DisplayName = "Collision Capsule Half Height"))
	float CollisionCapsuleHalfHeight = 48.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Collision", meta = (DisplayName = "Destroy On Hit Trigger"))
	bool bDestroyOnHitTrigger = true;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Collision", meta = (DisplayName = "Destroy On World Static Hit"))
	bool bDestroyOnWorldStaticHit = true;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (DisplayName = "Effect Class"))
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (DisplayName = "Apply Pure Damage Fallback"))
	bool bApplyPureDamageFallback = false;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (DisplayName = "SetByCaller Magnitude Tag"))
	FGameplayTag SetByCallerMagnitudeTag;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (DisplayName = "Base Effect Magnitude"))
	float BaseEffectMagnitude = 0.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (DisplayName = "Creator Attack Magnitude Scale"))
	float CreatorAttackMagnitudeScale = 1.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (DisplayName = "Creator Attack Power Magnitude Scale"))
	float CreatorAttackPowerMagnitudeScale = 0.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (ClampMin = "0.0", DisplayName = "Effect Radius"))
	float EffectRadius = 0.f;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Effect", meta = (ClampMin = "0", DisplayName = "Max Triggers Per Target"))
	int32 MaxTriggersPerTarget = 0;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Events", meta = (DisplayName = "Trigger Gameplay Event Tag"))
	FGameplayTag TriggerGameplayEventTag;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Events", meta = (DisplayName = "Trigger Event To Creator"))
	bool bSendTriggerEventToCreator = true;

	UPROPERTY(EditAnywhere, Category = "BuffFlow Projectile|Events", meta = (DisplayName = "Expire Gameplay Event Tag"))
	FGameplayTag ExpireGameplayEventTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;

private:
	FTransform ResolveSpawnTransform(AActor* SourceActor) const;
	int32 ResolveSpawnCount(const UBuffFlowComponent* BuffFlowComponent, int32& OutComboBonusProjectiles) const;
	FBuffFlowProjectileRuntimeConfig BuildRuntimeConfig() const;
};
