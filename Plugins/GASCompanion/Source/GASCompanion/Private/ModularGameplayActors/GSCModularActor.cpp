// Copyright 2020 Mickael Daniel. All Rights Reserved.

#include "ModularGameplayActors/GSCModularActor.h"

#include "GSCLog.h"
#include "Abilities/GSCAbilitySystemComponent.h"
#include "Components/GameFrameworkComponentManager.h"

AGSCModularActor::AGSCModularActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set this actor to replicate like a Pawn would
	bReplicates = true;

	// Set Ability System Companion with GAS Companion subclass
	AbilitySystemComponent = CreateDefaultSubobject<UGSCAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
}

UAbilitySystemComponent* AGSCModularActor::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AGSCModularActor::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularActor::BeginPlay()
{
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
	Super::BeginPlay();
}

void AGSCModularActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularActor::PostInitProperties()
{
	Super::PostInitProperties();
	if (AbilitySystemComponent)
	{
		GSC_LOG(Verbose, TEXT("AGSCModularActor::PostInitProperties for %s - Setting up ASC Replication Mode to: %d"), *GetName(), ReplicationMode)
		AbilitySystemComponent->SetReplicationMode(ReplicationMode);
	}
}

void AGSCModularActor::GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const
{
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
		OutTagContainer = MoveTemp(OwnedTags);
	}
}

bool AGSCModularActor::HasMatchingGameplayTag(const FGameplayTag InTagToCheck) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(InTagToCheck);
	}

	return false;
}

bool AGSCModularActor::HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAllMatchingGameplayTags(InTagContainer);
	}

	return false;
}

bool AGSCModularActor::HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent)
	{
		return AbilitySystemComponent->HasAnyMatchingGameplayTags(InTagContainer);
	}

	return false;
}