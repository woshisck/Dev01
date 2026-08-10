#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_PlayerHitImpact.generated.h"

class UNiagaraSystem;

/**
 * One-shot GameplayCue notify for player-hits-enemy impact feedback.
 * Spawns a Niagara VFX at the hit location and plays camera shake. Impact SFX
 * is target-scoped and played by the ability per victim, not here.
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	bool bSkipDedicatedServer = true;
};
