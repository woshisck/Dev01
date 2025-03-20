// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogGameInstanceBase.h"
#include "../Character/YogCharacterBase.h"


#include "Kismet/GameplayStatics.h"

UYogGameInstanceBase::UYogGameInstanceBase()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{}

AYogCharacterBase* UYogGameInstanceBase::GetPlayerCharacter()
{
	AYogCharacterBase* YogCharacterBase = NewObject<AYogCharacterBase>();
	UWorld* World = this->GetWorld();
	if (World)
	{
		APlayerController* PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PlayerController)
		{
			return Cast<AYogCharacterBase>(PlayerController->GetPawn());
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

		UGameplayStatics::AsyncSaveGameToSlot(GetCurrentSaveGame(), SaveSlot, SaveUserIndex, FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UYogGameInstanceBase::HandleAsyncSave));
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
