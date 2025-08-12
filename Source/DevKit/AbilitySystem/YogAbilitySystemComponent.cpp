#include "YogAbilitySystemComponent.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"




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

void UYogAbilitySystemComponent::RemoveGameplayTag(FGameplayTag Tag, int32 Count)
{
	int stack = this->GetTagCount(Tag);

	if (stack <= 1)
	{
		this->RemoveLooseGameplayTag(Tag, 1);
	}
	else
	{
		this->SetLooseGameplayTagCount(Tag, stack - Count);
	}
}


void UYogAbilitySystemComponent::AddGameplayTag(FGameplayTag Tag, int32 Count)
{
	this->AddLooseGameplayTag(Tag, Count);
}


void UYogAbilitySystemComponent::ReceiveDamage(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	ReceivedDamage.Broadcast(SourceASC, Damage);
}

void UYogAbilitySystemComponent::AddActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToBlock)
{
	this->AddLooseGameplayTags(TagsToBlock);
	this->SetTagMapCount(Tag, 1);
}

void UYogAbilitySystemComponent::RemoveActivationBlockedTags(const FGameplayTag& Tag, const FGameplayTagContainer& TagsToUnblock)
{
	this->RemoveLooseGameplayTags(TagsToUnblock);
	this->SetTagMapCount(Tag, 0);
}

UYogGameplayAbility* UYogAbilitySystemComponent::GetCurrentAbilityClass()
{

	UYogGameplayAbility* CurrentAbility = NewObject<UYogGameplayAbility>();
	
	if (this->GetAnimatingAbility())
	{
		CurrentAbility = Cast<UYogGameplayAbility>(this->GetAnimatingAbility());
	}

	return CurrentAbility;
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

void UYogAbilitySystemComponent::GetActiveAbilitiesWithTags(const FGameplayTagContainer& GameplayTagContainer, TArray<UYogGameplayAbility*>& ActiveAbilities)
{
	TArray<FGameplayAbilitySpec*> AbilitiesToActivate;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivate, false);

	// Iterate the list of all ability specs
	for (FGameplayAbilitySpec* Spec : AbilitiesToActivate)
	{
		// Iterate all instances on this ability spec
		TArray<UGameplayAbility*> AbilityInstances = Spec->GetAbilityInstances();

		for (UGameplayAbility* ActiveAbility : AbilityInstances)
		{
			ActiveAbilities.Add(Cast<UYogGameplayAbility>(ActiveAbility));
		}
	}

}

void UYogAbilitySystemComponent::OnAbilityActivated(UYogGameplayAbility* ActivatedAbility)
{
	//CurrentActiveAbility = ActivatedAbility;
}

void UYogAbilitySystemComponent::OnAbilityEnded(const FAbilityEndedData& EndedData)
{
	//if (CurrentActiveAbility == EndedData.AbilityThatEnded)
	//{
	//	CurrentActiveAbility = nullptr;
	//}
}

void UYogAbilitySystemComponent::SetAbilityRetriggerable(FGameplayAbilitySpecHandle Handle, bool bCanRetrigger)
{
	FGameplayAbilitySpec* Spec = this->FindAbilitySpecFromHandle(Handle);
	if (Spec && Spec->Ability)
	{
		// For instanced abilities
		if (UYogGameplayAbility* AbilityInstance = Cast<UYogGameplayAbility>(Spec->GetPrimaryInstance()))
		{
			AbilityInstance->UpdateRetrigger(bCanRetrigger);

		}
		// For non-instanced but might be relevant for some cases
		else
		{
			UYogGameplayAbility* Ability = Cast<UYogGameplayAbility>(Spec->Ability);
			Ability->UpdateRetrigger(bCanRetrigger);

		}

		// Mark the spec as dirty if needed
		this->MarkAbilitySpecDirty(*Spec);
	}
}


