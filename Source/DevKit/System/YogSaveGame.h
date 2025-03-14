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

/** Object that is written to and read from the save game archive, with a data version */
UCLASS(BlueprintType)
class DEVKIT_API UYogSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** Constructor */
	UYogSaveGame()
	{
		// Set to current version, this will get overwritten during serialization when loading
		SavedDataVersion = EYogSaveGameVersion::LatestVersion;
	}

	/** Map of items to item data */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FPrimaryAssetId, FRPGItemData> InventoryData;

	///** Map of slotted items */
	//UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	//TMap<FRPGItemSlot, FPrimaryAssetId> SlottedItems;

	/** User's unique id */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = SaveGame)
	FString UserId;

protected:
	/** Deprecated way of storing items, this is read in but not saved out */
	UPROPERTY()
	TArray<FPrimaryAssetId> InventoryItems_DEPRECATED;

	/** What LatestVersion was when the archive was saved */
	UPROPERTY()
	int32 SavedDataVersion;

	/** Overridden to allow version fixups */
	virtual void Serialize(FArchive& Ar) override;

	void SaveCharacterState(AYogCharacterBase* Character, UYogSaveGame* SaveGameInstance);
};
