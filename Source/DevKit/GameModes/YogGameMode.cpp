// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameMode.h"
#include "Player/YogPlayerControllerBase.h"
#include "Player/PlayerCharacterBase.h"
#include <Kismet/GameplayStatics.h>
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
	bAutoSpawnPlayer = false;
}

void AYogGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!bAutoSpawnPlayer)
	{
		// Do nothing - prevent auto spawning
		UE_LOG(LogTemp, Warning, TEXT("Auto player spawning disabled"));

		//UGameInstance* GameInstancePtr = Cast<UGameInstance>(GetWorld()->GetGameInstance());
		//UYogSaveSubsystem* SaveSubsystem = GameInstancePtr->GetSubsystem<UYogSaveSubsystem>();
		//if (SaveSubsystem->CurrentSaveGame)
		//{
		//	SpawnPlayerFromSaveData(SaveSubsystem->CurrentSaveGame);
		//}

		return;
	}

	// Fall back to default behavior if enabled
	Super::RestartPlayer(NewPlayer);
}

void AYogGameMode::StartPlay()
{
	Super::StartPlay();
	//TODO: this function calls after openLevel : 
	//[get player + get transform -> spawn player -> poccess ->] in game mode
	
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	UYogSaveSubsystem* save_subsystem = UGameInstance::GetSubsystem<UYogSaveSubsystem>(GetGameInstance());
	if (save_subsystem->CurrentSaveGame)
	{
		//NEXT MOVE : save_subsystem->CurrentSaveGame->
		//APlayerCharacterBase* currentSave_player = Cast<APlayerCharacterBase>(save_subsystem->LoadData());
	}
}

void AYogGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

}

void AYogGameMode::SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName)
{
	//YogSpawnPoint_0
	UWorld* World = GetWorld();
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	//TODO: NEED TO CHANGE IN THE FUTURE DEV
	APlayerController* PlayerController = World->GetFirstPlayerController();


	AActor* PlayerStarter = FindPlayerStart(PlayerController, TEXT("YogSpawnPoint_0"));
	if (PlayerStarter)
	{
		if (PlayerController)
		{
			//if (APawn* ExistingPawn = PlayerController->GetPawn())
			//{
			//	ExistingPawn->Destroy();
			//}


			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			APlayerCharacterBase* NewCharacter = World->SpawnActor<APlayerCharacterBase>
			(
				DefaultPawnClass,
				PlayerStarter->GetActorLocation(),
				PlayerStarter->GetActorRotation(),
				SpawnParams
			);
			NewCharacter = player;

			if (NewCharacter)
			{
				
				PlayerController->Possess(Cast<APawn>(NewCharacter));
				UE_LOG(LogTemp, Log, TEXT("Player spawned manually at specified location"));
			}

		}
	}

}

void AYogGameMode::SpawnAndPossessCharacter(APlayerController* PlayerController, UYogSaveGame* LoadedData)
{
	if (!PlayerController || !LoadedData) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// Clear existing pawn if needed
	if (APawn* ExistingPawn = PlayerController->GetPawn())
	{
		ExistingPawn->Destroy();
	}

	// Spawn parameters
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Spawn character at saved location
	TSubclassOf<APlayerCharacterBase> player_class;
	APlayerCharacterBase* SpawnedCharacter = GetWorld()->SpawnActorDeferred<APlayerCharacterBase>(player_class, FTransform::Identity);

	//APlayerCharacterBase* SpawnedCharacter = World->SpawnActorDeferred<APlayerCharacterBase>(
	//	YourCharacterClass, // Your character class reference
	//	LoadedData->PlayerLocation,
	//	LoadedData->PlayerRotation,
	//	SpawnParams
	//);

	//setup Savedata, reference:
	if (SpawnedCharacter)
	{
		// Apply loaded data to character
		//SpawnedCharacter->SetHealth(LoadedData->Health);
		//SpawnedCharacter->SetScore(LoadedData->Score);

		// Possess the character
		PlayerController->Possess(SpawnedCharacter);
	}
}


void AYogGameMode::OnGameRuleLoaded(const UYogGameRule* CurrentGameRule)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Cast<APlayerController>(*Iterator);
		if ((PC != nullptr) && (PC->GetPawn() == nullptr))
		{
			if (PlayerCanRestart(PC))
			{
				RestartPlayer(PC);
			}
		}
	}
}

