// Fill out your copyright notice in the Description page of Project Settings.


#include "WeaponAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <DevKit/Character/YogCharacterBase.h>
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UWeaponAttributeSet::UWeaponAttributeSet()
{	
    InitWeaponAtk(0);
    InitWeaponAtkPower(0);
    InitWeaponAtkRange(0);
    InitWeapon_CritRate(0);
    InitWeapon_CritDmg(1);

}
