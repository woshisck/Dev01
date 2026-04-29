// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "ComboGraphAbilitySystemGlobals.generated.h"

/**
 * UAbilitySystemGlobals holds global data for the ability system. Can be configured per project via config file.
 *
 * This subclass is designed to handle custom Gameplay Effect Context to return FComboGraphGameplayEffectContext which is required for
 * Combo Nodes Gameplay Cue Containers to be able to pass down custom and arbitrary cue parameters to gameplay cue handlers.
 */
UCLASS()
class COMBOGRAPH_API UComboGraphAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:

	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
