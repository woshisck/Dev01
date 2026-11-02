// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_EnemySkillEvent.generated.h"

/**
 * Raises a named event on the UGA_EnemyScriptedSkill that is playing this montage, which forwards it to the
 * skill asset's OnEvent (e.g. "WindupEnd" -> start the charge). Does nothing on any other montage.
 */
UCLASS(meta = (DisplayName = "Enemy Skill Event"))
class DEVKIT_API UAN_EnemySkillEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy Skill")
	FName EventName = TEXT("WindupEnd");

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};
