// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularPlayerController.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Components/ControllerComponent.h"

void AGSCModularPlayerController::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularPlayerController::ReceivedPlayer()
{
	// Player controllers always get assigned a player and can't do much until then
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::ReceivedPlayer();

	TArray<UControllerComponent*> ModularComponents;
	GetComponents(ModularComponents);
	for (UControllerComponent* Component : ModularComponents)
	{
		Component->ReceivedPlayer();
	}
}

void AGSCModularPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	TArray<UControllerComponent*> ModularComponents;
	GetComponents(ModularComponents);
	for (UControllerComponent* Component : ModularComponents)
	{
		Component->PlayerTick(DeltaTime);
	}
}
