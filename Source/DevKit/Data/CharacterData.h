// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterData.generated.h"


USTRUCT(BlueprintType)
struct FCharacterData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCharacterData(){};

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MiscNum = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SkillCD = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MAX_PassiveGA = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MAX_OffensiveGA = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OutRoundLifeTime = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed = 6;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Dash = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DashCD = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DashDist = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Dodge = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Resilience = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Resist = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Shield = 0;




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
	

	//Get the item price info
	UFUNCTION(BlueprintPure)
	const FMovementData& GetMovementData() const;

	//Get the item price info
	UFUNCTION(BlueprintPure)
	const FCharacterData& GetCharacterData() const;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "MovementData"))
	FDataTableRowHandle MoveDataRow;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "CharacterData"))
	FDataTableRowHandle CharacterDataRow;

	inline static const FMovementData DefaultMovementData;

	inline static const FCharacterData DefaultCharacterData;
};
