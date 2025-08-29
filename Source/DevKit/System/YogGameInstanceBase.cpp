// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogGameInstanceBase.h"
#include "../Character/PlayerCharacterBase.h"
#include "../System/YogSaveGame.h"

#include "Kismet/GameplayStatics.h"

UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{}

APlayerCharacterBase* UYogGameInstanceBase::GetPlayerCharacter()
{
	APlayerCharacterBase* YogCharacterBase = NewObject<APlayerCharacterBase>();
	UWorld* World = this->GetWorld();
	if (World)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			return Cast<APlayerCharacterBase>(PlayerController->GetPawn());
		}
	}


	return YogCharacterBase;
}

void UYogGameInstanceBase::AddDefaultInventory(UYogSaveGame* SaveGame, bool bRemoveExtra)
{

}

//bool UYogGameInstanceBase::IsValidItemSlot(FRPGItemSlot ItemSlot) const
//{
//
//	return false;
//}

void UYogGameInstanceBase::Init()
{
	Super::Init();
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

void UYogGameInstanceBase::ResetSaveGame()
{

}

void UYogGameInstanceBase::SavePersistentData()
{
	if (PersistentSaveData)
	{

		UPROPERTY(VisibleAnywhere, Category = Basic)
		PersistentSaveData->CharacterSaveData->Player = GetPlayerCharacter();


		UGameplayStatics::SaveGameToSlot(PersistentSaveData, TEXT("PersistentSave"), 0);
	}
	else
	{
		UE_LOG(DevKitCriticalErrors, Display, TEXT("not found PersistentSaveData"));
	}
}

void UYogGameInstanceBase::LoadPersistentData()
{
	if (UGameplayStatics::DoesSaveGameExist(TEXT("PersistentSave"), 0))
	{
		PersistentSaveData = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(TEXT("PersistentSave"), 0));
	}
	else
	{
		PersistentSaveData = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
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
