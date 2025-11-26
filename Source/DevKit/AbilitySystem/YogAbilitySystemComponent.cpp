#include "YogAbilitySystemComponent.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "DevKit/AbilitySystem/Abilities/WeaponAbility.h"
#include "DevKit/AbilitySystem/Abilities/PassiveAbility.h"
#include "DevKit/AbilitySystem/Abilities/GeneralAbility.h"
#include "DevKit/SaveGame/YogSaveGame.h"



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
	//const TSubclassOf<UGameplayEffect> DynamicTagGE = ULyraAssetManager::GetSubclass(ULyraGameData::Get().DynamicTagGameplayEffect);
	//if (!DynamicTagGE)
	//{
	//	UE_LOG(LogLyraAbilitySystem, Warning, TEXT("RemoveDynamicTagGameplayEffect: Unable to find gameplay effect [%s]."), *ULyraGameData::Get().DynamicTagGameplayEffect.GetAssetName());
	//	return;
	//}

	FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
	//Query.EffectDefinition = DynamicTagGE;

	RemoveActiveEffects(Query);
}

void UYogAbilitySystemComponent::GetAbilityTargetData(const FGameplayAbilitySpecHandle AbilityHandle, FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle& OutTargetDataHandle)
{
}

FGameplayAbilitySpecHandle UYogAbilitySystemComponent::GrantAbility(TSubclassOf<UYogGameplayAbility> ability_class)
{
	FGameplayAbilitySpecHandle StoredAbilityHandle;

	FGameplayAbilitySpec AbilitySpec(ability_class, 1, 0); // Specify ability class, level, input ID
	StoredAbilityHandle = this->GiveAbility(AbilitySpec); // Store the handle!
	if (StoredAbilityHandle.IsValid())
	{
		if (ability_class == UWeaponAbility::StaticClass())
		{
			WeaponAbilities.Add(AbilitySpec);
		}
		if (ability_class == UPassiveAbility::StaticClass())
		{
			PassiveAbilities.Add(AbilitySpec);
		}
		if (ability_class == UGeneralAbility::StaticClass())
		{
			GeneralAbilities.Add(AbilitySpec);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("The AbilitySpecHandle is invalid."));
	}
	return StoredAbilityHandle;

}

void UYogAbilitySystemComponent::RemoveAbility(TSubclassOf<UYogGameplayAbility> ability_class)
{

	if (ability_class == UWeaponAbility::StaticClass())
	{
		for (int32 i = WeaponAbilities.Num() - 1; i >= 0; --i)
		{
			FGameplayAbilitySpec& ability_spec = WeaponAbilities[i];
			if (ability_spec.Ability && ability_spec.Ability->GetClass() == ability_class)
			{
				ClearAbility(ability_spec.Handle);
				WeaponAbilities.RemoveAt(i);
			}
		}

	}

	if (ability_class == UPassiveAbility::StaticClass())
	{
		for (int32 i = PassiveAbilities.Num() - 1; i >= 0; --i)
		{
			FGameplayAbilitySpec& ability_spec = PassiveAbilities[i];
			if (ability_spec.Ability && ability_spec.Ability->GetClass() == ability_class)
			{
				ClearAbility(ability_spec.Handle);
				PassiveAbilities.RemoveAt(i);
			}
		}
	}

	if (ability_class == UGeneralAbility::StaticClass())
	{
		for (int32 i = GeneralAbilities.Num() - 1; i >= 0; --i)
		{
			FGameplayAbilitySpec& ability_spec = GeneralAbilities[i];
			if (ability_spec.Ability && ability_spec.Ability->GetClass() == ability_class)
			{
				ClearAbility(ability_spec.Handle);
				GeneralAbilities.RemoveAt(i);
			}
		}
	}

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



bool UYogAbilitySystemComponent::TryActivateRandomAbilitiesByTag(const FGameplayTagContainer& GameplayTagContainer, bool bAllowRemoteActivation)
{
	TArray<FGameplayAbilitySpec*> AbilitiesToActivatePtrs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(GameplayTagContainer, AbilitiesToActivatePtrs);
	if (AbilitiesToActivatePtrs.Num() < 1)
	{
		return false;
	}

	// Convert from pointers (which can be reallocated, since they point to internal data) to copies of that data
	TArray<FGameplayAbilitySpec> AbilitiesToActivate;
	AbilitiesToActivate.Reserve(AbilitiesToActivatePtrs.Num());
	Algo::Transform(AbilitiesToActivatePtrs, AbilitiesToActivate, [](FGameplayAbilitySpec* SpecPtr) { return *SpecPtr; });

	bool bSuccess = false;

	int32 RandomIndex = FMath::RandRange(0, AbilitiesToActivate.Num() - 1);


	bSuccess |= TryActivateAbility(AbilitiesToActivate[RandomIndex].Handle, bAllowRemoteActivation);
	return bSuccess;
	//AbilitiesToActivate.Random
	//for (const FGameplayAbilitySpec& GameplayAbilitySpec : AbilitiesToActivate)
	//{
	//	ensure(IsValid(GameplayAbilitySpec.Ability));
	//	bSuccess |= TryActivateAbility(GameplayAbilitySpec.Handle, bAllowRemoteActivation);
	//}

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

TArray<FYogAbilitySaveData> UYogAbilitySystemComponent::GetAllGrantedAbilities()
{
	TArray<FYogAbilitySaveData> array_result;


	TArray<FGameplayAbilitySpec>& AbilitySpecs = this->GetActivatableAbilities();

	for (FGameplayAbilitySpec& Spec : AbilitySpecs)
	{
		if (UYogGameplayAbility* Ability = Cast<UYogGameplayAbility>(Spec.Ability))
		{
			FYogAbilitySaveData YogAbilitySaveData;

			YogAbilitySaveData.Level = Spec.Level;
			YogAbilitySaveData.AbilityClass = Ability->StaticClass();
			
			array_result.Add(YogAbilitySaveData);

			//UE_LOG(LogTemp, Warning, TEXT("granted abilities is: %s"), *Ability->GetName());
		}
	}

	return array_result;
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



