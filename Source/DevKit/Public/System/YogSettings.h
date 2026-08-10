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

	// DataTable of FBuffHitFeedbackRow: buff GameplayTag -> on-hit VFX/SFX/shake for a buffed victim.
	// Resolved per victim by GA_MeleeAttack when a hit lands.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSoftObjectPtr<UDataTable> BuffHitFeedbackTable;

	// Shared per-tier enemy hit sound/VFX (Soft/Hard/Immaterial). Resolved by UHitImpactVisualComponent.
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Combat")
	TSoftObjectPtr<UEnemyHitImpactData> EnemyHitImpactData;
};
