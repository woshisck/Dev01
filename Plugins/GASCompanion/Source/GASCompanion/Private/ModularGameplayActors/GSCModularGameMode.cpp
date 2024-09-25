// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularGameMode.h"

#include "GameFramework/PlayerState.h"
#include "ModularGameplayActors/GSCModularCharacter.h"
#include "ModularGameplayActors/GSCModularGameState.h"
#include "ModularGameplayActors/GSCModularPlayerController.h"

AGSCModularGameModeBase::AGSCModularGameModeBase()
{
	GameStateClass = AGSCModularGameStateBase::StaticClass();
	PlayerControllerClass = AGSCModularPlayerController::StaticClass();
	PlayerStateClass = APlayerState::StaticClass();
	DefaultPawnClass = AGSCModularCharacter::StaticClass();
}

AGSCModularGameMode::AGSCModularGameMode()
{
	GameStateClass = AGSCModularGameState::StaticClass();
	PlayerControllerClass = AGSCModularPlayerController::StaticClass();
	PlayerStateClass = APlayerState::StaticClass();
	DefaultPawnClass = AGSCModularCharacter::StaticClass();
}
