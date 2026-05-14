// Fill out your copyright notice in the Description page of Project Settings.

#include "GameModes/YogEntryGameMode.h"

#include "Character/YogPlayerControllerBase.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "System/YogGameInstanceBase.h"
#include "TimerManager.h"
#include "UI/YogUIManagerSubsystem.h"
#include "UObject/ConstructorHelpers.h"

AYogEntryGameMode::AYogEntryGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = nullptr;
	HUDClass = nullptr;
	static ConstructorHelpers::FClassFinder<AYogPlayerControllerBase> PlayerControllerBP(TEXT("/Game/Code/Core/Controller/B_YogPlayerControllerBase"));
	PlayerControllerClass = PlayerControllerBP.Succeeded()
		? static_cast<UClass*>(PlayerControllerBP.Class)
		: AYogPlayerControllerBase::StaticClass();
	bStartPlayersAsSpectators = true;
}

void AYogEntryGameMode::StartPlay()
{
	Super::StartPlay();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &AYogEntryGameMode::ShowEntryMenu);
	}
}

void AYogEntryGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	// Keep the local PlayerController alive for CommonUI, but never spawn a menu pawn.
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);
}

void AYogEntryGameMode::RestartPlayer(AController* NewPlayer)
{
	// Entry menu intentionally has no pawn.
}

APawn* AYogEntryGameMode::SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot)
{
	return nullptr;
}

void AYogEntryGameMode::ShowEntryMenu()
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	ULocalPlayer* LocalPlayer = PC ? PC->GetLocalPlayer() : nullptr;
	if (!LocalPlayer || !LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &AYogEntryGameMode::ShowEntryMenu);
		return;
	}

	if (UYogGameInstanceBase* YogGI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		YogGI->ShowMainMenu();
	}
}
