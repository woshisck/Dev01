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

void UYogAbilitySystemComponent::LogAllGrantedAbilities()
{
	TArray<FGameplayAbilitySpec>& AbilitySpecs = this->GetActivatableAbilities();

	for (FGameplayAbilitySpec& Spec : AbilitySpecs)
	{
		if (UYogGameplayAbility* Ability = Cast<UYogGameplayAbility>(Spec.Ability))
		{
			UE_LOG(LogTemp, Warning, TEXT("granted abilities is: %s"), *Ability->GetName());
		}
	}

    int32 TotalAbilities = AbilitySpecs.Num();
    UE_LOG(LogTemp, Warning, TEXT("Total number of granted abilities: %d"), TotalAbilities);

}



UYogGameplayAbility* UYogAbilitySystemComponent::GetCurrentActiveAbility()
{
				// Get the ability instance
	UYogGameplayAbility* AbilityInstance = Cast<UYogGameplayAbility>(cache_AbilitySpec.GetPrimaryInstance());

	// Or get the CDO if no instance exists
	UGameplayAbility* AbilityCDO = cache_AbilitySpec.Ability;

	if (AbilityInstance)
	{
		// Work with the active ability instance
		FString AbilityName = AbilityInstance->GetName();
		UE_LOG(LogTemp, Log, TEXT("Active Ability: %s"), *AbilityName);
	}
	return AbilityInstance;
}
