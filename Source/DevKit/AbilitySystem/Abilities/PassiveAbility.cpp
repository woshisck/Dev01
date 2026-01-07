// Fill out your copyright notice in the Description page of Project Settings.


#include "PassiveAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"
#include "DevKit/Character/YogCharacterBase.h"

UPassiveAbility::UPassiveAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UPassiveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent);
	ASC->CurrentAbilitySpecHandle = Handle;


	//AYogCharacterBase* Owner = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
	//FGameplayTag ability_tag = this->GetFirstTagFromContainer(GetAbilityTags());
	//if (Owner->AbilityData)
	//{
	//	PassiveData_cache = Owner->AbilityData->GetPassiveAbility(ability_tag);
	//}

	//Owner->UpdateCharacterState(EYogCharacterState::Action, FVector(0, 0, 0));
}

void UPassiveAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	//ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);


}

void UPassiveAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);


	// UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	// if (ASC)
	// {
	// 	ASC->ClearAbility(Handle);
	// }
}

//void UPassiveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
