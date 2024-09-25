// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Settings/ComboGraphProjectSettings.h"


/** Library of helpers to handle various utilities. */
class COMBOGRAPH_API FComboGraphUtils
{
public:
	/** Checks if the owner actor of this notify is persona preview actor (when asset played in persona viewports), in which case notify logic should be ignored. */
	static bool IsAnimationPreviewActor(const AActor* OwnerActor);

	/** Returns plugin settings from Editor config */
	static const UComboGraphProjectSettings* GetPluginProjectSettings();

	static FString GetWorldLogPrefix(UWorld* World);
};
