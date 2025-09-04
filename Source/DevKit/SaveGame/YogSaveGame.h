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
struct FObjectRecord
{
	GENERATED_BODY()

public:
	FObjectRecord()
	{
		Class = nullptr;
		Outer = nullptr;
		OuterId = 0;
		Self = nullptr;
		bIsStaticWorldActor = false;
	}

	UPROPERTY()
	UClass* Class;

	UPROPERTY()
	UObject* Outer;

	UPROPERTY()
	int32 OuterId;

	UPROPERTY()
	UObject* Self;

	UPROPERTY()
	FString Name;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	bool bIsStaticWorldActor;

	UPROPERTY()
	TArray<uint8> Data;
};



/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UYogSaveGame();


	UPROPERTY()
	FString SlotName;

	UPROPERTY()
	FString LevelName;

	UPROPERTY()
	FTransform PlayerTransform;

	UPROPERTY()
	TArray<uint8> PlayerCharacter;


public:
	void Initialize(FString InSlotName);

};
