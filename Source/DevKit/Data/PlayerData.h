// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CharacterData.h"
#include "PlayerData.generated.h"

/**
 * 
 */



USTRUCT(BlueprintType)
struct FCameraMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCameraMovementData()
		: MaxWalkSpeed(0.5f), GroundFriction(0.5f), BreakingDeceleration(100.f), MaxAcceleration(0.1f)
	{
	}
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
class DEVKIT_API UPlayerData : public UCharacterData
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintPure)
	const FCameraMovementData& GetCameraMove() const;

	UPROPERTY(EditDefaultsOnly, meta = (RowType = "MovementData"))
	FDataTableRowHandle CameraDataRow;

	inline static const FCameraMovementData DefaultMovement;

};
