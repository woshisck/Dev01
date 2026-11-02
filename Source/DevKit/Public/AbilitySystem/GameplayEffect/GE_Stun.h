#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_Stun.generated.h"

/**
 * Grants Buff.Status.Stunned for its duration. APlayerCharacterBase reacts to the tag by locking movement,
 * and AYogPlayerControllerBase::IsGameplayInputBlocked treats it as blocked input.
 *
 * Duration defaults to 1.5 s; make a Blueprint child and edit Duration Magnitude for other lengths.
 * Re-applying refreshes the duration instead of stacking.
 */
UCLASS()
class DEVKIT_API UGE_Stun : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_Stun();
};
