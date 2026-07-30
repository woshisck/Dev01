#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YogSettings.generated.h"

class UGlobalHitShakeData;

/**
 * Project Settings → Game → Yog
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Yog"))
class DEVKIT_API UYogSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UYogSettings* Get() { return GetDefault<UYogSettings>(); }

	// Global camera-shake config for the player-hits-enemy impact cue. Bigger damage = bigger shake.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TSoftObjectPtr<UGlobalHitShakeData> HitShakeConfig;
};
