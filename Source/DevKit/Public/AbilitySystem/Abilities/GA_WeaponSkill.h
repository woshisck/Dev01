#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GA_PlayMontage.h"
#include "GA_WeaponSkill.generated.h"

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill : public UGA_PlayMontage
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	void ConfigureWeaponSkillComboTag(const TCHAR* ComboTagName);
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo1 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo1(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo2 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo2(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo3 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo3(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_WeaponSkill_Combo4 : public UGA_WeaponSkill
{
	GENERATED_BODY()

public:
	UGA_WeaponSkill_Combo4(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
