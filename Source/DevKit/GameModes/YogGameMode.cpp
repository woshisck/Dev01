// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameMode.h"
#include "Player/YogPlayerControllerBase.h"
#include "Player/PlayerCharacterBase.h"
#include <Kismet/GameplayStatics.h>
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "DevKit/Map/YogLevelScript.h"
#include "DevKit/System/YogGameInstanceBase.h"
#include "DevKit/Enemy/EnemyCharacterBase.h"
#include "DevKit/Mob/MobSpawner.h"

AYogGameMode::AYogGameMode(const FObjectInitializer& ObjectInitializer)
{
	bAutoSpawnPlayer = false;
	DefaultPawnClass = nullptr;

	//CurrentWaveIndex = 0;
	//SpawnedInWave = 0;
	//ActiveMobCount = 0;

}


void AYogGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());
	if (GI && GI->PersistentSaveData)
	{
		FActorSpawnParameters Params;
		Params.Owner = NewPlayer;

		APlayerCharacterBase* LoadedChar = GetWorld()->SpawnActor<APlayerCharacterBase>(
			GI->PersistentSaveData->SavedCharacterClass,
			Params
		);

		NewPlayer->Possess(LoadedChar);
	}
	else
	{
		Super::HandleStartingNewPlayer_Implementation(NewPlayer); // fallback
	}
}



void AYogGameMode::RestartPlayer(AController* NewPlayer)
{
	if (!bAutoSpawnPlayer)
	{
		// Do nothing - prevent auto spawning
		UE_LOG(LogTemp, Warning, TEXT("Auto player spawning disabled"));
		return;
	}

	// Fall back to default behavior if enabled
	Super::RestartPlayer(NewPlayer);

	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

}

void AYogGameMode::StartPlay()
{
	Super::StartPlay();

	UWorld* World = GetWorld();

	if (!World)
	{
		return;
	}

	ULevel* CurrentLevel = World->GetCurrentLevel();
	if (!CurrentLevel)
	{
		return;
	}

	UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance());

	UGameInstance* GameInstancePtr = Cast<UGameInstance>(GetWorld()->GetGameInstance());
	UYogSaveSubsystem* SaveSubsystem = GI->GetSubsystem<UYogSaveSubsystem>();

	//if (SaveSubsystem->CurrentSaveGame)
	//{
	//	SaveSubsystem->LoadSaveGame(SaveSubsystem->CurrentSaveGame);
	//}
	//else
	//{
	//	// spawn default player char
	//}

	//TODO: this function calls after openLevel : 
	//[get player + get transform -> spawn player -> poccess ->] in game mode
	



	AYogLevelScript* LevelScriptActor = Cast<AYogLevelScript>(CurrentLevel->GetLevelScriptActor());
	if (LevelScriptActor)
	{
		RemainKillCount = LevelScriptActor->MonsterKillCountTarget;

		UE_LOG(LogTemp, Warning, TEXT("Found LevelScriptActor: %s"), *LevelScriptActor->GetName());

		this->OnFinishLevelEvent().AddUObject(SaveSubsystem, &UYogSaveSubsystem::WriteSaveGame);
	}



	//if (UWorld* World = GetWorld())
	//{
	//	AYogGameMode* GameMode = Cast<AYogGameMode>(World->GetAuthGameMode());
	//	if (GameMode)
	//	{
	//		// Bind the subsystem's function to the GameMode's event.
	//		GameMode->OnFinishLevelEvent().AddUObject(this, &UYogSaveSubsystem::WriteSaveGame);
	//	}
	//}



	//SaveSubsystem->WriteSaveGame().AddUObject(this, &AYogGameMode::OnFinishLevel);


	
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


//TArray<AActor*> OutActors
//TArray<AMobSpawner*> Spawners;
//UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);






///////////////////////////////  AI  ////////////////////////////////
void AYogGameMode::StartSpawnTimer()
{
	GetWorld()->GetTimerManager().SetTimer(SpawnTimerHandle, this, &AYogGameMode::SpawnMob, SpawnConfig.Interval, true, SpawnConfig.FirstDelay);

}

void AYogGameMode::SpawnMob()
{
	//Spawn Algo:
	UE_LOG(LogTemp, Warning, TEXT("SpawnMob called at %f"), GetWorld()->GetTimeSeconds());

	TArray<AActor*> OutActors;
	TArray<AMobSpawner*> Spawners;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMobSpawner::StaticClass(), OutActors);

	if (OutActors.Num() <= 0)
	{
		return;
	}

	for (AActor* a : OutActors)
	{
		Spawners.Add(Cast<AMobSpawner>(a));
	}

	//const int32 TotalMobToSpawn = 15; 


	//for (int32 i = 0; i < TotalMobToSpawn; i++)
	for (int32 i = 0; i < SpawnConfig.MaxCall; i++)
	{
		int32 RandomIndex = FMath::RandRange(0, Spawners.Num() - 1);
		AMobSpawner* ChosenSpawner = Spawners[RandomIndex];

		if (ChosenSpawner)
		{
			ChosenSpawner->SpawnMob(SpawnConfig.MobClass); // Assuming AMobSpawner has SpawnMob()
			UE_LOG(LogTemp, Warning, TEXT("Mob %d spawned at spawner %d"), i + 1, RandomIndex);
		}
	}


	Current_CallCount++; 
	if (Current_CallCount >= SpawnConfig.MaxCall)
	{ 
		GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);
	}

}


void AYogGameMode::TriggerImmediateSpawn()
{
	// Clear the current timer
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimerHandle);

	// Calculate how much time has passed since the last spawn
	float TimeSinceLastSpawn = GetWorld()->GetTimerManager().GetTimerElapsed(SpawnTimerHandle);
	float TimeRemaining = SpawnConfig.Interval - TimeSinceLastSpawn;

	// Call spawn immediately
	SpawnMob();

	// Restart the timer with the remaining time
	if (Current_CallCount < SpawnConfig.MaxCall)
	{
		GetWorld()->GetTimerManager().SetTimer(
			SpawnTimerHandle,
			this,
			&AYogGameMode::SpawnMob,
			SpawnConfig.Interval,
			true,
			FMath::Max(TimeRemaining, 0.1f) // Ensure there's at least a small delay
		);
	}
}


///////////////////////////////  AI  ////////////////////////////////


void AYogGameMode::SomeEventThatTriggersImmediateSpawn()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &AYogGameMode::TriggerImmediateSpawn);

}

 
void AYogGameMode::UpdateFinishLevel(int count)
{
	this->MonsterKillCount += count;
	UE_LOG(LogTemp, Log, TEXT("MonsterKillCount: %d"), this->MonsterKillCount);
	//TODO: HARD CODE
	if (this->MonsterKillCount >= RemainKillCount)
	{
		OnFinishLevel.Broadcast();
		FinishLevelEvent.Broadcast();
		UE_LOG(LogTemp, Log, TEXT("OnFinishLevel.Broadcast() Calling;"));
	}
}

void AYogGameMode::BeginPlay()
{
	Super::BeginPlay();



}
