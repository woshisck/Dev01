// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attribute/PlayerAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UPlayerAttributeSet::UPlayerAttributeSet()
{
    MaxDashCharge        = 1.0f;
    DashCooldownDuration = 1.0f;
}

void UPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
}
