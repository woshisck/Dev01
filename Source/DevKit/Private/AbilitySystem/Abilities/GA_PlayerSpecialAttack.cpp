#include "AbilitySystem/Abilities/GA_PlayerSpecialAttack.h"

UGA_PlayerSpecialAttack::UGA_PlayerSpecialAttack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = false;

	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.HitReact")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Knockback")));
}

void UGA_PlayerSpecialAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UE_LOG(LogTemp, Warning, TEXT("[GA_PlayerSpecialAttack] Deprecated special attack ability ignored. Use PlayerActiveSkillComponent instead."));
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
}
