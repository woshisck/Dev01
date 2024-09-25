// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "GSCModularPlayerState.generated.h"

class UGSCAbilitySystemComponent;

/** Minimal class that supports extension by game feature plugins */
UCLASS(Blueprintable)
class GASCOMPANION_API AGSCModularPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGSCModularPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	UPROPERTY(Category=PlayerState, VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UGSCAbilitySystemComponent> AbilitySystemComponent;

	//~ Begin AActor interface
	virtual void PreInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PostInitProperties() override;
	virtual void Reset() override;
	//~ End AActor interface

	//~ Begin IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End IAbilitySystemInterface

protected:
	//~ Begin APlayerState interface
	virtual void CopyProperties(APlayerState* PlayerState) override;
	//~ End APlayerState interface
};
