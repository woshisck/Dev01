
#include "YogSaveSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/YogGameMode.h"
#include "DevKit/DevAssetManager.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"


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

void UYogSaveSubsystem::SaveData(UObject* Object, UPARAM(ref)TArray<uint8>& Data)
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

void UYogSaveSubsystem::SaveTransformData(FVector& saveGame_loc, FVector& target_loc, FRotator& saveGame_rotate, FRotator& target_rot)
{
	saveGame_loc = target_loc;
	saveGame_rotate = target_rot;
}


void UYogSaveSubsystem::LoadLevelData(UYogSaveGame* SaveGame)
{
	UWorld* World = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport)->World();
	if (World)
	{
		// Load the level if it's different from current

		//FString CurrentLevelName = UGameplayStatics::GetCurrentLevelName(World, true);
		//if (CurrentLevelName != SaveGame->LevelName)
		//{
		//	UGameplayStatics::OpenLevel(World, FName(*SaveGame->LevelName));
		//	// Wait for level load complete before spawning actors
		//	return;
		//}

	}
	else
	{
		return;
	}

}


void UYogSaveSubsystem::SavePlayer(UYogSaveGame* SaveGame)
{

	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	
	SaveGame->PlayerStateData.SetupAttribute(*player->BaseAttributeSet);
	SaveGame->PlayerStateData.PlayerLocation = player->GetActorLocation();
	SaveGame->PlayerStateData.PlayerRotation = player->GetActorRotation();

	UYogAbilitySystemComponent* ASC = player->GetASC();

	TArray<FYogAbilitySaveData> container = ASC->GetAllGrantedAbilities();

	for (FYogAbilitySaveData& abilitydata : container)
	{
		SaveGame->PlayerStateData.YogAbilityDataArray.Add(abilitydata);
	}


	//Weapon find & serialize 
	TArray<AActor*> attachedActors;
	player->GetAttachedActors(attachedActors, true, true);
	for (AActor* attachActor : attachedActors)
	{
		if (Cast<AWeaponInstance>(attachActor))
		{
			//SaveGame->PlayerStateData.WeaponData.
			
			SaveData(attachActor, CurrentSaveGame->PlayerStateData.WeaponActorByteData);
		}
	}
}

void UYogSaveSubsystem::LoadPlayer(UYogSaveGame* SaveGame)
{
	
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC)
	{
		APawn* pawn = PC->GetPawn();
		if (pawn)
		{

			pawn->SetActorLocation(CurrentSaveGame->PlayerStateData.PlayerLocation);
			pawn->SetActorRotation(CurrentSaveGame->PlayerStateData.PlayerRotation);	
			

			AWeaponInstance* weaponActor = GetWorld()->SpawnActorDeferred<AWeaponInstance>(AWeaponInstance::StaticClass(), pawn->GetTransform());

			LoadData(weaponActor, CurrentSaveGame->PlayerStateData.WeaponActorByteData);
			
			FName socket = FName(TEXT("WeaponAttachSocket"));
			weaponActor->AttachToComponent(Cast<APlayerCharacterBase>(pawn)->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, socket);
		
		}
	}
//APlayerCharacterBase* player = NewObject<APlayerCharacterBase>(this, APlayerCharacterBase::StaticClass());
//
////LoadData(player, CurrentSaveGame->PlayerCharacter);


}

