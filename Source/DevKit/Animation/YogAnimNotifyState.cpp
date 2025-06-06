// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAnimNotifyState.h"
#include "../AbilitySystem/Abilities/YogGameplayAbility.h"

void UYogAnimNotifyState::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	//TODO: Set animation montage duration time
	//TotalDuration = NotifyDuration;
}

void UYogAnimNotifyState::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
}

void UYogAnimNotifyState::Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const
{
	Super::Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
}



//FString UYogAnimNotifyState::GetNotifyName_Implementation() const
//{
//	return FString("AbilityQueueWindow");
//}
