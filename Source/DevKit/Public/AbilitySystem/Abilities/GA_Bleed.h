#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GA_Bleed.generated.h"

UCLASS()
class DEVKIT_API UGA_Bleed : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Bleed(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float BleedTickInterval = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float DefaultDamagePerSecond = 5.f;

private:
	float DamagePerSecond = 0.f;

	FTimerHandle BleedTimerHandle;
	FDelegateHandle TagChangeDelegateHandle;

	TWeakObjectPtr<UYogAbilitySystemComponent> InstigatorASC;

	void BleedTick();
	void OnBleedingTagChanged(const FGameplayTag Tag, int32 NewCount);
};
