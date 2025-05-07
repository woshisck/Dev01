// Fill out your copyright notice in the Description page of Project Settings.


#include "AsyncTaskGameplayAbilityEnd.h"
#include <AbilitySystemComponent.h>

UAsyncTaskGameplayAbilityEnd* UAsyncTaskGameplayAbilityEnd::ListenForGameplayAbilityEnd(UAbilitySystemComponent* abilitySystemComponent, TSubclassOf<UGameplayAbility> abilityClass)
{
	//if (!IsValid(abilitySystemComponent))
	//{

	//	return nullptr;
	//}

	//auto abilitySpec = abilitySystemComponent->FindAbilitySpecFromClass(abilityClass);
	//if (abilitySpec == nullptr)
	//{
	//
	//	return nullptr;
	//}

	//auto abilityInstance = abilitySpec->GetPrimaryInstance();
	//if (abilityInstance == nullptr)
	//{
	//	return nullptr;
	//}

	//UGameplayAbility_Crow* abilityCrow = Cast<UGameplayAbility_Crow>(abilityInstance);
	//if (abilityCrow == nullptr)
	//{
	//	//Print::Say("Couldn't create Task, Ability " + abilityClass->GetName() + " needs to inherit from UGameplayAbility_Crow");
	//	return nullptr;
	//}

	//UAsyncTaskGameplayAbilityEnd* r = NewObject<UAsyncTaskGameplayAbilityEnd>();
	//abilityCrow->EventOn_AbilityEnded.AddDynamic(r, &UAsyncTaskGameplayAbilityEnded::OnCallback);
	//r->AbilityListeningTo = abilityCrow;

	//return r;
}

void UAsyncTaskGameplayAbilityEnd::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

}

void UAsyncTaskGameplayAbilityEnd::OnCallback(const FGameplayTag CallbackTag, int32 NewCount)
{

}
