#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/CharacterData.h"
#include "Data/EnemyData.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "EnemyWeaponDefinition.generated.h"

class UAbilityData;
class UGameplayEffect;

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyWeaponAttributeModifiers
{
	GENERATED_BODY()

	FEnemyWeaponAttributeModifiers()
	{
		Add.Attack = 0.0f;
		Add.AttackPower = 0.0f;
		Add.MaxHealth = 0.0f;
		Add.MaxHeat = 0.0f;
		Add.Shield = 0.0f;
		Add.AttackSpeed = 0.0f;
		Add.AttackRange = 0.0f;
		Add.Sanity = 0.0f;
		Add.MoveSpeed = 0.0f;
		Add.Dodge = 0.0f;
		Add.Resilience = 0.0f;
		Add.Resist = 0.0f;
		Add.DmgTaken = 0.0f;
		Add.Crit_Rate = 0.0f;
		Add.Crit_Damage = 0.0f;
		Add.MaxArmorHP = 0.0f;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Add")
	FYogBaseAttributeData Add;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Multiply", meta = (ClampMin = "0.0"))
	float AttackPowerMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Multiply", meta = (ClampMin = "0.0"))
	float AttackSpeedMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attributes|Multiply", meta = (ClampMin = "0.0"))
	float MoveSpeedMultiplier = 1.0f;
};

UCLASS(BlueprintType, Blueprintable, DisplayName = "Enemy Weapon Definition")
class DEVKIT_API UEnemyWeaponDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|Combat")
	TObjectPtr<UAbilityData> AbilityData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|AI")
	bool bOverrideAttackProfile = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|AI", meta = (EditCondition = "bOverrideAttackProfile"))
	FEnemyAIAttackProfile AttackProfile;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|Attributes")
	FEnemyWeaponAttributeModifiers AttributeModifiers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|Effects")
	TArray<TSubclassOf<UGameplayEffect>> PassiveEffects;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|Visual")
	TArray<FWeaponSpawnData> ActorsToSpawn;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Enemy Weapon|Animation")
	TArray<TSubclassOf<UAnimInstance>> AnimLayers;
};
