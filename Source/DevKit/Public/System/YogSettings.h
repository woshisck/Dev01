#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YogSettings.generated.h"

class UEffectRegistry;

/**
 * Project Settings → Game → Yog
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Yog"))
class DEVKIT_API UYogSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Global EffectRegistry DataAsset (Tag → BuffDefinition lookup) */
	UPROPERTY(Config, EditAnywhere, Category = "Effect")
	TSoftObjectPtr<UEffectRegistry> DefaultEffectRegistry;

	static const UYogSettings* Get() { return GetDefault<UYogSettings>(); }
};
