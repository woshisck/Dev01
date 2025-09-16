
#include "YogSaveSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/YogGameMode.h"



UYogSaveGame* UYogSaveSubsystem::GetCurrentSave()
{
	if (CurrentSaveGame)
	{
		return CurrentSaveGame;
	}
	else
	{
		UYogSaveGame* SaveGameInstance = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		CurrentSaveGame = SaveGameInstance;
		return SaveGameInstance;
	}

}

UYogSaveGame* UYogSaveSubsystem::CreateSaveGameInst()
{
	UYogSaveGame* SaveGameInstance = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
	return SaveGameInstance;
}

void UYogSaveSubsystem::SaveData(UObject* Object, TArray<uint8>& Data)
{
	if (Object == nullptr)
	{
		return;
	}
	else
	{
		FMemoryWriter MemoryWriter = FMemoryWriter(Data, true);
		FYogSaveGameArchive MyArchive = FYogSaveGameArchive(MemoryWriter);

		Object->Serialize(MyArchive);
	}


}

void UYogSaveSubsystem::LoadData(UObject* Object, UPARAM(ref)TArray<uint8>& Data)
{
	if (Object == nullptr)
	{
		return;
	}
	else
	{
		FMemoryReader MemoryReader(Data, true);

		FYogSaveGameArchive Ar(MemoryReader);
		Object->Serialize(Ar);
	}

}

void UYogSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!CurrentSaveGame)
	{
		UYogSaveGame* SaveGameInstance = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		CurrentSaveGame = SaveGameInstance;
	}
}



void UYogSaveSubsystem::WriteSaveGame()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WriteSaveGame);
	CurrentSaveGame->SavedPlayers.Empty();
	CurrentSaveGame->SavedActorMap.Empty();
	
	//CHECK FOR PLAYER STAT, NOT FOR IDE 
	AGameStateBase* GS = GetWorld()->GetGameState();
	check(GS);


	//PLAYER SAVE 
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (player)
	{
		SaveData(player, CurrentSaveGame->PlayerCharacter);
		SaveData(player, CurrentSaveGame->YogSavePlayers.CharacterByteData);
		CurrentSaveGame->YogSavePlayers.PlayerLocation = player->GetActorLocation();
		CurrentSaveGame->YogSavePlayers.PlayerRotation = player->GetActorRotation();
	}

	//WORLD SAVE
	UYogWorldSubsystem* worldsubsystem = GetWorld()->GetSubsystem<UYogWorldSubsystem>();
	UWorld* current_world = worldsubsystem->GetCurrentWorld();
	CurrentSaveGame->LevelName = current_world->GetFName();
	CurrentSaveGame->YogSaveMap.LevelName = current_world->GetFName();


	UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SaveSlotName, 0);
	UE_LOG(DevKitGame, Log, TEXT("FINISH WRITE SAVE GAME"));

	OnSaveGameWritten.Broadcast(CurrentSaveGame);
}

/* Load from disk, optional slot name */
void UYogSaveSubsystem::LoadSaveGame(FString InSlotName)
{
	if (CurrentSaveGame)
	{
		UE_LOG(DevKitGame, Log, TEXT("CurrentSaveGame->LevelName: %s"), *CurrentSaveGame->YogSaveMap.LevelName.ToString());

		APlayerCharacterBase* player = NewObject<APlayerCharacterBase>(this, APlayerCharacterBase::StaticClass());
		
		//LoadData(player, CurrentSaveGame->PlayerCharacter);

		LoadData(player, CurrentSaveGame->YogSavePlayers.CharacterByteData);

		UGameplayStatics::OpenLevel(GetWorld(), CurrentSaveGame->LevelName, true);

		AYogGameMode* CurrentGameMode = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld()));

		if (CurrentGameMode)
		{
			//CurrentGameMode->SpawnAndPoccessAvatar(APlayerCharacterBase* player, FVector location, FRotator rotation)
			//CurrentGameMode->SpawnAndPoccessAvatar(player, CurrentSaveGame->YogSavePlayers.PlayerLocation, CurrentSaveGame->YogSavePlayers.PlayerRotation);

		}


		UE_LOG(DevKitGame, Log, TEXT("player name : %s"), *player->GetName());
	}
	//open level ->
}