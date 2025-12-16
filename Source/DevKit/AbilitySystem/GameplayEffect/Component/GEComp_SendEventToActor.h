// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GEComp_SendEventToActor.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UGEComp_SendEventToActor : public UGameplayEffectComponent
{
	GENERATED_BODY()
	
	UGEComp_SendEventToActor();

	/** Register for the appropriate events when we're applied */
	virtual bool OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const override;


	virtual void OnGameplayEffectApplied(FActiveGameplayEffectsContainer& ActiveGEContainer, FGameplayEffectSpec& GESpec, FPredictionKey& PredictionKey) const override;

	/** Adds an entry for granting Gameplay Abilities */
	void SendEventDataToActor();

#if WITH_EDITOR
	/** Warn on misconfigured Gameplay Effect */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

};
