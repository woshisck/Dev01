#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GA_RangeAttack.generated.h"

class UAnimMontage;

/**
 * Range-attack GA. Plays a montage and waits for the hit event sent back by the bullet.
 *
 * Bullet spawning is handled by AN_FireProjectile directly via UYogBulletManagerSubsystem,
 * reading URangedProjectileDefinition from the character's EquippedWeaponDef.
 *
 * Setup:
 *   1. Configure the attack montage on the equipped weapon's AttackAbilityData
 *      (keyed by Character.State.Skill.Attack). AttackMontage below is only a fallback.
 *   2. Set HitEventTag to match URangedProjectileDefinition.HitEventTag
 *      (default: GameplayEvent.RangeAttack.Hit).
 *   3. Add an FYogGameplayEffectContainer entry keyed by HitEffectContainerTag.
 *      Use UYogTargetType_UseEventData as the TargetType.
 *   4. Place AN_FireProjectile on the fire frame of the montage.
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

	/** Fallback montage used only when the equipped weapon's AttackAbilityData has no matching entry. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Range")
	TObjectPtr<UAnimMontage> AttackMontage;

	// ── Tags ────────────────────────────────────────────────────────────────

	/**
	 * Tag sent to this ability's ASC by the bullet on impact.
	 * Must match URangedProjectileDefinition.HitEventTag.
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
	/** Resolves the attack montage from the equipped weapon's merged AbilityData, falling back to AttackMontage. */
	UAnimMontage* ResolveAttackMontage(const FGameplayAbilityActorInfo* ActorInfo) const;

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

	void ApplyHitDamage(const FGameplayEventData& HitEventData);
};
