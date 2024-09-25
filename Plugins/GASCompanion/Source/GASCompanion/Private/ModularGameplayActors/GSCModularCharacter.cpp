// Copyright 2021 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularCharacter.h"

#include "Components/GameFrameworkComponentManager.h"
#include "Abilities/GSCAbilitySystemComponent.h"
// #include "GSCLog.h"

AGSCModularCharacter::AGSCModularCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = CreateDefaultSubobject<UGSCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	// Replication Mode is set in PostInitProperties to allow users to change the default value from BP
}

UAbilitySystemComponent* AGSCModularCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSCModularCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularCharacter::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularCharacter::PostInitProperties()
{
	Super::PostInitProperties();
	if (AbilitySystemComponent)
	{
		// GSC_LOG(Verbose, TEXT("AGSCModularCharacter::PostInitProperties for %s - Setting up ASC Replication Mode to: %d"), *GetName(), ReplicationMode)
		AbilitySystemComponent->SetReplicationMode(ReplicationMode);
	}
}

void AGSCModularCharacter::GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const
{
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
		OutTagContainer = MoveTemp(OwnedTags);
	}
}

bool AGSCModularCharacter::HasMatchingGameplayTag(const FGameplayTag InTagToCheck) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(InTagToCheck);
	}

	return false;
}

bool AGSCModularCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAllMatchingGameplayTags(InTagContainer);
	}

	return false;
}

bool AGSCModularCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAnyMatchingGameplayTags(InTagContainer);
	}

	return false;
}
