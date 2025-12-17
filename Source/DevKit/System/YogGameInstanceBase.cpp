// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogGameInstanceBase.h"
#include "Player/PlayerCharacterBase.h"
#include "DevKit/SaveGame/YogSaveGame.h"

#include "Kismet/GameplayStatics.h"

UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{}

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
}

void UYogGameInstanceBase::SaveGame()
{
	if (!PersistentSaveData)
	{
		PersistentSaveData = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	}
	APlayerCharacterBase* CurrentCharacter = GetPlayerCharacter();

	PersistentSaveData->SavedLocation = CurrentCharacter->GetActorLocation();
	PersistentSaveData->SavedRotation = CurrentCharacter->GetActorRotation();
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
