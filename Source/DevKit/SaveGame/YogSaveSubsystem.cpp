// Fill out your copyright notice in the Description page of Project Settings.


#include "YogSaveSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

UYogSaveSubsystem::UYogSaveSubsystem()
{
	CurrentSaveGame->Initialize(SlotName);

}

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

void UYogSaveSubsystem::LoadGame()
{
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, DefaultUserIndex_SOLID))
	{
		return;
	}

	CurrentSaveGame = Cast<UYogSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, DefaultUserIndex_SOLID));

	// Player
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Player)
	{
		DeserializeObject(CurrentSaveGame->PlayerCharacter, Player);
	}



}

void UYogSaveSubsystem::SaveGame()
{
	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSaveGame->SlotName, DefaultUserIndex_SOLID);
}

void UYogSaveSubsystem::CreateNewSaveGame()
{

	// Create a new save game instance.
	CurrentSaveGame = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	if (!CurrentSaveGame)
	{
		return;
	}

	// Initialize the new instance.
	CurrentSaveGame->Initialize(SlotName);

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSaveGame->SlotName, DefaultUserIndex_SOLID);
	//SaveCurrentSlot();
}

void UYogSaveSubsystem::SaveCurrentSlot()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	PopulateCurrentSlot();

	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, CurrentSaveGame->SlotName, DefaultUserIndex_SOLID);

}

void UYogSaveSubsystem::PopulateCurrentSlot()
{
	// Player
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (Player)
	{
		SerializeObject(Player, CurrentSaveGame->PlayerCharacter);
	}

}

void UYogSaveSubsystem::PopulateFromCurrentSlot()
{
	if (!CurrentSaveGame)
	{
		return;
	}

	// Player controller.
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		
		//SerializeObject(Cast<UObject>(PC), CurrentSaveGame->PlayerController);
	}


	TArray<AActor*> WorldObjects;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), WorldObjects);

}

//serialize/de-serialize example:
//TArray<unit8> Data;
//AGameStateBase* GS = UGameplayStatics::GetGameState(GetWorld());
//SerializeObject(GS, Data);
// 
// 
//TArray<unit8> Data; // Imagine this actually has data...
//AGameStateBase* GS = UGameplayStatics::GetGameState(GetWorld());
//DeserializeObject(Data, GS);