#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_PlayerSpecialAttack.h"
#include "GA_Special.generated.h"

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Special : public UGA_PlayerSpecialAttack
{
	GENERATED_BODY()

public:
	UGA_Special(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
