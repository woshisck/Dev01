// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameplayEffect.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <AbilitySystemGlobals.h>
#include <AbilitySystemBlueprintLibrary.h>


void UYogGameplayEffect::CreateOwnEffectContext(AActor* TargetActor)
{
    UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

}
