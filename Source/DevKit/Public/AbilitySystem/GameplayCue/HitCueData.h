#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HitCueData.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

/**
 * Per-hit visual payload for the shared hit-impact GameplayCue.
 * Passed to GCN_PlayerHitImpact via FGameplayCueParameters::SourceObject so one
 * cue tag/class can produce many different looks — one asset per weapon/notify,
 * instead of a unique cue notify per weapon.
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|SFX")
	TObjectPtr<USoundBase> ImpactSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|SFX", meta = (ClampMin = "0.0"))
	float SoundVolumeMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|SFX", meta = (ClampMin = "0.0"))
	float SoundPitchMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact|CameraShake", meta = (ClampMin = "0.0"))
	float CameraShakeScale = 1.f;
};
