#include "YogSaveSubsystem.h"
#include "System/YogWorldSubsystem.h"
#include "YogSaveGame.h"
#include "YogSaveGameArchive.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameModes/YogGameMode.h"
#include "DevKit/DevAssetManager.h"
#include "DevKit/YogBlueprintFunctionLibrary.h"
#include "DevKit/Item/Weapon/WeaponDefinition.h"
#include "DevKit/AbilitySystem/YogAbilitySystemComponent.h"

#include "Components/SkeletalMeshComponent.h"


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

	SaveGame->PlayerStateData.WeaponAbilities = player->AbilityData;

	
	TMap<FGameplayTag, int32> container = player->GetASC()->GetPlayerOwnedTagsWithCounts();

	for (const auto& Pair : container)
	{
		SaveGame->PlayerStateData.PlayerOwnedTags.Add(Pair.Key, Pair.Value);
	}


	//UYogAbilitySystemComponent* ASC = player->GetASC();
	//TArray<FAbilitySaveData> PlayerAbilities = ASC->GetAllGrantedAbilities();
	//
	//// Get all ability specs
	//const TArray<FGameplayAbilitySpec>& AbilitySpecs = ASC->GetActivatableAbilities();
	//for (const FGameplayAbilitySpec& Spec : AbilitySpecs)
	//{
	//	// Skip if ability is invalid
	//	if (!Spec.Ability) continue;
	//	// Convert to save data
	//	FAbilitySaveData SaveData = ConvertAbilitySpecToSaveData(Spec);
	//	SaveGame->PlayerStateData.Abilities.Add(SaveData);
	//}


	//for (const FAbilitySaveData& ability_save_data : PlayerAbilities)
	//{
	//	SaveGame->PlayerStateData.Abilities.Add(ability_save_data);
	//}

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
		weaponInstanceData.Transform = weaponInstance->AttachTransform;

		UAnimInstance* AnimInstance = player->GetMesh()->GetAnimInstance();
		weaponInstanceData.WeaponLayer = weaponInstance->WeaponLayer->GetClass();

		UBlueprint* blueprint = UBlueprint::GetBlueprintFromClass(weaponInstance->WeaponLayer);

		weaponInstanceData.WeaponLayerClassPath = blueprint->GetPathName();

		SaveData(weaponInstance, weaponInstanceData.ByteData);
		SaveGame->WeaponInstanceItems.Add(weaponInstanceData);
		

		UE_LOG(LogTemp, Warning, TEXT("Save Weapon Data Success!"));
	}


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

	if (!Player->GetASC())
	{
		return;
	}

	//LoadData(Player, SaveGame->PlayerStateData.CharacterByteData);

	GiveAbilitiesFromSaveData(Player->GetASC(), SaveGame->PlayerStateData.Abilities);

	//Weapon Actor load
	for (FWeaponInstanceData& weaponInstance : SaveGame->WeaponInstanceItems)
	{
		UClass* WeaponClass = StaticLoadClass(AActor::StaticClass(), nullptr, *weaponInstance.ActorClassPath);

		if (!WeaponClass)
		{
			UE_LOG(LogTemp, Error, TEXT("can not load weapon actor: %s"), *weaponInstance.ActorClassPath);
			return;
		}




		//Generate Actor
		FActorSpawnParameters SpawnParams;
		//SpawnParams.Name = FName(*weaponInstance.ActorName);
		SpawnParams.Name = FName("Loaded Weapon Actor");
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;


		//ASAP:: Throw ERROR BECAUSE LOAD GAME IN GI

		AWeaponInstance* WeaponActor = Cast<AWeaponInstance>(GetWorld()->SpawnActor<AActor>(WeaponClass, weaponInstance.Transform, SpawnParams));
		

		if (!WeaponActor)
		{
			UE_LOG(LogTemp, Error, TEXT("generaete Actor failed: %s"), *weaponInstance.ActorClassPath);
			return;
		}

		UBlueprint* WeaponLayerBlueprint = LoadObject<UBlueprint>(nullptr, *weaponInstance.WeaponLayerClassPath);
		UClass* LayerClass = WeaponLayerBlueprint->GeneratedClass;

	
		if (!LayerClass)
		{
			UE_LOG(LogTemp, Error, TEXT("can not load LayerClass: %s"), *weaponInstance.ActorClassPath);
			return;
		}

		if (LayerClass && Player->GetMesh())
		{
			if (UAnimInstance* AnimInstance = Player->GetMesh()->GetAnimInstance())
			{
				// Link the new layer
				AnimInstance->LinkAnimClassLayers(LayerClass);

			}
		}

		LoadData(WeaponActor, weaponInstance.ByteData);


		//WeaponActor->WeaponLayer = weaponInstance.WeaponLayer;
		

		UE_LOG(LogTemp, Warning, TEXT("Load Actor success! : %s (Class: %s)"),*WeaponActor->GetName(), *weaponInstance.ActorClassPath);
		UE_LOG(LogTemp, Warning, TEXT("AttachSocket : %s (Class: %s)"), *WeaponActor->GetName(), *weaponInstance.AttachSocket.ToString());
		

		//UYogBlueprintFunctionLibrary::EquipWeapon(Player->GetWorld(), Player, WeaponActor);

		FWeaponSpawnData SpawnData;
		SpawnData.ActorToSpawn = WeaponClass;
		SpawnData.AttachSocket = weaponInstance.AttachSocket;
		SpawnData.AttachTransform = weaponInstance.Transform;
		SpawnData.WeaponLayer = weaponInstance.WeaponLayer;
		SpawnData.bShouldSaveToGame = true;


		UYogBlueprintFunctionLibrary::SpawnWeaponOnCharacter(Player, Player->GetTransform(), SpawnData);


		//WeaponActor->EquipWeaponToCharacter(Player);
		//Cast<AWeaponInstance>(WeaponActor)->EquipWeaponToCharacter(Player);

	}
	Player->AbilityData = SaveGame->PlayerStateData.WeaponAbilities;

	
	//Gameplay Tag
	for (const auto& Pair : SaveGame->PlayerStateData.PlayerOwnedTags)
	{
		Player->GetASC()->AddGameplayTagWithCount(Pair.Key, Pair.Value);
		//SaveGame->PlayerStateData.PlayerOwnedTags.Add(Pair.Key, Pair.Value);
	}




}

