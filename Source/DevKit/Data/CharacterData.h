// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "CharacterData.generated.h"


USTRUCT(BlueprintType)
struct FYogBaseData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FYogBaseData(){};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100;

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

	UFUNCTION(BlueprintCallable)
	const FMovementData& GetMovementData() const;

	UFUNCTION(BlueprintCallable)
	const FYogBaseData& GetCharacterData() const;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "MovementData"))
	FDataTableRowHandle MoveDataRow;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "YogBaseData"))
	FDataTableRowHandle YogBaseDataRow;


	inline static const FMovementData DefaultMovementData;

	inline static const FYogBaseData DefaultCharacterData;
};
