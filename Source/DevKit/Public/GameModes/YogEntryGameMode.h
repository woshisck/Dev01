// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ModularGameMode.h"
#include "YogEntryGameMode.generated.h"

class AController;
class AActor;
class APawn;
class APlayerController;

UCLASS()
class DEVKIT_API AYogEntryGameMode : public AModularGameModeBase
{
	GENERATED_BODY()

public:
	AYogEntryGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void StartPlay() override;
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;
	virtual void RestartPlayer(AController* NewPlayer) override;
	virtual APawn* SpawnDefaultPawnFor_Implementation(AController* NewPlayer, AActor* StartSpot) override;

private:
	void ShowEntryMenu();
};
