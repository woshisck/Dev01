#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RangedProjectileDefinition.generated.h"

class UNiagaraSystem;

/**
 * Lightweight ranged bullet definition consumed by UAN_FireProjectile.
 *
 * Fields map directly onto FYogBulletSpawnParams — the bullet is simulated by
 * UYogBulletManagerSubsystem (flat struct, straight-line motion, sphere sweep),
 * NOT an ABuffFlowProjectile actor. Ranged weapons reference this from
 * UWeaponDefinition so one shared range-attack ability fires different bullets.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "Ranged Projectile Definition")
class DEVKIT_API URangedProjectileDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	URangedProjectileDefinition();

	// ── Motion / Collision ──────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet", meta = (ClampMin = "1.0"))
	float Speed = 1600.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet", meta = (ClampMin = "0.01"))
	float Lifetime = 1.25f;

	/** Capsule radius used for overlap detection (cm). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet", meta = (ClampMin = "1.0"))
	float CollisionRadius = 18.f;

	/** Capsule half-height along the flight direction (cm). Clamped to at least CollisionRadius. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet", meta = (ClampMin = "1.0"))
	float CollisionHalfHeight = 24.f;

	/** If true the bullet keeps going after a hit until MaxHits or lifetime. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet")
	bool bPiercing = false;

	/** Max targets to hit when piercing. 0 = unlimited. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet", meta = (ClampMin = "0", EditCondition = "bPiercing", EditConditionHides))
	int32 MaxHits = 0;

	// ── Damage magnitude (snapshot at fire time) ────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float BaseEffectMagnitude = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float CreatorAttackMagnitudeScale = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	float CreatorAttackPowerMagnitudeScale = 0.f;

	// ── Events ──────────────────────────────────────────────────────────

	/** Sent to the firing ASC on hit. Must match GA_RangeAttack.HitEventTag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	FGameplayTag HitEventTag;

	/** Optionally sent to the firing ASC when the bullet expires without hitting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	FGameplayTag ExpireEventTag;

	/** true = route the hit event to the creator ASC (live-capture damage pattern). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Events")
	bool bSendHitEventToCreator = true;

	// ── Debug ───────────────────────────────────────────────────────────

	/** Draw the collision capsule each frame while the bullet is in flight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Debug")
	bool bDrawDebug = false;

	// ── VFX ─────────────────────────────────────────────────────────────

	/** Persistent Niagara that follows the bullet while in flight. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> TravelNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FVector TravelNiagaraScale = FVector(1.f);

	/** One-shot burst spawned at the hit location. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FVector HitNiagaraScale = FVector(1.f);

	/** One-shot burst spawned where the bullet expires without hitting. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VFX")
	FVector ExpireNiagaraScale = FVector(1.f);
};
