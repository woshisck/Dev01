// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_MeleeAttackFrame.generated.h"

/**
 * Attack-frame stat window applied by AN_MeleeDamage when the notify fires.
 * Removed by the notify when the montage ends (via OnMontageEnded binding).
 *
 * Modifiers (all magnitudes SetByCaller — caller must set all three before applying):
 *   BaseAttributeSet.Attack       Additive  Attribute.ActDamage
 *   BaseAttributeSet.AttackRange  Override  Attribute.ActRange
 *   BaseAttributeSet.Resist       Override  Attribute.ActDmgReduce
 *
 * Resilience is deliberately absent: it is the defender's poise pool, so overriding it
 * on the attacker would wipe the pool every time this character swings. Per-attack poise
 * damage travels via UYogAbilitySystemComponent::CurrentActionPoiseDamage instead.
 */
UCLASS()
class DEVKIT_API UGE_MeleeAttackFrame : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_MeleeAttackFrame();
};
