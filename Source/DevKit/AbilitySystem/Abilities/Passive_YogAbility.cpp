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