void UYogSaveSubsystem::LoadMap(UYogSaveGame* SaveGame)
{
	//serialize Map object if necessary
}

void UYogSaveSubsystem::SaveMap(UYogSaveGame* SaveGame)
{
	SaveGame->MapStateData.LevelName = FName(UGameplayStatics::GetCurrentLevelName(GetWorld(), true));
}

FAbilitySaveData UYogSaveSubsystem::ConvertAbilitySpecToSaveData(const FGameplayAbilitySpec& Spec)
{
	FAbilitySaveData SaveData;

	if (Spec.Ability)
	{
		SaveData.AbilityClassPath = Spec.Ability->GetClass()->GetPathName();
		SaveData.AbilityClass = Spec.Ability->GetClass();
	}

	SaveData.Level = Spec.Level;
	SaveData.InputID = Spec.InputID;
	//SaveData.ActiveCount = Spec.ActiveCount;
	//SaveData.bIsActive = Spec.IsActive();
	//SaveData.AbilityTags = Spec.Ability->AbilityTags;
	//SaveData.DynamicAbilityTags = Spec.DynamicAbilityTags;

	//if (Spec.SourceObject)
	//{
	//	SaveData.SourceObjectPath = Spec.SourceObject->GetPathName();
	//}

	return SaveData;

}

FGameplayAbilitySpecHandle UYogSaveSubsystem::ConvertSaveDataToAbilitySpec(UYogAbilitySystemComponent* ASC, const FAbilitySaveData& SaveData)
{
	// Load the ability class
	UClass* AbilityClass = SaveData.AbilityClassPath.TryLoadClass<UYogGameplayAbility>();
	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load ability class: %s"), *SaveData.AbilityClassPath.ToString());
		return FGameplayAbilitySpecHandle();
	}

	// Create ability spec
	FGameplayAbilitySpec Spec(AbilityClass, SaveData.Level, SaveData.InputID);

	// Restore tags
	//Spec.Ability->AbilityTags = SaveData.AbilityTags;
	//Spec.DynamicAbilityTags = SaveData.DynamicAbilityTags;

	// Give ability to ASC
	return ASC->GiveAbility(Spec);
}

void UYogSaveSubsystem::GiveAbilitiesFromSaveData(UYogAbilitySystemComponent* ASC, const TArray<FAbilitySaveData>& AbilitiesData)
{

	for (const FAbilitySaveData& SaveData : AbilitiesData)
	{
		// Give ability to ASC
		FGameplayAbilitySpecHandle Handle = ConvertSaveDataToAbilitySpec(ASC, SaveData);

		if (Handle.IsValid())
		{
			UE_LOG(DevKitGame, Log, TEXT("FGameplayAbilitySpecHandle Handle is Valid "));
			// Try to restore active state if needed
			//if (SaveData.bIsActive && SaveData.ActiveCount > 0)
			//{
			//	// Note: You might need custom logic here based on your game
			//	// Some abilities might auto-activate, others might not
			//	FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
			//	if (Spec && Spec->Ability)
			//	{
			//		// Try to activate if it makes sense for your game
			//		// ASC->TryActivateAbility(Handle);
			//	}
			//}
		}
	}

}






void UYogSaveSubsystem::WriteSaveGame()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(WriteSaveGame);
	CurrentSaveGame->SavedCharacter.Empty();
	
	//CHECK FOR PLAYER STAT, NOT FOR IDE 
	AGameStateBase* GS = GetWorld()->GetGameState();
	check(GS);

	SavePlayer(CurrentSaveGame);
	SaveMap(CurrentSaveGame);


	//WORLD SAVE
	UYogWorldSubsystem* worldsubsystem = GetWorld()->GetSubsystem<UYogWorldSubsystem>();
	UWorld* current_world = worldsubsystem->GetCurrentWorld();
	//CurrentSaveGame->LevelName = current_world->GetFName();
	//CurrentSaveGame->LevelName = current_world->GetFName();


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
	//UGameplayStatics::OpenLevel(GetWorld(), FName(CurrentSaveGame->MapStateData.LevelName));
		
	
	LoadPlayer(SaveGame);


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
	//open level ->
}

