// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogSaveGame.h"
#include "System/YogGameInstanceBase.h"

#include "Character/YogCharacterBase.h"


UYogSaveGame::UYogSaveGame()
{
}


UWorld* UYogSaveGame::LoadSavedLevel() const
{
    // If soft ptr is not loaded, try to load from saved path
    if (!SavedLevelPath.IsEmpty())
    {
        TSoftObjectPtr<UWorld> LevelPtr = TSoftObjectPtr<UWorld>(FSoftObjectPath(SavedLevelPath));
        return LevelPtr.LoadSynchronous();
    }

    return nullptr;

}

void UYogSaveGame::Initialize(FString InSlotName)
{
	SlotName = InSlotName;

	// Reset all data in case this slot has already be used before.
	//GameState.Empty();
	//PlayerState.Empty();
	//PlayerCharacter.Empty();
	//PlayerController.Empty();
	//WorldObjects.Empty();

	FTransform NewPlayerTransform;
	PlayerTransform = NewPlayerTransform;
}

//void UYogSaveGame::LoadCharacterData()
//{
//	if (UYogSaveGame* LoadedGame = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("AutoSave0"), 0)))
//	{
//		// The operation was successful, so LoadedGame now contains the data we saved earlier.
//		//UE_LOG(LogTemp, Warning, TEXT("LOADED: %s"), *LoadedGame->PlayerName);
//	}
//}


