// Fill out your copyright notice in the Description page of Project Settings.


#include "YogSaveSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"

UYogSaveSubsystem::UYogSaveSubsystem()
{
	CurrentSaveGame->Initialize(SlotName);

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

void UYogSaveSubsystem::UObjectArraySaver(UPARAM(ref) TArray<UObject*>& SaveObjects)
{
	for (UObject* SaveObject : SaveObjects)
	{
		UObjectSaver(SaveObject);
	}
}

void UYogSaveSubsystem::UObjectSaver(UObject* SaveObject)
{
}

void UYogSaveSubsystem::CurrentLevelSaver(UWorld* level)
{
	if (level)
	{
		UYogWorldSubsystem* worldsubsystem = this->GetWorld()->GetSubsystem<UYogWorldSubsystem>();
		UWorld* current_world = worldsubsystem->GetCurrentWorld();
		CurrentSaveGame->CurrentLevel.AssetPath = current_world->GetPathName();
		SaveData(current_world, CurrentSaveGame->CurrentLevel.ByteData);
	}
	else
	{
		return;
	}
}

void UYogSaveSubsystem::SaveData(UObject* Object, TArray<uint8>& Data)
{
	if (Object == nullptr)
	{
		return;
	}

	FMemoryWriter MemoryWriter = FMemoryWriter(Data, true);
	FYogSaveGameArchive Archive = FYogSaveGameArchive(MemoryWriter);

	Object->Serialize(Archive);
}


void UYogSaveSubsystem::LoadData(UObject* Object, UPARAM(ref)TArray<uint8>& Data)
{

	if (!Object)
	{
		return;
	}
	FMemoryReader MemoryReader(Data, true);

	FYogSaveGameArchive Ar(MemoryReader);
	Object->Serialize(Ar);
}


//void UYogSaveSubsystem::PopulateCurrentSlot()
//{
//	// Player
//	UYogWorldSubsystem* worldsubsystem = this->GetWorld()->GetSubsystem<UYogWorldSubsystem>();
//	UWorld* current_world = worldsubsystem->GetCurrentWorld();
//	if (current_world)
//	{
//		SerializeObject(current_world, CurrentSaveGame->CurrentLevel.ByteData);
//
//
//		//FString level_name = current_world->GetPathName();
//		//CurrentSaveGame->CurrentLevel.
//
//		//FLevelRecord level_detail = CurrentSaveGame->GenerateLevelRecord(current_world);
//		
//		//SerializeObject(level_detail, CurrentSaveGame->LevelPathName);
//
//	}
//
//
//	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
//	if (Player)
//	{
//		SerializeObject(Player, CurrentSaveGame->PlayerCharacter);
//	}
//
//} 

//void UYogSaveSubsystem::PopulateFromCurrentSlot()
//{
//	if (!CurrentSaveGame)
//	{
//		return;
//	}
//
//	// Player controller.
//	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
//	if (PC)
//	{
//		
//		//SerializeObject(Cast<UObject>(PC), CurrentSaveGame->PlayerController);
//	}
//
//
//	TArray<AActor*> WorldObjects;
//	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AActor::StaticClass(), WorldObjects);
//
//}
