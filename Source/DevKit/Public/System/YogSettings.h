#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YogSettings.generated.h"

class UGlobalHitShakeData;
class UDataTable;
class UEnemyHitImpactData;

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

	// DataTable of FCameraShakeLevelRow: integer level -> camera shake (heavy attack = 1, crit = 2, ...).
	// Played through AYogPlayerCameraManager::PlayShakeLevel.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TSoftObjectPtr<UDataTable> CameraShakeLevelTable;

	// Victim-side hit sound/VFX, keyed by the victim's material and state tags.
	// Resolved by UHitImpactVisualComponent on every landed hit.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSoftObjectPtr<UEnemyHitImpactData> EnemyHitImpactData;
};
