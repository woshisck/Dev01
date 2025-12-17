// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectComponent.h"
#include "GameplayEffectComponents/AbilitiesGameplayEffectComponent.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GEComp_SendEventToActor.generated.h"

/**
 * 
 */
UCLASS(DisplayName = "Send Event To Target Actor")
class DEVKIT_API UGEComp_SendEventToActor : public UAbilitiesGameplayEffectComponent
{
	GENERATED_BODY()
public:

	UGEComp_SendEventToActor();


public:
	// Override the protected function
	virtual void OnInhibitionChanged(FActiveGameplayEffectHandle ActiveGEHandle, bool bIsInhibited) const override;


public:

	UPROPERTY(EditDefaultsOnly, Category = EventData)
	FGameplayTag Trigger_EventTag;
;


};
