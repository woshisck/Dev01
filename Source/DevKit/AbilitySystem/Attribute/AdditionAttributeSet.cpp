// Fill out your copyright notice in the Description page of Project Settings.


#include "AdditionAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include <DevKit/AbilitySystem/YogAbilitySystemComponent.h>
#include <DevKit/Character/YogCharacterBase.h>
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UAdditionAttributeSet::UAdditionAttributeSet()
{	

	InitAdd_Attack(0);
	InitAdd_AttackPower(0);
	InitAdd_MaxHealth(0);
	InitAdd_Health(0);
	InitAdd_Shield(0);
	InitAdd_AttackSpeed(0);
	InitAdd_AttackRange(0);
	InitAdd_Sanity(0);
	InitAdd_MoveSpeed(0);
	InitAdd_Dodge(0);
	InitAdd_Resilience(0);
	InitAdd_Resist(0);
	InitAdd_DmgTaken(0);
	InitAdd_Crit_Rate(0);
	InitAdd_Crit_Damage(0);

}
