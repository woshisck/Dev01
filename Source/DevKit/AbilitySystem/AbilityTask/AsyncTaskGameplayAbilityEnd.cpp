// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncTaskGameplayAbilityEnd.h"
#include "../Abilities/YogGameplayAbility.h"
#include <AbilitySystemComponent.h>

UAsyncTaskGameplayAbilityEnd* UAsyncTaskGameplayAbilityEnd::ListenForGameplayAbilityEnd(UAbilitySystemComponent* abilitySystemComponent, TSubclassOf<UGameplayAbility> abilityClass)
{
	if (!IsValid(abilitySystemComponent))
	{
		return nullptr;
	}
	auto abilitySpec = abilitySystemComponent->FindAbilitySpecFromClass(abilityClass);
	if (abilitySpec == nullptr)
	{
	
		return nullptr;
	}
	auto abilityInstance = abilitySpec->GetPrimaryInstance();
	if (abilityInstance == nullptr)
	{
		return nullptr;
	}
	UYogGameplayAbility* abilityCrow = Cast<UYogGameplayAbility>(abilityInstance);
	if (abilityCrow == nullptr)
	{
		//Print::Say("Couldn't create Task, Ability " + abilityClass->GetName() + " needs to inherit from UGameplayAbility_Crow");
		return nullptr;
	}



	UAsyncTaskGameplayAbilityEnd* Action = NewObject<UAsyncTaskGameplayAbilityEnd>();
	//TODO: Call event delegate handle
	abilityCrow->EventOn_AbilityEnded.AddDynamic(Action, &UAsyncTaskGameplayAbilityEnd::OnCallback);
	Action->AbilityListeningTo = abilityCrow;
	return Action;
}

void UAsyncTaskGameplayAbilityEnd::Activate()
{
}



void UAsyncTaskGameplayAbilityEnd::OnCallback()
{
	OnEnded.Broadcast();
}
