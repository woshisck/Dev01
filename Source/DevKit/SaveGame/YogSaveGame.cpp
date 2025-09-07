// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogSaveGame.h"
#include "System/YogGameInstanceBase.h"

#include "Character/YogCharacterBase.h"


UYogSaveGame::UYogSaveGame()
{


    SlotName = TEXT("Player");;

    SavedLevelPath = TEXT("Player");;

    LevelName = TEXT("Player");;



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




	//PlayerCharacter.Empty();

	//WorldObjects.Empty();

}

//void UYogSaveGame::LoadCharacterData()
//{
//	if (UYogSaveGame* LoadedGame = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("AutoSave0"), 0)))
//	{
//		// The operation was successful, so LoadedGame now contains the data we saved earlier.
//		//UE_LOG(LogTemp, Warning, TEXT("LOADED: %s"), *LoadedGame->PlayerName);
//	}
//}


