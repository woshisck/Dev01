// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"
#include "GameFramework/SaveGame.h"
#include "../System/YogGameInstanceBase.h"
#include "Player/PlayerCharacterBase.h"
#include "../Item/Weapon/WeaponInstance.h"
#include "DevKit/Animation/YogAnimInstance.h"
#include "DevKit/AbilitySystem/Attribute/BaseAttributeSet.h"
#include "DevKit/Item/Weapon/WeaponInstance.h"
#include "YogSaveGame.generated.h"


class AYogCharacterBase;
class UYogGameInstanceBase;
class UYogGameplayEffect;

class AYogCharacterBase;

USTRUCT()
struct DEVKIT_API FGameAbilitySystemSaveData
{
	GENERATED_BODY()


};



USTRUCT()
struct DEVKIT_API FAttributeSaveData
{
	GENERATED_BODY()

public:

	FAttributeSaveData()
	{

		Attack = 0.0;
		AttackPower = 0.0;
		Health = 0.0;
		MaxHealth = 0.0;
		AttackSpeed = 0.0;
		AttackRange = 0.0;
		Sanity = 0.0;
		MoveSpeed = 0.0;
		Dodge = 0.0;
		Resilience = 0.0;
		Resist = 0.0;
		DmgTaken = 0.0;
		Crit_Rate = 0.0;
		Crit_Damage = 0.0;
	}

	UPROPERTY()
	float Attack;

	UPROPERTY()
	float AttackPower;

	UPROPERTY()
	float Health;

	UPROPERTY()
	float MaxHealth;

	UPROPERTY()
	float AttackSpeed;

	UPROPERTY()
	float AttackRange;

	UPROPERTY()
	float Sanity;

	UPROPERTY()
	float MoveSpeed;

	UPROPERTY()
	float Dodge;

	UPROPERTY()
	float Resilience;

	UPROPERTY()
	float Resist;

	UPROPERTY()
	float DmgTaken;

	UPROPERTY()
	float Crit_Rate;

	UPROPERTY()
	float Crit_Damage;

};



USTRUCT()
struct DEVKIT_API FYogActorSaveData
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

//TEST ONLY, SAVE FOR DELETE --START

USTRUCT(BlueprintType)
struct FWeaponInstanceData
{

	GENERATED_BODY()

public:
	// Identifier to find the actor in the world or know its class to spawn
	UPROPERTY()
	FString ActorClassPath; 
	
	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	FName AttachSocket;

	/* Contains all 'SaveGame' marked variables of the Actor */
	UPROPERTY()
	TArray<uint8> ByteData;

};

//TEST ONLY, SAVE FOR DELETE --END




USTRUCT(BlueprintType)
struct FWeaponMeshData
{
	GENERATED_BODY()

	// Identifier to find the actor in the world or know its class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<AWeaponInstance>> weaponInstanceClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName AttachSocket;
};

USTRUCT(BlueprintType)
struct FWeaponSaveData
{
	GENERATED_BODY()

	// Identifier to find the actor in the world or know its class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FWeaponMeshData WeaponMeshData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UYogAnimInstance> WeaponLayer;

};


USTRUCT()
struct DEVKIT_API FCharacterSaveData
{
	GENERATED_BODY()

public:

	FCharacterSaveData()
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
	FAttributeSaveData AttributeSaveData;



	UPROPERTY()
	TArray<uint8> ByteData;

};

USTRUCT(BlueprintType)
struct DEVKIT_API FAbilitySaveData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Level = 0;

	UPROPERTY()
	int32 InputID = -1;

	UPROPERTY()
	FSoftClassPath AbilityClassPath;

	UPROPERTY()
	TSubclassOf<UYogGameplayAbility> AbilityClass;
};


USTRUCT()
struct DEVKIT_API FPlayerGameTagData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 count = 0;

	UPROPERTY()
	FGameplayTag gameplayTag;
};

USTRUCT()
struct DEVKIT_API FPlayerGASData
{
	GENERATED_BODY()
public:

	UPROPERTY()
	FAttributeSaveData PlayerAttributeData;

	UPROPERTY()
	TArray<FPlayerGameTagData> PlayerTags;

	//UPROPERTY()
	//FAbilitySystemComponentData ASC_Data;

	UPROPERTY()
	TArray<FAbilitySaveData> Abilities;

	UPROPERTY()
	TArray<uint8> CharacterByteData;

	void SetupAttribute(UBaseAttributeSet& playerAttribute)
	{
		PlayerAttributeData.Attack = playerAttribute.GetAttack();
		PlayerAttributeData.AttackPower = playerAttribute.GetAttackPower();
		PlayerAttributeData.Health = playerAttribute.GetHealth();
		PlayerAttributeData.MaxHealth = playerAttribute.GetMaxHealth();
		PlayerAttributeData.AttackSpeed = playerAttribute.GetAttackSpeed();
		PlayerAttributeData.AttackRange = playerAttribute.GetAttackRange();
		PlayerAttributeData.Sanity = playerAttribute.GetSanity();
		PlayerAttributeData.MoveSpeed = playerAttribute.GetMoveSpeed();
		PlayerAttributeData.Dodge = playerAttribute.GetDodge();
		PlayerAttributeData.Resilience = playerAttribute.GetResilience();
		PlayerAttributeData.Resist = playerAttribute.GetResist();
		PlayerAttributeData.DmgTaken = playerAttribute.GetDmgTaken();
		PlayerAttributeData.Crit_Rate = playerAttribute.GetCrit_Rate();
		PlayerAttributeData.Crit_Damage = playerAttribute.GetCrit_Damage();

	}
};

USTRUCT()
struct FYogMapStateData
{
	GENERATED_BODY()

public:

	/* Location if player was alive during save */
	UPROPERTY()
	FName LevelName;

	UPROPERTY()
	TArray<uint8> MapByteData;

};


UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<class APlayerCharacterBase> SavedCharacterClass;

	UPROPERTY()
	FWeaponSaveData WeaponData;

	UPROPERTY()
	FPlayerGASData PlayerStateData;

	UPROPERTY()
	TArray<FWeaponInstanceData> WeaponInstanceItems;


	UPROPERTY()
	FYogMapStateData MapStateData;


	//ACTOR WITH GAS SYSTEM ATTACHED
	UPROPERTY()
	TArray<FCharacterSaveData> SavedCharacter;


	UPROPERTY()
	TArray<FYogActorSaveData> SavedActors;


	UPROPERTY()
	FName LevelName;
	/* Actors stored from a level (currently does not support a specific level and just assumes the demo map) */
	UPROPERTY()
	TMap<FName, FYogActorSaveData> SavedActorMap;

	FCharacterSaveData* GetPlayerData(APlayerState* PlayerState);

};
