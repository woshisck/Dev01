// Copyright 2021 Mickael Daniel. All Rights Reserved.


#include "ModularGameplayActors/GSCModularPlayerStateCharacter.h"

#include "AbilitySystemGlobals.h"
#include "Components/GameFrameworkComponentManager.h"
#include "ModularGameplayActors/GSCModularPlayerState.h"

AGSCModularPlayerStateCharacter::AGSCModularPlayerStateCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilitySystemComponent* AGSCModularPlayerStateCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.IsValid() ? AbilitySystemComponent.Get() : nullptr;
}

void AGSCModularPlayerStateCharacter::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void AGSCModularPlayerStateCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
	Super::EndPlay(EndPlayReason);
}

void AGSCModularPlayerStateCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// For Player State ASC Pawns, initialize ASC on server in PossessedBy
	if (APlayerState* PS = GetPlayerState())
	{
		AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
		if (AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->InitAbilityActorInfo(PS, this);
			// TODO: Might consider sending GFC extension event in InitAbilityActorInfo instead
			UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);

			// Required for ability input binding to update itself when ability are granted again in case of a respawn
			UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(PS, UGameFrameworkComponentManager::NAME_GameActorReady);
		}
	}
}

void AGSCModularPlayerStateCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// For Player State ASC Pawns, initialize ASC on clients in OnRep_PlayerState
	if (APlayerState* PS = GetPlayerState())
	{
		AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PS);
		if (AbilitySystemComponent.IsValid())
		{
			AbilitySystemComponent->InitAbilityActorInfo(PS, this);
			UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, UGameFrameworkComponentManager::NAME_GameActorReady);
			
			// Required for ability input binding to update itself when ability are granted again in case of a respawn
			UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(PS, UGameFrameworkComponentManager::NAME_GameActorReady);
		}
	}
}

void AGSCModularPlayerStateCharacter::GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const
{
	if (AbilitySystemComponent.IsValid())
	{
		FGameplayTagContainer OwnedTags;
		AbilitySystemComponent->GetOwnedGameplayTags(OwnedTags);
		OutTagContainer = MoveTemp(OwnedTags);
	}
}

bool AGSCModularPlayerStateCharacter::HasMatchingGameplayTag(const FGameplayTag InTagToCheck) const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent->HasMatchingGameplayTag(InTagToCheck);
	}

	return false;
}

bool AGSCModularPlayerStateCharacter::HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent->HasAllMatchingGameplayTags(InTagContainer);
	}

	return false;
}

bool AGSCModularPlayerStateCharacter::HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const
{
	if (AbilitySystemComponent.IsValid())
	{
		return AbilitySystemComponent->HasAnyMatchingGameplayTags(InTagContainer);
	}

	return false;
}