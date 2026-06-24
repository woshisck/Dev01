#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_SlashWaveCounter.generated.h"

class ASlashWaveProjectile;
class UAbilityTask_WaitGameplayEvent;
class UGameplayEffect;

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_SlashWaveCounter : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_SlashWaveCounter();

	virtual void OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilitySpec& Spec) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Counter")
	int32 HitsRequired = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Counter")
	bool bHitRequired = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Counter")
	FGameplayTag SwingModeGateTag;

	UPROPERTY(EditDefaultsOnly, Category = "SlashWave|Projectile")
	TSubclassOf<ASlashWaveProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Projectile")
	float SlashDamage = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "SlashWave|Projectile")
	TSubclassOf<UGameplayEffect> SlashDamageEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "SlashWave|Projectile")
	float SpawnOffset = 80.f;

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

private:
	int32 CurrentCount = 0;

	TWeakObjectPtr<AActor> PendingSlashWaveInitialTarget;

	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitEventTask;

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);

	void SpawnSlashWave();
};
