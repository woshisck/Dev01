
#include "YogSaveSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/YogGameMode.h"
#include "DevKit/DevAssetManager.h"

#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"


void UYogSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!CurrentSaveGame)
	{
		UYogSaveGame* SaveGameInstance = Cast<UYogSaveGame>(UGameplayStatics::CreateSaveGameObject(UYogSaveGame::StaticClass()));
		CurrentSaveGame = SaveGameInstance;
	}

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UYogSaveSubsystem::OnLevelLoaded);

	if (UWorld* World = GetWorld())
	{
		AYogGameMode* GameMode = Cast<AYogGameMode>(World->GetAuthGameMode());
		if (GameMode)
		{
			// Bind the subsystem's function to the GameMode's event.
			GameMode->OnFinishLevelEvent().AddUObject(this, &UYogSaveSubsystem::WriteSaveGame);
		}
	}

}

void UYogSaveSubsystem::OnLevelLoaded(UWorld* LoadedWorld)
{
	//if (LoadedWorld && LoadedWorld->IsGameWorld())
	//{
	//	//TODO::
	//	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	//	if (PC)
	//	{
	//		APlayerCharacterBase* pawn = Cast<APlayerCharacterBase>(PC->GetPawn());
	//		if (pawn)
	//		{

	//			pawn->SetActorLocation(CurrentSaveGame->PlayerStateData.PlayerLocation);
	//			pawn->SetActorRotation(CurrentSaveGame->PlayerStateData.PlayerRotation);
	//		}
	//	}

	//}
}

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
	


	//Get current Attribute
	SaveGame->PlayerStateData.SetupAttribute(*player->BaseAttributeSet);
	


	//Abilities save to savegame
	UYogAbilitySystemComponent* ASC = player->GetASC();
	TArray<FYogAbilitySaveData> container = ASC->GetAllGrantedAbilities();


	//Weapon find & serialize 
	TArray<AActor*> attachedActors;
	player->GetAttachedActors(attachedActors, true, true);

	//TODO: INIT SAVE DATA
	
	for (AActor* attachActor : attachedActors)
	{
		if (Cast<AWeaponInstance>(attachActor))
		{
			//SaveGame->PlayerStateData.WeaponData.
			//SaveGame->WeaponData.WeaponInstanceClasses.Add(attachActor->GetClass());

			FWeaponMeshData weapon_data;

			weapon_data.weaponInstanceClasses.Add(attachActor->GetClass());
			weapon_data.AttachSocket = Cast<AWeaponInstance>(attachActor)->AttachSocket;
		
			SaveGame->WeaponData.WeaponMeshData = weapon_data;


			UE_LOG(LogTemp, Warning, TEXT("SaveGame->WeaponData.array_WeaponMeshData.Length: %d"), SaveGame->WeaponData.WeaponMeshData.weaponInstanceClasses.Num());
		}
	}
}

void UYogSaveSubsystem::LoadPlayer(UYogSaveGame* SaveGame)
{



	for (const TSubclassOf<AWeaponInstance> weapon_class : SaveGame->WeaponData.WeaponMeshData.weaponInstanceClasses)
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveGame->WeaponData.array_WeaponMeshData.Length: %s"), *weapon_class->GetName());
		
	}
	
	/*TODO: FIX THE SAVE GAME PLAYER*/
	//APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	//if (PC)
	//{
	//	APlayerCharacterBase* pawn = Cast<APlayerCharacterBase>(PC->GetPawn());
	//	if (pawn)
	//	{
	//		//AWeaponInstance* weaponActor = GetWorld()->SpawnActorDeferred<AWeaponInstance>(AWeaponInstance::StaticClass(), pawn->GetTransform());
	//		AWeaponInstance* weaponActor = nullptr;
	//		LoadData(weaponActor, SaveGame->PlayerStateData.WeaponActorByteData);
	//		//UE_LOG(LogTemp, Warning, TEXT("weaponActor: %s"), *weaponActor->ToString());
	//		FName socket = FName(TEXT("WeaponSocket_R"));
	//		weaponActor->AttachToComponent(Cast<APlayerCharacterBase>(pawn)->GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, socket);
	//	
	//	}
	//}



}

void UYogSaveSubsystem::LoadMap(UYogSaveGame* SaveGame)
{
	//serialize Map object if necessary
}

void UYogSaveSubsystem::SaveMap(UYogSaveGame* SaveGame)
{
	SaveGame->MapStateData.LevelName = FName(UGameplayStatics::GetCurrentLevelName(GetWorld(), true));

}






void UYogSaveSubsystem::WriteSaveGame()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WriteSaveGame);
	CurrentSaveGame->SavedCharacter.Empty();
	CurrentSaveGame->SavedActorMap.Empty();
	
	//CHECK FOR PLAYER STAT, NOT FOR IDE 
	AGameStateBase* GS = GetWorld()->GetGameState();
	check(GS);
	SavePlayer(CurrentSaveGame);

	SaveMap(CurrentSaveGame);


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
		//load saved map

		UGameplayStatics::OpenLevel(GetWorld(), FName(CurrentSaveGame->MapStateData.LevelName));

		LoadPlayer(CurrentSaveGame);

		//UDevAssetManager* devAssetManager = UDevAssetManager::GetDevAssetManager();
		

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

