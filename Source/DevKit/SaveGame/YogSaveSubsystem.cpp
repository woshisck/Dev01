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

		UE_LOG(LogTemp, Log, TEXT("Serialize Actor finish: %s, size: %d"),*Object->GetName(), Data.Num());
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
	//TODO: INIT SAVE DATA, NEED TO CHANGE IN FUTURE
	SaveGame->WeaponInstanceItems.Empty();
	SaveGame->PlayerStateData.Abilities.Empty();

	APlayerCharacterBase* player = Cast<APlayerCharacterBase>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));	

	//Save current Attribute
	SaveGame->PlayerStateData.SetupAttribute(*player->BaseAttributeSet);
	
	//Abilities save to savegame
	UYogAbilitySystemComponent* ASC = player->GetASC();
	
	TArray<FAbilitySaveData> PlayerAbilities = ASC->GetAllGrantedAbilities();
	for (const FAbilitySaveData& ability_save_data : PlayerAbilities)
	{
		SaveGame->PlayerStateData.Abilities.Add(ability_save_data);
	}

	SaveData(player, SaveGame->PlayerStateData.CharacterByteData);




	//filter the Actor with weaponInstance
	TArray<AActor*> attachedActors;
	player->GetAttachedActors(attachedActors, true, true);

	TArray<AWeaponInstance*> weaponInstance_array;


	for (AActor* actor : attachedActors)
	{
		weaponInstance_array.Add(Cast<AWeaponInstance>(actor));
	}

	for (AWeaponInstance* weaponInstance : weaponInstance_array)
	{
		FWeaponInstanceData weaponInstanceData;

		weaponInstanceData.ActorClassPath = weaponInstance->GetClass()->GetPathName();
		weaponInstanceData.AttachSocket = weaponInstance->AttachSocket;
		weaponInstanceData.Transform = weaponInstance->Relative_Transform;
		
		SaveData(weaponInstance, weaponInstanceData.ByteData);
		SaveGame->WeaponInstanceItems.Add(weaponInstanceData);
		UE_LOG(LogTemp, Warning, TEXT("Save Weapon Data Success!"));
	}


	//for (AActor* attachActor : attachedActors)
	//{
	//	if (Cast<AWeaponInstance>(attachActor))
	//	{
	//		//SaveGame->PlayerStateData.WeaponData.
	//		//SaveGame->WeaponData.WeaponInstanceClasses.Add(attachActor->GetClass());
	//		FWeaponMeshData weapon_data;
	//		weapon_data.weaponInstanceClasses.Add(attachActor->GetClass());
	//		weapon_data.AttachSocket = Cast<AWeaponInstance>(attachActor)->AttachSocket;
	//		SaveGame->WeaponData.WeaponMeshData = weapon_data;
	//		UE_LOG(LogTemp, Warning, TEXT("SaveGame->WeaponData.array_WeaponMeshData.Length: %d"), SaveGame->WeaponData.WeaponMeshData.weaponInstanceClasses.Num());
	//		AWeaponInstance* weaponActor;
	//	}
	//}
}

void UYogSaveSubsystem::LoadPlayer(UYogSaveGame* SaveGame)
{
	//Player load
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	// get index 0 as local client controller
	APlayerController* LocalPlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!LocalPlayerController)
	{
		return;
	}

	//APawn* LocalPlayerPawn = LocalPlayerController->GetPawn();
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(LocalPlayerController->GetPawn());
	if (!Cast<APlayerCharacterBase>(Player))
	{
		return;

	}


	//LOAD START HERE


	for (const FAbilitySaveData& ability_data : SaveGame->PlayerStateData.Abilities)
	{
		Player->GetASC()->K2_GiveAbility(ability_data.AbilityClass, ability_data.Level);
	}




	LoadData(Player, SaveGame->PlayerStateData.CharacterByteData);

	//Weapon Actor load
	for (FWeaponInstanceData& weaponInstance : SaveGame->WeaponInstanceItems)
	{
		UClass* ActorClass = StaticLoadClass(AActor::StaticClass(), nullptr, *weaponInstance.ActorClassPath);

		if (!ActorClass)
		{
			UE_LOG(LogTemp, Error, TEXT("can not load weapon actor: %s"), *weaponInstance.ActorClassPath);
			return;
		}

		//Generate Actor
		FActorSpawnParameters SpawnParams;
		//SpawnParams.Name = FName(*weaponInstance.ActorName);
		SpawnParams.Name = FName("Loaded Weapon Actor");
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


		AActor* WeaponActor = GetWorld()->SpawnActor<AActor>(ActorClass, weaponInstance.Transform, SpawnParams);
		

		if (!WeaponActor)
		{
			UE_LOG(LogTemp, Error, TEXT("generaete Actor failed: %s"), *weaponInstance.ActorClassPath);
			return;
		}

		LoadData(WeaponActor, weaponInstance.ByteData);

		UE_LOG(LogTemp, Warning, TEXT("Load Actor success! : %s (Class: %s)"),*WeaponActor->GetName(), *weaponInstance.ActorClassPath);
		UE_LOG(LogTemp, Warning, TEXT("AttachSocket : %s (Class: %s)"), *WeaponActor->GetName(), *weaponInstance.AttachSocket.ToString());


		Cast<AWeaponInstance>(WeaponActor)->EquipWeaponToCharacter(Player);



		//DEPRECATED CODE
		//AWeaponInstance * weaponActor = NewObject<AWeaponInstance>(this);
		//LoadData(weaponActor, weaponInstance.ByteData);
		//UE_LOG(LogTemp, Warning, TEXT("weaponActor class: %s"), *weaponActor->GetClass()->GetName());


		//UE_LOG(LogTemp, Warning, TEXT("weaponActor class: %s, socket: %s"), *weaponActor->GetClass()->GetName(), *weaponActor->AttachSocket.ToString());

	}




	//for (const TSubclassOf<AWeaponInstance> weapon_class : SaveGame->WeaponData.WeaponMeshData.weaponInstanceClasses)
	//{
	//	
	//	
	//	//UE_LOG(LogTemp, Warning, TEXT("SaveGame->WeaponData.array_WeaponMeshData.Length: %s"), *weapon_class->GetName());
	//	UE_LOG(LogTemp, Warning, TEXT("SaveGame->WeaponData.array_WeaponMeshData.Length: %s"), *weapon_class->GetName());
	//	//SetupWeaponToCharacter
	//}
	
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


	UE_LOG(DevKitGame, Log, TEXT("WRITE SAVE GAME SUCCESS! "));
	OnSaveGameWritten.Broadcast(CurrentSaveGame);
}

/* Load from disk, optional slot name */
void UYogSaveSubsystem::LoadSaveGame(UYogSaveGame* SaveGame)
{
	if (CurrentSaveGame)
	{
		//load saved map

		//UGameplayStatics::OpenLevel(GetWorld(), FName(CurrentSaveGame->MapStateData.LevelName));



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

