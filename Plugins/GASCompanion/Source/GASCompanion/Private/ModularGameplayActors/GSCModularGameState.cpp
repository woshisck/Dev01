// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularGameState.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/GameStateComponent.h"

void AGSCModularGameStateBase::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularGameStateBase::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularGameStateBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}


void AGSCModularGameState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularGameState::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	TArray<UGameStateComponent*> ModularComponents;
	GetComponents(ModularComponents);
	for (UGameStateComponent* Component : ModularComponents)
	{
		Component->HandleMatchHasStarted();
	}
}
