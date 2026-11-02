// Fill out your copyright notice in the Description page of Project Settings.

#include "Animation/AN_EnemySkillEvent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GA_EnemyScriptedSkill.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_EnemySkillEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                 const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AActor* Owner = MeshComp ? MeshComp->GetOwner() : nullptr;
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	if (!ASC)
	{
		return;
	}

	// The animating ability is the one that owns the montage this notify sits on, so a skill montage
	// previewed in the editor or reused by another GA never reaches an unrelated skill.
	if (UGA_EnemyScriptedSkill* Skill = Cast<UGA_EnemyScriptedSkill>(ASC->GetAnimatingAbility()))
	{
		Skill->RaiseSkillEvent(EventName, nullptr);
	}
}

FString UAN_EnemySkillEvent::GetNotifyName_Implementation() const
{
	return FString::Printf(TEXT("SkillEvent: %s"), *EventName.ToString());
}
