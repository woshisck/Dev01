// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "YogCharacterBase.h"
#include "PlayerCharacterBase.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct FCharacterMovementData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FCharacterMovementData()
		: MaxWalkSpeed(600.0f), GroundFriction(8.0f), BreakingDeceleration(2048.0f), MaxAcceleration(2048.0f), RotationRate(FRotator(0, 0, 360))
	{
	}
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float GroundFriction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BreakingDeceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRotator RotationRate;
};


UENUM()
enum class EPlayerBattleState : uint8
{
	OnGetHit		UMETA(DisplayName = "OnGetHit"),
	OnHitFrame		UMETA(DisplayName = "OnHitFrame")
};

UCLASS()
class DEVKIT_API APlayerCharacterBase : public AYogCharacterBase
{
	GENERATED_BODY()
	
public:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;


	APlayerCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character|Movement")
	TObjectPtr<UDataTable> CharacterMovementDataTable;
};
