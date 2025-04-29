// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "YogGameplayEffect.generated.h"

/**
 * 
 */
UCLASS()
class DEVKIT_API UYogGameplayEffect : public UGameplayEffect
{
	GENERATED_BODY()
	
public:

	UFUNCTION(BlueprintCallable)
	void CreateOwnEffectContext(AActor* TargetActor);


protected:
	// Store the effect context.  Crucial for passing it along.
	FGameplayEffectContextHandle EffectContextHandle;

};
