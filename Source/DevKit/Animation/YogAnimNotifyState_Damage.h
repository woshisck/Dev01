// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogAnimNotifyState.h"
#include "YogAnimNotifyState_Damage.generated.h"

/**
 * 
 */
USTRUCT()
struct FWeaponSocketLoc
{
public:
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FVector DmgBox_StartLoc = FVector(0,0,0);

	UPROPERTY()
	FVector DmgBox_End = FVector(0, 0, 0);
};

UCLASS()
class DEVKIT_API UYogAnimNotifyState_Damage : public UYogAnimNotifyState
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TArray<FWeaponSocketLoc> array_DamageBox; //[Start, end] 2 index array 

	UFUNCTION()
	void InsertSocketData(FWeaponSocketLoc loc);

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual void Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const;


};
