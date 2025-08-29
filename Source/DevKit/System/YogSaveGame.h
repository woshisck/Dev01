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


	UPROPERTY(VisibleAnywhere, Category = "SaveGame")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere)
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	TObjectPtr<UCharacterSaveData> CharacterSaveData;


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
