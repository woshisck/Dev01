#include "AbilitySystem/Abilities/GA_RangeAttack.h"

UGA_RangeAttack::UGA_RangeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_RangeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[GA_RangeAttack] Stub activated — implement projectile/hitscan pipeline here."));

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
