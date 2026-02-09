// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attribute/EnemyAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"




UEnemyAttributeSet::UEnemyAttributeSet()
{	
	InitDropExp(0);
	InitKnockBackDist(0);
}


