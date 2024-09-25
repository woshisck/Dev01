// Copyright 2020 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"
#include "GameFramework/Actor.h"
#include "GSCModularActor.generated.h"

class UGSCAbilitySystemComponent;
/** Minimal class that supports extension by game feature plugins, direct child of AActor */
UCLASS()
class GASCOMPANION_API AGSCModularActor : public AActor, public IAbilitySystemInterface, public IGameplayTagAssetInterface
{
	GENERATED_BODY()

public:
	AGSCModularActor(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	* Ability System Replication Mode: How gameplay effects will be replicated to clients
	*
	* - Full: Replicate full gameplay info to all. Every GameplayEffect is replicated to every client.
	* (Recommended for Single Player games)
	* - Mixed: Only replicate minimal gameplay effect info to simulated proxies but full info to owners and autonomous proxies.
	* GameplayEffects are only replicated to the owning client. Only GameplayTags and GameplayCues are replicated to everyone.
	* (Recommended for Multiplayer on Player controlled Actors)
	* - Minimal: Only replicate minimal gameplay effect info. Note: this does not work for Owned AbilitySystemComponents (Use Mixed instead).
	* GameplayEffects are never replicated to anyone. Only GameplayTags and GameplayCues are replicated to everyone.
	* (Recommended for Multiplayer on AI controlled Actors)
	*
	* @See https://github.com/tranek/GASDocumentation#concepts-asc-rm for more information
	*/
	UPROPERTY(EditDefaultsOnly, Category="GAS Companion|Ability System")
	EGameplayEffectReplicationMode ReplicationMode = EGameplayEffectReplicationMode::Mixed;

	UPROPERTY(Category=Pawn, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UGSCAbilitySystemComponent> AbilitySystemComponent;

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitProperties() override;
	//~ End AActor interface

	//~ Begin IGameplayTagAssetInterface
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& OutTagContainer) const override;
	virtual bool HasMatchingGameplayTag(FGameplayTag InTagToCheck) const override;
	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const override;
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& InTagContainer) const override;
	//~ End IGameplayTagAssetInterface
};
