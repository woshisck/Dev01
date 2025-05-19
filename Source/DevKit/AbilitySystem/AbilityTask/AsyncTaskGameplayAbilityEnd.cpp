// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncTaskGameplayAbilityEnd.h"
#include "../Abilities/YogGameplayAbility.h"
#include <AbilitySystemComponent.h>

UAsyncTaskGameplayAbilityEnd::UAsyncTaskGameplayAbilityEnd(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAsyncTaskGameplayAbilityEnd* UAsyncTaskGameplayAbilityEnd::ListenForGameplayAbilityEnd(UObject* InWorldContextObject, UAbilitySystemComponent* abilitySystemComponent, TSubclassOf<UYogGameplayAbility> abilityClass)
{
	UAsyncTaskGameplayAbilityEnd* Action = nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		Action = NewObject<UAsyncTaskGameplayAbilityEnd>();
		Action->WorldPtr = World;
		Action->RegisterWithGameInstance(World);
		Action->TargetASC = abilitySystemComponent;
		Action->AbilityClass = abilityClass;

		//auto abilitySpec = abilitySystemComponent->FindAbilitySpecFromClass(abilityClass);
		//auto abilityInstance = abilitySpec->GetPrimaryInstance();
		//UYogGameplayAbility* abilityCrow = Cast<UYogGameplayAbility>(abilityInstance);
		////TODO: Call event delegate handle
		//Action->AbilityListeningTo = abilityCrow;

		////abilityCrow->EventOn_AbilityEnded.AddDynamic(this, &UAsyncTaskGameplayAbilityEnd::OnCallback);
		//abilityCrow->EventOn_AbilityEnded.AddDynamic(Action, &UAsyncTaskGameplayAbilityEnd::OnCallback);


		return Action;

	}
	return Action;

}

void UAsyncTaskGameplayAbilityEnd::Activate()
{
	auto abilitySpec = TargetASC->FindAbilitySpecFromClass(AbilityClass);
	auto abilityInstance = abilitySpec->GetPrimaryInstance();
	UYogGameplayAbility* abilityCrow = Cast<UYogGameplayAbility>(abilityInstance);
	//TODO: Call event delegate handle
	this->AbilityListeningTo = abilityCrow;

	//abilityCrow->EventOn_AbilityEnded.AddDynamic(this, &UAsyncTaskGameplayAbilityEnd::OnCallback);
	abilityCrow->EventOn_AbilityEnded.AddDynamic(this, &UAsyncTaskGameplayAbilityEnd::OnCallback);
}



void UAsyncTaskGameplayAbilityEnd::OnCallback()
{
	OnEnded.Broadcast();
	SetReadyToDestroy();
}


