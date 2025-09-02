// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"

#include "GameFramework/SaveGame.h"
#include <DevKit/Character/PlayerCharacterBase.h>
#include "../System/YogGameInstanceBase.h"
#include "../Character/PlayerCharacterBase.h"
#include "../Item/Weapon/WeaponInstance.h"
#include "YogSaveGame.generated.h"



class AYogCharacterBase;
class UYogGameInstanceBase;
/** List of versions, native code will handle fixups for any old versions */
namespace EYogSaveGameVersion
{
	enum type
	{
		// Initial version
		Initial,
		// Added Inventory
		AddedStatsData,
		// Added ItemData to store count/level
		AddedAbilityData,

		// -----<new versions must be added before this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
}

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

//TODO: SAMPLE FROM https://medium.com/@chrhaase_71293/an-unreal-engine-saving-loading-system-part-1-of-2-62244e55e4b2




/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Constructor */
	UYogSaveGame();
	//{
	//	// Set to current version, this will get overwritten during serialization when loading
	//	SavedDataVersion = EYogSaveGameVersion::LatestVersion;
	//}

	//The reason why it set as TArray<uint8> is because the data will be serialize to uint8?
	//reference: https://medium.com/@chrhaase_71293/an-unreal-engine-saving-loading-system-part-1-of-2-62244e55e4b2
	UPROPERTY()
	FString SlotName;

	UPROPERTY()
	TArray<uint8> GameState;

	UPROPERTY()
	TArray<uint8> PlayerState;

	UPROPERTY()
	FTransform PlayerTransform;

	UPROPERTY()
	TArray<uint8> PlayerCharacter;


	UPROPERTY()
	TArray<uint8> PlayerController;

	UPROPERTY()
	FRotator ControlRotation;

	UPROPERTY()
	TArray<FObjectRecord> WorldObjects;

	UPROPERTY()
	TArray<FString> UniqueCollectiblesCollected;



	UPROPERTY(VisibleAnywhere)
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	TObjectPtr<UCharacterSaveData> CharacterSaveData;

	void Initialize(FString InSlotName);


protected:


	/** Overridden to allow version fixups */
	virtual void Serialize(FArchive& Ar) override;


	UFUNCTION(BlueprintCallable)
	void SaveCharacterData(APlayerCharacterBase* Character, UYogSaveGame* SaveGameInstance)
	{


		if (Character && SaveGameInstance)
		{


			UYogGameInstanceBase* CurrentGameInstance = Cast<UYogGameInstanceBase>(GetWorld()->GetGameInstance<UGameInstance>());

			APlayerCharacterBase* CurrentCharacter = CurrentGameInstance->GetPlayerCharacter();
			SaveGameInstance->CharacterSaveData->Player = CurrentGameInstance->GetPlayerCharacter();





			TArray<AActor*> AttachedActors;
			CurrentCharacter->GetAttachedActors(AttachedActors);
			for (AActor* actor : AttachedActors)
			{
				AWeaponInstance* weapon = Cast<AWeaponInstance>(actor);
				if (weapon)
				{
					SaveGameInstance->CharacterSaveData->array_WeaponAttach.Add(Cast<AWeaponInstance>(actor));
				}
			}
			//SaveGameInstance->CharacterSaveData->array_WeaponAttach = AttachedActors;
		}



		UGameplayStatics::SaveGameToSlot(SaveGameInstance, TEXT("AutoSave0"), 0);
	}

	UFUNCTION(BlueprintCallable)
	void LoadCharacterData();


	// Create new save data
	UYogSaveGame* CreateNewSaveData(const FString& slot_name, uint32 user_index);

};
