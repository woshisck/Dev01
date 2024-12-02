#include "YogAbilitySystemComponent.h"

// Sets default values
UYogAbilitySystemComponent::UYogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	//
}




void UYogAbilitySystemComponent::AddDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
}

void UYogAbilitySystemComponent::RemoveDynamicTagGameplayEffect(const FGameplayTag& Tag)
{
}

void UYogAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
}

void UYogAbilitySystemComponent::ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	ReceivedDamage.Broadcast(SourceASC, Damage);
}
