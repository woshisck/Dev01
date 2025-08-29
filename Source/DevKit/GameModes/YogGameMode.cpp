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

void AYogGameMode::SpawnPlayerManually(AYogPlayerControllerBase* PlayerController, const FVector& Location, const FRotator& Rotation)
{
	if (!PlayerController) return;

	// Destroy existing pawn
	if (APawn* ExistingPawn = PlayerController->GetPawn())
	{
		ExistingPawn->Destroy();
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// Spawn player character at specified location
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (DefaultPawnClass)
	{
		APlayerCharacterBase* NewCharacter = World->SpawnActor<APlayerCharacterBase>(
			DefaultPawnClass,
			Location,
			Rotation,
			SpawnParams
		);

		if (NewCharacter)
		{
			PlayerController->Possess(Cast<APawn>(NewCharacter));
			UE_LOG(LogTemp, Log, TEXT("Player spawned manually at specified location"));
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
