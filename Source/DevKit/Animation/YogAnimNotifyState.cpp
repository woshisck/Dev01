// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAnimNotifyState.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"

void UYogAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration)
{
}

void UYogAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
}

FString UYogAnimNotifyState::GetNotifyName_Implementation() const
{
	return FString("AbilityQueueWindow");
}