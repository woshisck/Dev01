// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayCue/GamePlayCue_MontageSlow.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"


bool UGamePlayCue_MontageSlow::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{

    return true;
}

bool UGamePlayCue_MontageSlow::OnRemove_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{

    return true;
}