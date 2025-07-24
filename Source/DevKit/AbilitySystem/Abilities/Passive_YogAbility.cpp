// Fill out your copyright notice in the Description page of Project Settings.


#include "Passive_YogAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.h"

UPassive_YogAbility::UPassive_YogAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPassive_YogAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);


}

void UPassive_YogAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);


	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (ASC)
	{
		ASC->ClearAbility(Handle);
	}
}

//void UPassive_YogAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
//{
//	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
//	{
//		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
//		return;
//	}
//	UE_LOG(LogTemp, Warning, TEXT("Ability Activated"));
//
//
//	if (TriggerEventData)
//	{
//		// Access data in TriggerEventData, like instigator, target, etc.
//	}
//
//	EndAbility(Handle, ActorInfo, ActivationInfo, false, false);
//
//}
