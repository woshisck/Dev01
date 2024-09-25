// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularDefaultPawn.h"

#include "Abilities/GSCAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GSCLog.h"

AGSCModularDefaultPawn::AGSCModularDefaultPawn(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Replication Mode is set in PostInitProperties to allow users to change the default value from BP
}

UAbilitySystemComponent* AGSCModularDefaultPawn::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSCModularDefaultPawn::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularDefaultPawn::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularDefaultPawn::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularDefaultPawn::PostInitProperties()
{
	Super::PostInitProperties();
	if (AbilitySystemComponent)
	{
		GSC_LOG(Verbose, TEXT("AGSCModularDefaultPawn::PostInitProperties for %s - Setting up ASC Replication Mode to: %d"), *GetName(), ReplicationMode)
		AbilitySystemComponent->SetReplicationMode(ReplicationMode);
	}
}

void AGSCModularDefaultPawn::GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const
{
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
		OutTagContainer = MoveTemp(OwnedTags);
	}
}

bool AGSCModularDefaultPawn::HasMatchingGameplayTag(const FGameplayTag InTagToCheck) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(InTagToCheck);
	}

	return false;
}

bool AGSCModularDefaultPawn::HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAllMatchingGameplayTags(InTagContainer);
	}

	return false;
}

bool AGSCModularDefaultPawn::HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAnyMatchingGameplayTags(InTagContainer);
	}

	return false;
}
