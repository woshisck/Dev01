// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncTaskGameplayAbilityEnd.h"
#include "../Abilities/YogGameplayAbility.h"
#include <AbilitySystemComponent.h>

UAsyncTaskGameplayAbilityEnd* UAsyncTaskGameplayAbilityEnd::ListenForGameplayAbilityEnd(UAbilitySystemComponent* abilitySystemComponent, TSubclassOf<UGameplayAbility> abilityClass)
{
	if (!IsValid(abilitySystemComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("InValid abilitySystemComponent"));
		return nullptr;
	}
	auto abilitySpec = abilitySystemComponent->FindAbilitySpecFromClass(abilityClass);
	if (abilitySpec == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("InValid abilitySpec"));
		return nullptr;
	}
	auto abilityInstance = abilitySpec->GetPrimaryInstance();
	if (abilityInstance == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("InValid abilityInstance"));
		return nullptr;
	}
	UYogGameplayAbility* abilityCrow = Cast<UYogGameplayAbility>(abilityInstance);
	if (abilityCrow == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("InValid abilityCrow"));
		return nullptr;
	}



	UAsyncTaskGameplayAbilityEnd* lv = NewObject<UAsyncTaskGameplayAbilityEnd>();


	//TODO: Call event delegate handle
	abilityCrow->EventOn_AbilityEnded.AddDynamic(lv, &UAsyncTaskGameplayAbilityEnd::OnCallback);
	lv->AbilityListeningTo = abilityCrow;
	return lv;
}



void UAsyncTaskGameplayAbilityEnd::OnCallback()
{
	OnEnded.Broadcast();
}
