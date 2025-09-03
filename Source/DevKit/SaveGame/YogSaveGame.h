// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "GameFramework/SaveGame.h"
#include "../System/YogGameInstanceBase.h"
#include "Player/PlayerCharacterBase.h"
#include "../Item/Weapon/WeaponInstance.h"
#include "YogSaveGame.generated.h"


USTRUCT()
struct FYogActorSaveData
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


class AYogCharacterBase;
class UYogGameInstanceBase;


class AYogCharacterBase;

UCLASS()
class UCharacterSaveData : public UObject
{
	GENERATED_BODY()

public:
	UCharacterSaveData() {};


	/* Identifier for which Actor this belongs to */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<APlayerCharacterBase> Player;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TArray<AWeaponInstance*> array_WeaponAttach;

};




/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UYogSaveGame();
	///////////////////////////////////////// Basic /////////////////////////////////////////


	//The reason why it set as TArray<uint8> is because the data will be serialize to uint8?
	//reference: https://medium.com/@chrhaase_71293/an-unreal-engine-saving-loading-system-part-1-of-2-62244e55e4b2

	UPROPERTY()
	FString SlotName;

	UPROPERTY()
	FString LevelName;

	UPROPERTY()
	FTransform PlayerTransform;

	UPROPERTY()
	TArray<uint8> PlayerCharacter;

	UPROPERTY()
	TArray<uint8> PlayerController;

	//UPROPERTY()
	//TArray<FObjectRecord> WorldObjects;

	//UPROPERTY()
	//TArray<FString> UniqueCollectiblesCollected;

	//UPROPERTY(VisibleAnywhere)
	//uint32 UserIndex;

	///////////////////////////////////////// Player Attribute /////////////////////////////////////////
	UPROPERTY()
	float Attack = 0;
	UPROPERTY()
	float AttackPower = 0;
	UPROPERTY()
	float Health = 0;
	UPROPERTY()
	float MaxHealth = 0;
	UPROPERTY()
	float Shield = 0;
	UPROPERTY()
	float AttackSpeed = 0;
	UPROPERTY()
	float AttackRange = 0;
	UPROPERTY()
	float Sanity = 0;
	UPROPERTY()
	float MoveSpeed = 0;
	UPROPERTY()
	float Dodge = 0;
	UPROPERTY()
	float Resilience = 0;
	UPROPERTY()
	float Resist = 0;
	UPROPERTY()
	float DmgTaken = 0;
	UPROPERTY()
	float Crit_Rate = 0;
	UPROPERTY()
	float Crit_Damage = 0;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	TObjectPtr<UCharacterSaveData> CharacterSaveData;

	void Initialize(FString InSlotName);


};
