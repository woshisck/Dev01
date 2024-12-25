// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogSaveGame.h"
#include "YogBaseGameInstance.h"

void UYogSaveGame::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && SavedDataVersion != EYogSaveGameVersion::LatestVersion)
	{
		// if (SavedDataVersion < EYogSaveGameVersion::AddedItemData)
		// {
		// 	// Convert from list to item data map
		// 	for (const FPrimaryAssetId& ItemId : InventoryItems_DEPRECATED)
		// 	{
		// 		InventoryData.Add(ItemId, FRPGItemData(1, 1));
		// 	}

		// 	InventoryItems_DEPRECATED.Empty();
		// }
		
		SavedDataVersion = EYogSaveGameVersion::LatestVersion;
	}
}