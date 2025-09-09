// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "GameFramework/SaveGame.h"
#include "../System/YogGameInstanceBase.h"
#include "Player/PlayerCharacterBase.h"
#include "../Item/Weapon/WeaponInstance.h"
#include "YogSaveGame.generated.h"


class AYogCharacterBase;
class UYogGameInstanceBase;
class UYogGameplayEffect;

class AYogCharacterBase;


USTRUCT()
struct FActorSaveData
{
	GENERATED_BODY()

public:
	/* Identifier for which Actor this belongs to */
	UPROPERTY()
	FName ActorName;

	/* For movable Actors, keep location,rotation,scale. */
	UPROPERTY()
	FTransform Transform;

	/* Contains all 'SaveGame' marked variables of the Actor */
	UPROPERTY()
	TArray<uint8> ByteData;

};

USTRUCT()
struct FPlayerSaveData
{
	GENERATED_BODY()

public:

	FPlayerSaveData()
	{
		Credits = 0;
		PersonalRecordTime = 0.0f;
		Location = FVector::ZeroVector;
		Rotation = FRotator::ZeroRotator;
		bResumeAtTransform = true;
	}

	/* Player Id defined by the online sub system (such as Steam) converted to FString for simplicity  */
	UPROPERTY()
	FString PlayerID;

	UPROPERTY()
	int32 Credits;

	/* Longest survival time */
	UPROPERTY()
	float PersonalRecordTime;

	/* Location if player was alive during save */
	UPROPERTY()
	FVector Location;

	/* Orientation if player was alive during save */
	UPROPERTY()
	FRotator Rotation;

	/* We don't always want to restore location, and may just resume player at specific respawn point in world. */
	UPROPERTY()
	bool bResumeAtTransform;

	UPROPERTY()
	TArray<uint8> ByteData;

};


/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY()
	TArray<FPlayerSaveData> SavedPlayers;
	
	UPROPERTY()
	FName LevelName;
	/* Actors stored from a level (currently does not support a specific level and just assumes the demo map) */
	UPROPERTY()
	TMap<FName, FActorSaveData> SavedActorMap;

	FPlayerSaveData* GetPlayerData(APlayerState* PlayerState);

};
