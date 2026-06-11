#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_WeaponSkill.generated.h"

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill(const FObjectInitializer& ObjectInitializer);


};
