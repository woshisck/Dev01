// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSequenceBase.h"

class UGameplayAbility;

struct GASCOMPANION_API FGSCDelegates
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FGSCDebugWidgetAnimMontage, UAnimSequenceBase*);
	DECLARE_MULTICAST_DELEGATE_OneParam(FGSCDebugWidgetUpdateAllowedAbilities, TArray<TSubclassOf<UGameplayAbility>>);

	/** Called to notify ability queue debug widget about montage infos. */
	static FGSCDebugWidgetAnimMontage OnAddAbilityQueueFromMontageRow;

	/** Called to notify ability queue debug widget about allowed abilities. */
	static FGSCDebugWidgetUpdateAllowedAbilities OnUpdateAllowedAbilities;
};
