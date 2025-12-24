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
	FActionData* action_data = Owner->AbilityData->AbilityMap.Find(ability_tag);
	if (action_data)
	{
		cache_action_data->ActDamage = action_data->ActDamage;
		cache_action_data->ActRange = action_data->ActRange;
		cache_action_data->ActResilience = action_data->ActResilience;
		cache_action_data->ActDmgReduce = action_data->ActDmgReduce;

		////DANGER
		//action_data_CACHE = *action_data;

		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackAttribute(), action_data->ActDamage);
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackRangeAttribute(), action_data->ActRange);
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetResilienceAttribute(), action_data->ActResilience);
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetDmgTakenAttribute(), action_data->ActDmgReduce);

	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}




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
	//FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());
	//FActionData* action_data = Owner->AbilityData->AbilityMap.Find(ability_tag);
	if (cache_action_data.IsValid())
	{
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackAttribute(), -cache_action_data->ActDamage);
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetAttackRangeAttribute(), -cache_action_data->ActRange);
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetResilienceAttribute(), -cache_action_data->ActResilience);
		Owner->AttributeStatsComponent->AddAttribute(Owner->BaseAttributeSet->GetDmgTakenAttribute(), -cache_action_data->ActDmgReduce);

	}

}

