// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAnimNotifyState_Damage.h"
#include <DevKit/Character/YogCharacterBase.h>
#include "../Item/Weapon/WeaponInstance.h"





void UYogAnimNotifyState_Damage::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);


}

void UYogAnimNotifyState_Damage::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

}

void UYogAnimNotifyState_Damage::Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const
{
	Super::Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	//TODO: called when received tick, NOT CALL WITH EVERY TICK
		
}

void UYogAnimNotifyState_Damage::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);


}




