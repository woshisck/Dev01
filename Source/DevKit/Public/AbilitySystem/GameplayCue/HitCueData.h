#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HitCueData.generated.h"

/**
 * Per-hit camera-shake payload for the shared hit-impact GameplayCue.
 * Passed to GCN_PlayerHitImpact via FGameplayCueParameters::SourceObject so one cue
 * tag/class can shake differently per weapon/notify, instead of needing a unique cue
 * notify per weapon. When CameraShakeLevel is 0 the cue falls back to the global
 * damage-scaled shake on UYogSettings::HitShakeConfig.
 *
 * Shake only, by design. Both VFX and SFX are target-scoped and resolved per victim by
 * UHitImpactVisualComponent from UEnemyHitImpactData against the victim's material and
 * state tags. This asset used to also carry ImpactVFX, which could double up with the
 * victim table's VFX on the same swing — nothing coordinated the two.
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UHitCueData : public UDataAsset
{
	GENERATED_BODY()

public:
	// Camera shake level for this hit, resolved via UYogSettings::CameraShakeLevelTable
	// and played by AYogPlayerCameraManager::PlayShakeLevel. 0 = no level shake (the cue
	// falls back to the global damage-scaled HitShakeConfig).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake", meta = (ClampMin = "0"))
	int32 CameraShakeLevel = 0;

	// Overrides CameraShakeLevel when the swing landed a critical hit. 0 = use CameraShakeLevel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake", meta = (ClampMin = "0"))
	int32 CritCameraShakeLevel = 0;
};
