#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HitCueData.generated.h"

class UNiagaraSystem;

/**
 * Per-hit visual payload for the shared hit-impact GameplayCue.
 * Passed to GCN_PlayerHitImpact via FGameplayCueParameters::SourceObject so one
 * cue tag/class can produce many different looks — one asset per weapon/notify,
 * instead of a unique cue notify per weapon.
 * Camera shake level is per-hit here; when left at 0 the cue falls back to the
 * global damage-scaled shake on UYogSettings::HitShakeConfig.
 * Impact SFX is target-scoped, not here — resolved per victim by
 * UHitImpactVisualComponent from the victim's material and state tags.
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UHitCueData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|VFX")
	TObjectPtr<UNiagaraSystem> ImpactVFX;

	// Rotation applied to the VFX spawn, relative to the hit surface normal.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|VFX")
	FRotator VFXRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|VFX")
	FVector VFXScale = FVector(1.f);

	// Camera shake level for this hit, resolved via UYogSettings::CameraShakeLevelTable
	// and played by AYogPlayerCameraManager::PlayShakeLevel. 0 = no level shake (the cue
	// falls back to the global damage-scaled HitShakeConfig).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake", meta = (ClampMin = "0"))
	int32 CameraShakeLevel = 0;

	// Overrides CameraShakeLevel when the swing landed a critical hit. 0 = use CameraShakeLevel.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake", meta = (ClampMin = "0"))
	int32 CritCameraShakeLevel = 0;
};
