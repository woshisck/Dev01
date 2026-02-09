// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "YogAnimNotifyState.generated.h"

class UYogGameplayAbility;
/**
 * 
 */
UCLASS()
class DEVKIT_API UYogAnimNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()
public:
	

	//UPROPERTY(BlueprintReadWrite,Category = "player buffer")
	//int32 ActInputBuffer_start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "player buffer")
	uint32 ActBufferUpdated : 1;


	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimNotify")
	//TArray<TSubclassOf<UYogGameplayAbility>> AllowedAbilities;

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	virtual void Received_NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) const;

	
	//virtual FString GetNotifyName_Implementation() const override;
	
	UPROPERTY()
	float NotifyDuration;

};
