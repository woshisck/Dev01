// Copyright 2022 Mickael Daniel. All Rights Reserved.


#include "Abilities/ComboGraphAbilitySystemGlobals.h"

#include "ComboGraphAbilityTypes.h"
#include "ComboGraphLog.h"

FGameplayEffectContext* UComboGraphAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	CG_RUNTIME_LOG(Verbose, TEXT("UComboGraphAbilitySystemGlobals::AllocGameplayEffectContext"))
	return new FComboGraphGameplayEffectContext();
}
