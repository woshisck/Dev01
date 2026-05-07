// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Data/AbilityData.h"
#include "Data/GasTemplate.h"
#include "CharacterData.generated.h"


USTRUCT(BlueprintType)
struct FYogBaseAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FYogBaseAttributeData(){};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHeat = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Shield = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackSpeed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackRange = 150;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Sanity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed = 600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Dodge = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Resilience = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Resist = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DmgTaken = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Crit_Rate = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Crit_Damage = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxArmorHP = 0;
};

USTRUCT(BlueprintType)
struct FMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FString Name;
	FMovementData() {};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundFriction = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BreakingDeceleration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAcceleration = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator RotationRate = FRotator(0, 0, 0);
};

UCLASS()
class DEVKIT_API UCharacterData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:



	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (RowType = "MovementData"))
	FDataTableRowHandle MovementDataRow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (RowType = "YogBaseAttributeData"))
	FDataTableRowHandle YogBaseAttributeDataRow;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UAbilityData> AbilityData;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Data")
	TObjectPtr<UGASTemplate> GasTemplate;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AnimeLayer")
	TArray<TSubclassOf<UAnimInstance>> DefaultAnimeLayers;


	const FMovementData* GetMovementData() const;

	const FYogBaseAttributeData* GetBaseAttributeData() const;

	const UAbilityData* GetAbilityData() const;

	const UGASTemplate* GetGASTemplate() const;

	const TArray<TSubclassOf<UAnimInstance>> GetDefaultAnimeLayers() const;

	// ── 细粒度访问器（业务代码请使用，避免重复 GetBaseAttributeData() 后再读字段）──
	// 数值缺失时返回 FYogBaseAttributeData 默认值。

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetMaxHeat() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetShield() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetAttack() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetAttackPower() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetAttackSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetAttackRange() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetMoveSpeed() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetCritRate() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetCritDamage() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetMaxArmorHP() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetDodge() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetResilience() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetResist() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetDmgTaken() const;

	UFUNCTION(BlueprintPure, Category = "Character|Accessor")
	float GetSanity() const;

	inline static const FMovementData DefaultMovementData;

	inline static const FYogBaseAttributeData DefaultCharacterData;
};
