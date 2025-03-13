// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

// ----------------------------------------------------------------------------------------------------------------
// This header is for enums and structs used by classes and blueprints accross the game
// Collecting these in a single header helps avoid problems with recursive header includes
// It's also a good place to put things like data table row structs
// ----------------------------------------------------------------------------------------------------------------
#include "CoreMinimal.h"
#include "UObject/PrimaryAssetId.h"
#include "YogTypes.generated.h"

class YogSaveGame;

USTRUCT(BlueprintType)
struct DEVKIT_API FYogPickupSlot
{
	GENERATED_BODY()

	FYogPickupSlot() {

	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Item)
	FPrimaryAssetType ItemType;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FYogPickupData
{
	GENERATED_BODY()

	FYogPickupData()
	{ }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Item)
	int32 ItemLevel;
};



/** Delegate called when the save game has been loaded/reset */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveGameLoaded, UYogSaveGame*, SaveGame);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnSaveGameLoadedNative, UYogSaveGame*);
