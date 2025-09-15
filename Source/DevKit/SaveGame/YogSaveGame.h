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
		PlayerLocation = FVector::ZeroVector;
		PlayerRotation = FRotator::ZeroRotator;
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
	FVector PlayerLocation;

	/* Orientation if player was alive during save */
	UPROPERTY()
	FRotator PlayerRotation;

	/* We don't always want to restore location, and may just resume player at specific respawn point in world. */
	UPROPERTY()
	bool bResumeAtTransform;

	UPROPERTY()
	TArray<uint8> ByteData;

};

USTRUCT()
struct FYogPlayerData
{
	GENERATED_BODY()

public:

	FYogPlayerData()
	{

	}

	/* Location if player was alive during save */
	UPROPERTY()
	FVector PlayerLocation;

	/* Orientation if player was alive during save */
	UPROPERTY()
	FRotator PlayerRotation;

	UPROPERTY()
	TArray<uint8> CharacterByteData;

};

USTRUCT()
struct FYogMapData
{
	GENERATED_BODY()

public:

	FYogMapData()
	{

	}

	/* Location if player was alive during save */
	UPROPERTY()
	FName LevelName;

	UPROPERTY()
	TArray<uint8> MapByteData;

};

/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY()
	FYogPlayerData YogSavePlayers;

	UPROPERTY()
	FYogMapData YogSaveMap;

	UPROPERTY()
	TArray<FPlayerSaveData> SavedPlayers;


	UPROPERTY()
	TArray<uint8> PlayerCharacter;




	UPROPERTY()
	FName LevelName;
	/* Actors stored from a level (currently does not support a specific level and just assumes the demo map) */
	UPROPERTY()
	TMap<FName, FActorSaveData> SavedActorMap;

	FPlayerSaveData* GetPlayerData(APlayerState* PlayerState);

};
