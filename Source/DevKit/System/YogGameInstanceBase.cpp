// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogGameInstanceBase.h"
#include "Player/PlayerCharacterBase.h"
#include "DevKit/SaveGame/YogSaveGame.h"
#include "DevKit/SaveGame/YogSaveSubsystem.h"
#include "Kismet/GameplayStatics.h"

UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{
	bShouldLoadSaveAfterMap = false;
}

APlayerCharacterBase* UYogGameInstanceBase::GetPlayerCharacter()
{
	APlayerCharacterBase* PlayerCharacterBase = NewObject<APlayerCharacterBase>();
	UWorld* World = this->GetWorld();
	if (World)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			return Cast<APlayerCharacterBase>(PlayerController->GetPawn());
		}
	}

	return PlayerCharacterBase;
}




void UYogGameInstanceBase::Init()
{
	Super::Init();
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UYogGameInstanceBase::OnPostLoadMap);

}

void UYogGameInstanceBase::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Shutdown();
}

void UYogGameInstanceBase::OpenMapAndLoadSave(const TSoftObjectPtr<UWorld> Level)
{
	// Set the pending save slot and flag
	PendingSaveSlot = SaveSlot;
	bShouldLoadSaveAfterMap = true;

	// Bind the delegate to know when the map is loaded

	// Open the map
	//void UGameplayStatics::OpenLevelBySoftObjectPtr(const UObject * WorldContextObject, const TSoftObjectPtr<UWorld> Level, bool bAbsolute, FString Options)

	UGameplayStatics::OpenLevelBySoftObjectPtr(this, Level, true);
}

void UYogGameInstanceBase::OnPostLoadMap(UWorld* World)
{
	
	// If we are supposed to load a save after the map, do it
	if (bShouldLoadSaveAfterMap && World)
	{
		// Load the save game from the pending slot
		// Assuming you have a function to load the save game and apply it to the world
		
		//LoadSaveGame(PendingSaveSlot);
		//UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

		//UGameInstance* GameInstancePtr = Cast<UGameInstance>(GetWorld()->GetGameInstance());


		UYogSaveSubsystem* SaveSubsystem = this->GetSubsystem<UYogSaveSubsystem>();

		if (SaveSubsystem->CurrentSaveGame)
		{
			SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
		}



		// Reset the flag
		bShouldLoadSaveAfterMap = false;
	}

	// Unbind the delegate to avoid multiple bindings
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}

void UYogGameInstanceBase::SaveGame()
{
	if (!PersistentSaveData)
	{
		PersistentSaveData = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}
	APlayerCharacterBase* CurrentCharacter = GetPlayerCharacter();
	PersistentSaveData->SavedCharacterClass = CurrentCharacter->GetClass();

	UGameplayStatics::SaveGameToSlot(PersistentSaveData, "Slot1", 0);
}

UYogSaveGame* UYogGameInstanceBase::GetCurrentSaveGame()
{
	return CurrentSaveGame;
}

void UYogGameInstanceBase::SetSavingEnabled(bool bEnabled)
{
	bSavingEnabled = bEnabled;
}

bool UYogGameInstanceBase::LoadOrCreateSaveGame()
{
	return false;
}

bool UYogGameInstanceBase::HandleSaveGameLoaded(USaveGame* SaveGameObject)
{
	bool bLoaded = false;
	return bLoaded;
}

void UYogGameInstanceBase::GetSaveSlotInfo(FString& SlotName, int32& UserIndex) const
{

}

bool UYogGameInstanceBase::WriteSaveGame()
{
	if (bSavingEnabled)
	{
		if (bCurrentlySaving)
		{
			bPendingSaveRequested = true;
			return true;
		}
		bCurrentlySaving = true;
		USaveGame* currentSaveGame = Cast<USaveGame>(GetCurrentSaveGame());
		UGameplayStatics::AsyncSaveGameToSlot(currentSaveGame, SaveSlot, SaveUserIndex, FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UYogGameInstanceBase::HandleAsyncSave));
		return true;
	}
	else 
	{
		return false;
	}
}





void UYogGameInstanceBase::HandleAsyncSave(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{
	ensure(bCurrentlySaving);
	bCurrentlySaving = false;

	if (bPendingSaveRequested)
	{

		bPendingSaveRequested = false;
		WriteSaveGame();
	}

}
