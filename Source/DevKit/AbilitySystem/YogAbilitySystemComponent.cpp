#include "YogAbilitySystemComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"




// Sets default values
UYogAbilitySystemComponent::UYogAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
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

void UYogAbilitySystemComponent::DebugActivatableAbilities()
{
	TArray<FGameplayAbilitySpec> TargetArray = GetActivatableAbilities();
	int count = 0;
	FString results;
	for (const FGameplayAbilitySpec& spec : TargetArray)
	{
		
		UE_LOG(LogTemp, Display, TEXT("Current ability spec handle: %s"), *spec.Handle.ToString());

	}

}


