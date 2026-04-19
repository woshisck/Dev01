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
    CurrentAmmo          = 6.0f;
    MaxAmmo              = 6.0f;
}

void UPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetCurrentAmmoAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, GetMaxAmmo());
    }
    else if (Attribute == GetMaxAmmoAttribute())
    {
        NewValue = FMath::Max(NewValue, 1.f);
    }
}
