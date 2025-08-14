// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterData.generated.h"


USTRUCT(BlueprintType)
struct FYogAttributeData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FYogAttributeData()
		: Health(1234.f), MaxHealth(1234.f), BaseDMG(1234.f), WeaponDMG(1234.f), BuffAmplify(1234.f), DMGAbsorb(1234.f), ActResist(1234.f)
	{
	}
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseDMG; 

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float WeaponDMG;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BuffAmplify;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DMGAbsorb;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ActResist;

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
	const FMovementData& GetMovement() const;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "MovementData"))
	FDataTableRowHandle MoveDataRow;

	inline static const FMovementData DefaultMovement;
};
