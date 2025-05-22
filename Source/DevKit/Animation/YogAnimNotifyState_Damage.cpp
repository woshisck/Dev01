// Fill out your copyright notice in the Description page of Project Settings.


#include "YogAnimNotifyState_Damage.h"
#include <DevKit/Character/YogCharacterBase.h>


void UYogAnimNotifyState_Damage::InsertSocketData(FWeaponSocketLoc loc)
{
	array_DamageBox.Pop();
	array_DamageBox.Insert(loc, 0);
}

void UYogAnimNotifyState_Damage::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	array_DamageBox.Empty();
}

void UYogAnimNotifyState_Damage::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	array_DamageBox.Empty();
}

void UYogAnimNotifyState_Damage::Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const
{
	Super::Received_NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);
	AYogCharacterBase* Player = Cast<AYogCharacterBase>(MeshComp->GetOwner());
	
}



