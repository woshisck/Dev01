// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularPlayerState.h"

#include "Abilities/GSCAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Components/PlayerStateComponent.h"
#include "GSCLog.h"

AGSCModularPlayerState::AGSCModularPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create ability system component, and set it to be explicitly replicated
	AbilitySystemComponent = CreateDefaultSubobject<UGSCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	// By default, specifically set abilities to persist across deaths / respawns or possessions
	//
	// Note: This is to avoid an issue with ability input binding with Player State setups, as re-granting the ability
	// and trying to bind input again is giving trouble (whereas keeping them around solves this).
	// ASC on Pawns doesn't have this problem.
	AbilitySystemComponent->bResetAbilitiesOnSpawn = false;
	
	// Replication Mode is set in PostInitProperties to allow users to change the default value from BP

	// Set PlayerState's NetUpdateFrequency to sensible defaults.
	//
	// Default is very low for PlayerStates and introduces perceived lag in the ability system.
	// 100 is probably way too high for a shipping game, you can adjust to fit your needs.
	NetUpdateFrequency = 10.0f;
}

void AGSCModularPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularPlayerState::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularPlayerState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularPlayerState::PostInitProperties()
{
	Super::PostInitProperties();
	if (AbilitySystemComponent)
	{
		GSC_LOG(Verbose, TEXT("AGSCModularPlayerState::PostInitProperties for %s - Setting up ASC Replication Mode to: %d"), *GetName(), ReplicationMode)
		AbilitySystemComponent->SetReplicationMode(ReplicationMode);
	}
}

void AGSCModularPlayerState::Reset()
{
	Super::Reset();

	TArray<UPlayerStateComponent*> ModularComponents;
	GetComponents(ModularComponents);
	for (UPlayerStateComponent* Component : ModularComponents)
	{
		Component->Reset();
	}
}

UAbilitySystemComponent* AGSCModularPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSCModularPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	TInlineComponentArray<UPlayerStateComponent*> PlayerStateComponents;
	GetComponents(PlayerStateComponents);
	for (UPlayerStateComponent* SourceComponent : PlayerStateComponents)
	{
		if (UPlayerStateComponent* TargetComp = Cast<UPlayerStateComponent>(static_cast<UObject*>(FindObjectWithOuter(PlayerState, SourceComponent->GetClass(), SourceComponent->GetFName()))))
		{
			SourceComponent->CopyProperties(TargetComp);
		}
	}
}
