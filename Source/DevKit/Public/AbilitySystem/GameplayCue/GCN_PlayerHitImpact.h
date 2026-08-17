#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "GCN_PlayerHitImpact.generated.h"

/**
 * One-shot GameplayCue notify for player-hits-enemy impact feedback. Camera shake only.
 *
 * Responsibility boundary — hit feedback is split by who owns the knowledge:
 *  · VFX + SFX   → the victim's UHitImpactVisualComponent, resolved per victim from
 *                  UEnemyHitImpactData against that victim's material and state tags.
 *  · Camera shake → here, because shake is a player-global concern that must fire once
 *                  per swing rather than once per victim.
 *
 * Do not add VFX or sound fields back to this class. It previously spawned its own Niagara,
 * which could double up with the victim table's VFX on the same swing since nothing
 * coordinated the two.
 *
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HitImpact")
	bool bSkipDedicatedServer = true;
};
