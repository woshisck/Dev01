#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_RuneBurn.generated.h"

/** Native 512 burn DOT GE. Damage comes from SetByCaller Data.Damage.Burn. */
UCLASS()
class DEVKIT_API UGE_RuneBurn : public UGameplayEffect
{
	GENERATED_BODY()

public:
	UGE_RuneBurn();
};
