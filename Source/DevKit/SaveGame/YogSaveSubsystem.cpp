// Fill out your copyright notice in the Description page of Project Settings.


#include "YogSaveSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

void UYogSaveSubsystem::SerializeObject(UObject* Object, TArray<uint8>& OutResult)
{
	if (!Object)
	{
		return;
	}

	FMemoryWriter MemoryWriter = FMemoryWriter(OutResult, true);
	FYogSaveGameArchive Archive = FYogSaveGameArchive(MemoryWriter);

	Object->Serialize(Archive);
}

void UYogSaveSubsystem::DeserializeObject(const TArray<uint8>& Data, UObject* Object)
{
	if (!Object)
	{
		return;
	}

	FMemoryReader MemoryReader(Data, true);
	FYogSaveGameArchive Archive(MemoryReader);

	Object->Serialize(Archive);
}

void UYogSaveSubsystem::CreateNewSaveGame(FString SlotName)
{
	// Create a new save game instance.
	CurrentSaveGame = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	if (!CurrentSaveGame)
	{
		return;
	}

	// Initialize the new instance.
	CurrentSaveGame->Initialize(SlotName);
	SaveCurrentSlot();
}

void UYogSaveSubsystem::SaveCurrentSlot()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	PopulateCurrentSlot();

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSaveGame->SlotName, DefaultUserIndex);

}

void UYogSaveSubsystem::PopulateCurrentSlot()
{

}

void UYogSaveSubsystem::PopulateFromCurrentSlot()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	AGameStateBase* GS = UGameplayStatics::GetGameState(GetWorld());
	if (GS)
	{
		SerializeObject(GS, CurrentSaveGame->GameState);
	}

	APlayerState* PS = UGameplayStatics::GetPlayerState(GetWorld(), 0);
	if (PS)
	{
		SerializeObject(PS, CurrentSaveGame->PlayerState);
	}

	// Player controller.
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		SerializeObject(Cast<UObject>(PC), CurrentSaveGame->PlayerController);
		CurrentSaveGame->ControlRotation = PC->GetControlRotation();
	}
}

//serialize/de-serialize example:
//
//TArray<unit8> Data; // Imagine this actually has data...
//AGameStateBase* GS = UGameplayStatics::GetGameState(GetWorld());
//DeserializeObject(Data, GS);