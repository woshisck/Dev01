// Fill out your copyright notice in the Description page of Project Settings.


#include "ActionAbility.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"
#include "DevKit/Character/YogCharacterBase.h"

UActionAbility::UActionAbility(const FObjectInitializer& ObjectInitializer)
{
}

void UActionAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	ASC->CurrentAbilitySpecHandle = Handle;


	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());
	if (Owner->AbilityData)
	{
		ActionData_cache = Owner->AbilityData->GetAbility(ability_tag);
	}

	Owner->UpdateCharacterState(EYogCharacterState::OnAction, FVector(0, 0, 0));

}

void UActionAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);



}

void UActionAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}



FActionData& UActionAbility::GetActionDataCache()
{
	return ActionData_cache;
}
