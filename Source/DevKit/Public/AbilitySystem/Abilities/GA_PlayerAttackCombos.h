#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "GA_PlayerAttackCombos.generated.h"

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_PlayerAttack_Combo1 : public UGA_MeleeAttack
{
	GENERATED_BODY()

public:
	UGA_PlayerAttack_Combo1();
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_PlayerAttack_Combo2 : public UGA_MeleeAttack
{
	GENERATED_BODY()

public:
	UGA_PlayerAttack_Combo2();
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_PlayerAttack_Combo3 : public UGA_MeleeAttack
{
	GENERATED_BODY()

public:
	UGA_PlayerAttack_Combo3();
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_PlayerAttack_Combo4 : public UGA_MeleeAttack
{
	GENERATED_BODY()

public:
	UGA_PlayerAttack_Combo4();
};
