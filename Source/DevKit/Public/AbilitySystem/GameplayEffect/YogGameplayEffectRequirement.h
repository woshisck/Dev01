// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectCustomApplicationRequirement.h"
#include "YogGameplayEffect.h"
#include "YogGameplayEffectRequirement.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UYogGameplayEffectRequirement : public UGameplayEffectCustomApplicationRequirement
{
	GENERATED_BODY()

public:

    virtual bool CanApplyGameplayEffect_Implementation(const UGameplayEffect* GameplayEffect, const FGameplayEffectSpec& Spec, UAbilitySystemComponent* ASC) const override;



};
