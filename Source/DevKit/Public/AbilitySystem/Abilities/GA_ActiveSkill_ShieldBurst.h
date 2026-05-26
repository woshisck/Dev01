#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GameplayEffectTypes.h"
#include "GA_ActiveSkill_ShieldBurst.generated.h"

class UYogAbilitySystemComponent;

UCLASS()
class DEVKIT_API UGA_ActiveSkill_ShieldBurst : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_ActiveSkill_ShieldBurst(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	static FGameplayEventData MakeKnockbackPayload(
		AActor* SourceActor,
		AActor* TargetActor,
		float InKnockbackDistance,
		const FGameplayEffectContextHandle& DamageContext);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Active Skill", meta = (ClampMin = "0.0"))
	float BuffDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Active Skill", meta = (ClampMin = "0.0"))
	float KnockbackDistance = 500.0f;

private:
	UFUNCTION()
	void HandlePlayerDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage);

	void HandlePlayerDamageDealtWithContext(
		UYogAbilitySystemComponent* TargetASC,
		float Damage,
		const FGameplayEffectContextHandle& DamageContext);

	void FinishBuff();

	UPROPERTY()
	TObjectPtr<UYogAbilitySystemComponent> SourceASC;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	FTimerHandle BuffTimerHandle;
	FDelegateHandle DealtDamageWithContextHandle;
};
