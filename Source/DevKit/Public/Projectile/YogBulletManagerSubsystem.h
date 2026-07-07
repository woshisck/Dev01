#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "NiagaraSystem.h"
#include "YogBulletManagerSubsystem.generated.h"

class ACharacter;
class UAbilitySystemComponent;
class UNiagaraSystem;
class UNiagaraComponent;

/**
 * Public spawn parameters. Fill this and pass to UYogBulletManagerSubsystem::SpawnBullet.
 *
 * Hit resolution mirrors ABuffFlowProjectile::SendTriggerGameplayEvent:
 *   HitEventTag is sent to the creator's ASC (or target's ASC when bSendHitEventToCreator=false)
 *   so GA_RangeAttack::ApplyHitDamage receives it and builds the damage spec live from current
 *   ASC attributes — the Dota2 "current state at hit time" pattern.
 */
USTRUCT(BlueprintType)
struct DEVKIT_API FYogBulletSpawnParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	FVector SpawnLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	FVector Direction = FVector::ForwardVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "1.0"))
	float Speed = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "0.01"))
	float Lifetime = 1.f;

	/** Capsule radius used for overlap detection (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "1.0"))
	float CollisionRadius = 16.f;

	/** Capsule half-height along the flight direction (cm). Clamped to at least CollisionRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "1.0"))
	float CollisionHalfHeight = 24.f;

	/** If false the bullet is removed on the first hit. If true it continues until MaxHits or lifetime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	bool bPiercing = false;

	/** Maximum number of targets to hit when piercing. 0 = unlimited. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet", meta = (ClampMin = "0", EditCondition = "bPiercing"))
	int32 MaxHits = 0;

	/** Sent to the ASC (creator or target) when the bullet hits a valid target. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Events")
	FGameplayTag HitEventTag;

	/** Optionally sent to the creator ASC when the bullet expires without hitting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Events")
	FGameplayTag ExpireEventTag;

	/** true = route hit event to creator ASC (default, used by GA_RangeAttack live-capture pattern). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Events")
	bool bSendHitEventToCreator = true;

	/** Forwarded as EventData.EventMagnitude in the hit event. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Events")
	float EffectMagnitude = 0.f;

	/** The character that fired this bullet. Used for source filtering and GE context. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TWeakObjectPtr<ACharacter> SourceCharacter;

	/** ASC of the firing character. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet")
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	/** Persistent Niagara that follows the bullet while in flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	TObjectPtr<UNiagaraSystem> TravelNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	FVector TravelNiagaraScale = FVector(1.f);

	/** Optional Niagara burst spawned at the hit location. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	FVector HitNiagaraScale = FVector(1.f);

	/** Optional Niagara burst spawned at the bullet position when it expires without hitting. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|VFX")
	FVector ExpireNiagaraScale = FVector(1.f);

	/** Draw the collision capsule each frame while the bullet is in flight. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bullet|Debug")
	bool bDrawDebug = false;
};

// ─── Internal state ───────────────────────────────────────────────────────────

USTRUCT()
struct FYogBulletState
{
	GENERATED_BODY()

	FVector Position = FVector::ZeroVector;
	FVector Direction = FVector::ForwardVector;
	float Speed = 1200.f;
	float CollisionRadius = 16.f;
	float CollisionHalfHeight = 24.f;
	float Lifetime = 1.f;
	float Elapsed = 0.f;
	bool bPiercing = false;
	bool bDrawDebug = false;
	int32 HitsRemaining = 1;

	TWeakObjectPtr<ACharacter> SourceCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	float EffectMagnitude = 0.f;
	FGameplayTag HitEventTag;
	FGameplayTag ExpireEventTag;
	bool bSendHitEventToCreator = true;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> HitNiagaraSystem;

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> ExpireNiagaraSystem;

	// Live per-bullet travel VFX instance. Position is updated each Tick and the
	// component is destroyed when the bullet is removed.
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> TravelNiagaraComp;

	FVector HitNiagaraScale = FVector(1.f);
	FVector ExpireNiagaraScale = FVector(1.f);
};

// ─── Subsystem ────────────────────────────────────────────────────────────────

/**
 * Manages all active "lightweight" bullets as a flat struct array.
 *
 * Eliminates the per-bullet actor overhead (PMC tick, overlap registration,
 * timer entries, GC allocation) in favor of one Tick with manual sphere sweeps.
 * Suitable for up to ~500 simultaneous bullets.
 *
 * Hit resolution is GAS-compatible: on hit, a GameplayEvent is sent to the
 * creator's ASC (or target's ASC) so existing GAs (e.g. GA_RangeAttack) handle
 * damage via live attribute capture — no change to downstream ability logic.
 */
UCLASS()
class DEVKIT_API UYogBulletManagerSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Add a bullet to the simulation. Safe to call from any game thread context. */
	UFUNCTION(BlueprintCallable, Category = "Bullet Manager")
	void SpawnBullet(const FYogBulletSpawnParams& Params);

	/** Number of currently active bullets (for debugging / stats). */
	UFUNCTION(BlueprintPure, Category = "Bullet Manager")
	int32 GetActiveBulletCount() const { return ActiveBullets.Num(); }

	// UTickableWorldSubsystem
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Deinitialize() override;

private:
	UPROPERTY()
	TArray<FYogBulletState> ActiveBullets;

	void StepAndTestBullet(int32 Index, float DeltaTime, TArray<int32>& OutToRemove);
	bool CheckBulletHits(FYogBulletState& Bullet);
	void SendHitEvent(const FYogBulletState& Bullet, AActor* Target) const;
	void SendExpireEvent(const FYogBulletState& Bullet) const;
	void SpawnBurstNiagara(UNiagaraSystem* System, const FVector& Location, const FVector& Scale) const;
	void DestroyBulletVisual(FYogBulletState& Bullet) const;
};
