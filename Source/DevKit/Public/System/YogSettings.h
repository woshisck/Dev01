#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YogSettings.generated.h"

/**
 * Project Settings → Game → Yog
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Yog"))
class DEVKIT_API UYogSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UYogSettings* Get() { return GetDefault<UYogSettings>(); }
};
