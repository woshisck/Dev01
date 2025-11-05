// Fill out your copyright notice in the Description page of Project Settings.


#include "GetHitAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "DevKit/Player/PlayerCharacterBase.h"
#include "AbilitySystemComponent.h"

UGetHitAbility::UGetHitAbility(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	//NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UGetHitAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);


}

void UGetHitAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);


	if (Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
		Player->UpdateCharacterState(EYogCharacterState::OnHurt, FVector(0,0,0));
		
	}



}

void UGetHitAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);


	if (Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo());
		Player->UpdateCharacterState(EYogCharacterState::Idle, FVector(0, 0, 0));

	}


	// UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	// if (ASC)
	// {
	// 	ASC->ClearAbility(Handle);
	// }
}

//void UGetHitAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
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
