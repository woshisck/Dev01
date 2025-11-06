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

	UPROPERTY(VisibleAnywhere)
	FGameplayEffectAttributeCaptureDefinition CapturedAttribute;
	
};
