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
		: FocusSpeed(0.5f), FollowSpeed(0.5f), DistFromCharacter(100.f)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FocusSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FollowSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DistFromCharacter;

};



UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UPlayerData : public UCharacterData
{
	GENERATED_BODY()
	
public:
	UPlayerData();


};
