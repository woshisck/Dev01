// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogSaveGame.h"
#include "YogGameInstanceBase.h"

#include "../Character/YogCharacterBase.h"


UYogSaveGame::UYogSaveGame()
{
	SaveSlotName = TEXT("TestSaveSlot");
	UserIndex = 0;
}

void UYogSaveGame::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	//if (Ar.IsLoading())
	//{
		// if (SavedDataVersion < EYogSaveGameVersion::AddedItemData)
		// {
		// 	// Convert from list to item data map
		// 	for (const FPrimaryAssetId& ItemId : InventoryItems_DEPRECATED)
		// 	{
		// 		InventoryData.Add(ItemId, FRPGItemData(1, 1));
		// 	}

		// 	InventoryItems_DEPRECATED.Empty();
		// }
		

	//
}

void UYogSaveGame::LoadCharacterData()
{
	if (UYogSaveGame* LoadedGame = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("AutoSave0"), 0)))
	{
		// The operation was successful, so LoadedGame now contains the data we saved earlier.
		//UE_LOG(LogTemp, Warning, TEXT("LOADED: %s"), *LoadedGame->PlayerName);
	}
}

UYogSaveGame* UYogSaveGame::CreateNewSaveData(const FString& slot_name, uint32 user_index)
{
	UYogSaveGame* NewSaveData = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	if (NewSaveData)
	{
		NewSaveData->SaveSlotName = slot_name;
		NewSaveData->UserIndex = user_index;
	}
	return NewSaveData;
}

