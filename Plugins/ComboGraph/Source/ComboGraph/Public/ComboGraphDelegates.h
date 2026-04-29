// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class APawn;
class UComboGraph;
class UComboGraphAbilityTask_StartGraph;

struct COMBOGRAPH_API FComboGraphDelegates
{
	/** delegate type for combo graph execution events (Params: const UComboGraphAbilityTask_StartGraph*, const UComboGraph* ComboGraphAsset) */
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnComboGraphStarted, const UComboGraphAbilityTask_StartGraph&, const UComboGraph&);

	/** Called when the Combo Graph starts execution. */
	static FOnComboGraphStarted OnComboGraphStarted;

	/** Called when the Combo Graph ends execution. */
	static FOnComboGraphStarted OnComboGraphEnded;
};
