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
	//FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());
	//FActionData* action_data = Owner->AbilityData->AbilityMap.Find(ability_tag);

	////DANGER
	//action_data_CACHE = *action_data;

	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackAttribute(), ActDamage);
	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackRangeAttribute(), ActRange);
	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetResilienceAttribute(), ActResilience);
	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetDmgTakenAttribute(), ActDmgReduce);



	//if (Owner && Owner->GetMesh() && Owner->AbilityData)
	//{
	//	UAnimInstance* AnimInstance = Owner->GetMesh()->GetAnimInstance();
	//	if (AnimInstance && AnimInstance->Montage_IsPlaying(Owner->AbilityData->Montage))
	//	{
	//		// Montage is already playing, handle accordingly
	//		// Either stop it first or skip playing new one
	//		AnimInstance->Montage_Stop(0.25f, ActionData_cache->Montage);
	//	}
	//	if (Owner->AbilityData)
	//	{
	//		SetCurrentMontage(ActionData_cache->Montage);
	//		//ActionData_cache = &Owner->AbilityData->GetAbility(ability_tag);
	//	}
	//}
	// 
	//CurrentMontage = ActionData_cache.Montage;


	//Owner->UpdateCharacterState(EYogCharacterState::OnAction, FVector(0, 0, 0));





}


void UActionAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());

	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackAttribute(), -ActDamage);
	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackRangeAttribute(), -ActRange);
	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetResilienceAttribute(), -ActResilience);
	Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetDmgTakenAttribute(), -ActDmgReduce);
}

