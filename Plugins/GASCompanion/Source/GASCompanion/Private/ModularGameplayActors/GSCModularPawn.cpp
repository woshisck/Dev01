// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularPawn.h"

#include "Abilities/GSCAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GSCLog.h"

AGSCModularPawn::AGSCModularPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Replication Mode is set in PostInitProperties to allow users to change the default value from BP
}

UAbilitySystemComponent* AGSCModularPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSCModularPawn::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularPawn::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularPawn::PostInitProperties()
{
	Super::PostInitProperties();
	if (AbilitySystemComponent)
	{
		GSC_LOG(Verbose, TEXT("AGSCModularPawn::PostInitProperties for %s - Setting up ASC Replication Mode to: %d"), *GetName(), ReplicationMode)
		AbilitySystemComponent->SetReplicationMode(ReplicationMode);
	}
}

void AGSCModularPawn::GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const
{
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
		OutTagContainer = MoveTemp(OwnedTags);
	}
}

bool AGSCModularPawn::HasMatchingGameplayTag(const FGameplayTag InTagToCheck) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(InTagToCheck);
	}

	return false;
}

bool AGSCModularPawn::HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAllMatchingGameplayTags(InTagContainer);
	}

	return false;
}

bool AGSCModularPawn::HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAnyMatchingGameplayTags(InTagContainer);
	}

	return false;
}
