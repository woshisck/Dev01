// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_MeleeAttackFrame.generated.h"

/**
 * Attack-frame stat window applied by AN_MeleeDamage when the notify fires.
 * Removed by the notify when the montage ends (via OnMontageEnded binding).
 *
 * Modifiers (all magnitudes SetByCaller — caller must set all four before applying):
 *   BaseAttributeSet.AttackPower  Additive  Attribute.ActDamage
 *   BaseAttributeSet.AttackRange  Override  Attribute.ActRange
 *   BaseAttributeSet.Resilience   Override  Attribute.ActResilience
 *   BaseAttributeSet.Resist       Override  Attribute.ActDmgReduce
 */
UCLASS()
class DEVKIT_API UGE_MeleeAttackFrame : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_MeleeAttackFrame();
};
