// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../DevKit.h"

#include "GameFramework/SaveGame.h"
#include "YogSaveGame.generated.h"



class AYogCharacterBase;
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
	UPROPERTY()
	TObjectPtr<AYogCharacterBase> CharacterBase;

	UPROPERTY()
	TArray<AActor*> AttachActorArray;

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


	UPROPERTY(VisibleAnywhere, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	uint32 UserIndex;

	UPROPERTY(VisibleAnywhere, Category = Basic)
	TObjectPtr<UCharacterSaveData> CharacterSaveData;

protected:

	/** Overridden to allow version fixups */
	virtual void Serialize(FArchive& Ar) override;


	UFUNCTION(BlueprintCallable)
	void SaveCharacterData(AYogCharacterBase* Character, UYogSaveGame* SaveGameInstance);

	UFUNCTION(BlueprintCallable)
	void LoadCharacterData();
};
