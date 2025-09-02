// Fill out your copyright notice in the Description page of Project Settings.


#include "YogGameMode.h"
#include "Player/YogPlayerControllerBase.h"
#include "Character/PlayerCharacterBase.h"

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
		return;
	}

	// Fall back to default behavior if enabled
	Super::RestartPlayer(NewPlayer);
}

void AYogGameMode::StartPlay()
{
	Super::StartPlay();

	// You can manually trigger spawning here if needed
	// or leave it disabled for complete manual control
}

void AYogGameMode::SpawnPlayerAtPlayerStart(APlayerCharacterBase* player, const FString& IncomingName)
{
	//YogSpawnPoint_0
	UWorld* World = GetWorld();

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
