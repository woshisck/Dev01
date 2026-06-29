#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Projectile/BuffFlowProjectile.h"
#include "GameplayTagContainer.h"
#include "GA_RangeAttack.generated.h"

class UAnimMontage;

/**
 * Range-attack GA. Plays a montage and waits for two event types:
 *
 *   Fire event  (AN_FireProjectile notify) — spawns a BuffFlowProjectile on the game thread.
 *   Hit  event  (projectile on impact)    — creates the damage GE spec live from current ASC
 *                                           attributes, so buffs gained during bullet flight
 *                                           are included at the moment of impact.
 *
 * Setup:
 *   1. Set AttackMontage, ProjectileClass (BP subclass of ABuffFlowProjectile).
 *   2. Configure ProjectileConfig (Speed, Lifetime, visuals). Leave EffectClass null —
 *      GA_RangeAttack applies damage via live spec creation in OnEventReceived.
 *   3. Add an FYogGameplayEffectContainer entry keyed by HitEffectContainerTag.
 *      Use UYogTargetType_UseEventData as the TargetType.
 *   4. Place AN_FireProjectile on the fire frame; set its EventTag == FireEventTag.
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_RangeAttack : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_RangeAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	// ── Montage ─────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	TObjectPtr<UAnimMontage> AttackMontage;

	// ── Projectile ─────────────────────────────────────────────────────────

	/** Blueprint subclass of ABuffFlowProjectile to spawn on fire event. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	TSubclassOf<ABuffFlowProjectile> ProjectileClass;

	/**
	 * Base runtime config forwarded to the projectile.
	 * TriggerGameplayEventTag is overridden to HitEventTag at spawn time,
	 * and EffectClass is cleared so damage is applied via live attribute capture.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	FBuffFlowProjectileRuntimeConfig ProjectileConfig;

	// ── Tags ────────────────────────────────────────────────────────────────

	/**
	 * Tag sent by AN_FireProjectile. Must match the EventTag on the notify.
	 * Default: GameplayEvent.RangeAttack.Fire
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	FGameplayTag FireEventTag;

	/**
	 * Tag sent back to the creator ASC by the projectile on impact.
	 * GA_RangeAttack creates the damage spec at this point for live attribute capture.
	 * Default: GameplayEvent.RangeAttack.Hit
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	FGameplayTag HitEventTag;

	/**
	 * Key into EffectContainerMap for the damage GE applied when a hit event arrives.
	 * The container must use UYogTargetType_UseEventData — the hit actor comes from EventData.Target.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	FGameplayTag HitEffectContainerTag;

private:
	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);

	void SpawnProjectile(const FGameplayEventData& FireEventData, const FGameplayAbilityActorInfo* ActorInfo);
	void ApplyHitDamage(const FGameplayEventData& HitEventData);
};
