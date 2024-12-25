// Copyright Epic Games, Inc. All Rights Reserved.

#include "YogBaseGameInstance.h"
#include "Kismet/GameplayStatics.h"

UYogBaseGameInstance::UYogBaseGameInstance()
	: SaveSlot(TEXT("SaveGame"))
	, SaveUserIndex(0)
{}

void UYogBaseGameInstance::AddDefaultInventory(UYogSaveGame* SaveGame, bool bRemoveExtra)
{

}

//bool UYogBaseGameInstance::IsValidItemSlot(FRPGItemSlot ItemSlot) const
//{
//
//	return false;
//}

//UYogSaveGame* UYogBaseGameInstance::GetCurrentSaveGame()
//{
//	return CurrentSaveGame;
//}

void UYogBaseGameInstance::SetSavingEnabled(bool bEnabled)
{
	bSavingEnabled = bEnabled;
}

bool UYogBaseGameInstance::LoadOrCreateSaveGame()
{
	return false;
}

bool UYogBaseGameInstance::HandleSaveGameLoaded(USaveGame* SaveGameObject)
{
	bool bLoaded = false;
	return bLoaded;
}

void UYogBaseGameInstance::GetSaveSlotInfo(FString& SlotName, int32& UserIndex) const
{

}

bool UYogBaseGameInstance::WriteSaveGame()
{

	return false;
}

void UYogBaseGameInstance::ResetSaveGame()
{

}

void UYogBaseGameInstance::HandleAsyncSave(const FString& SlotName, const int32 UserIndex, bool bSuccess)
{

}
