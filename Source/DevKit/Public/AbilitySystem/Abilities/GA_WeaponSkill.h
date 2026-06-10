#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "GA_WeaponSkill.generated.h"

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill : public UGA_MeleeAttack
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill();
};
