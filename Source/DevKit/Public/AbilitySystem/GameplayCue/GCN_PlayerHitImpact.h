#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_PlayerHitImpact.generated.h"

class UNiagaraSystem;
class USoundBase;
class UCameraShakeBase;

/**
 * One-shot GameplayCue notify for player-hits-enemy impact feedback.
 * Spawns a Niagara VFX and sound at the hit location, and optionally shakes the local player camera.
 * Triggered via ExecuteGameplayCue — not intended for persistent (active/remove) use.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API AGCN_PlayerHitImpact : public AGameplayCueNotify_Actor
{
	GENERATED_BODY()

public:
	AGCN_PlayerHitImpact();

	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) override;

protected:
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	bool bSkipDedicatedServer = true;
};