void UYogSaveSubsystem::SaveAttachedWeapon()
{
	//PLAYER SAVE 
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));

	if (player)
	{
		TArray<AActor*> attachedActors;
		player->GetAttachedActors(attachedActors,true, true);




		//SaveData(player, CurrentSaveGame->YogSavePlayers.CharacterByteData);
		FVector target_loc = player->GetActorLocation();
		FRotator target_rot = player->GetActorRotation();

		SaveTransformData(CurrentSaveGame->current_Location, target_loc, CurrentSaveGame->current_Rotation, target_rot);

		//CurrentSaveGame->YogSavePlayers.PlayerLocation = player->GetActorLocation();
		//CurrentSaveGame->YogSavePlayers.PlayerRotation = player->GetActorRotation();
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
	CurrentSaveGame->SavedCharacter.Empty();
	CurrentSaveGame->SavedActorMap.Empty();
	
	//CHECK FOR PLAYER STAT, NOT FOR IDE 
	AGameStateBase* GS = GetWorld()->GetGameState();
	check(GS);


	//PLAYER SAVE 
	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
	if (player)
	{
		
		//SaveData(player, CurrentSaveGame->YogSavePlayers.CharacterByteData);
		FVector target_loc = player->GetActorLocation();
		FRotator target_rot = player->GetActorRotation();

		//TODO: SAVE CURRENT ABILITIES
		//CurrentSaveGame->YogSavePlayers.SavedAbilities = player->GetAbilitySystemComponent()->GetAllAbilities();

		SaveTransformData(CurrentSaveGame->current_Location, target_loc, CurrentSaveGame->current_Rotation, target_rot);

	}

	//WORLD SAVE
	UYogWorldSubsystem* worldsubsystem = GetWorld()->GetSubsystem<UYogWorldSubsystem>();
	UWorld* current_world = worldsubsystem->GetCurrentWorld();
	//CurrentSaveGame->LevelName = current_world->GetFName();
	CurrentSaveGame->LevelName = current_world->GetFName();


	if (UGameplayStatics::DoesSaveGameExist(SaveSlotName, 0))
	{
		UGameplayStatics::DeleteGameInSlot(SaveSlotName, 0);

		
		UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SaveSlotName, 0);
	}
	else
	{
		UGameplayStatics::SaveGameToSlot(CurrentSaveGame, SaveSlotName, 0);
	}


	UE_LOG(DevKitGame, Log, TEXT("FINISH WRITE SAVE GAME"));

	OnSaveGameWritten.Broadcast(CurrentSaveGame);
}

/* Load from disk, optional slot name */
void UYogSaveSubsystem::LoadSaveGame(FString InSlotName)
{
	if (CurrentSaveGame)
	{


		UE_LOG(DevKitGame, Log, TEXT("CurrentSaveGame->LevelName: %s"), *CurrentSaveGame->current_Location.ToString());
		UE_LOG(DevKitGame, Log, TEXT("CurrentSaveGame->LevelName: %s"), *CurrentSaveGame->current_Rotation.ToString());

		APlayerController* PC = GEngine->GetFirstLocalPlayerController(GEngine->GameViewport->GetWorld());

		APlayerCharacterBase* player_pawn = Cast<APlayerCharacterBase>(PC->GetPawn());
		if (player_pawn)
		{
			UE_LOG(DevKitGame, Log, TEXT("player_pawn is : %s"), *player_pawn->GetName());
			player_pawn->SetActorLocation(CurrentSaveGame->current_Location);
			player_pawn->SetActorRotation(CurrentSaveGame->current_Rotation);
			
		}
		//APlayerCharacterBase* player = NewObject<APlayerCharacterBase>(this, APlayerCharacterBase::StaticClass());
		//
		////LoadData(player, CurrentSaveGame->PlayerCharacter);

		

		//TODO: OpenLevel

		//UGameplayStatics::OpenLevel(GetWorld(), CurrentSaveGame->LevelName, true);

		UDevAssetManager* devAssetManager = UDevAssetManager::GetDevAssetManager();
		

		if (PC->GetPawn())
		{
			
		}

		//APlayerCharacterBase* T_player = GetWorld()->SpawnActorDeferred<APlayerCharacterBase>(devAssetManager->PlayerBlueprintClass, FTransform::Identity);


		//if (devAssetManager->YogPlayerClass)
		//{
		//	//spawn player character at location;
		//	AYogGameMode* CurrentGameMode = Cast<AYogGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
		//	if (CurrentGameMode)
		//	{
		//		CurrentGameMode->SpawnPlayerFromSaveData(CurrentSaveGame);
		//		//CurrentGameMode->SpawnAndPoccessAvatar(APlayerCharacterBase* player, FVector location, FRotator rotation)
		//		//CurrentGameMode->SpawnAndPoccessAvatar(player, CurrentSaveGame->YogSavePlayers.PlayerLocation, CurrentSaveGame->YogSavePlayers.PlayerRotation);
		//	}
		//	UE_LOG(DevKitGame, Log, TEXT("player name : %s"), *player->GetName());
		//}

	}
	//open level ->
}

