// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PlayerData.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FPlayerStatData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FPlayerStatData(){}


	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Attack = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AtkPower = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SkillCD = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MoveSpeed = 6;

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

};



USTRUCT(BlueprintType)
struct FCameraMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCameraMovementData(){}
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


UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UPlayerData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure)
	const FCameraMovementData& GetCameraMove() const;

	UFUNCTION(BlueprintPure)
	const FPlayerStatData& GetPlayerState() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "CameraMovementData"))
	FDataTableRowHandle CameraDataRow;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RowType = "PlayerStatData"))
	FDataTableRowHandle PlayerDataRow;


	inline static const FCameraMovementData DefaultMovement;
	inline static const FPlayerStatData DefaultPlayerState;

};
