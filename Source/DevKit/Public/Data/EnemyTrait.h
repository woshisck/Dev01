// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffectTypes.h"
#include "EnemyTrait.generated.h"

class AEnemyCharacterBase;

/**
 * Per-enemy bookkeeping for one trait. Trait assets are shared by every enemy that lists them,
 * so anything that differs per enemy lives here, on the enemy, instead of on the asset.
 */
struct DEVKIT_API FEnemyTraitRuntime
{
	int32 ActiveTier = INDEX_NONE;
	bool bRecountPending = false;
	FActiveGameplayEffectHandle TierEffectHandle;
	FGameplayTag ActiveCueTag;
	FDelegateHandle EnemyRegisteredHandle;
	FDelegateHandle EnemyUnregisteredHandle;
};

/**
 * A reusable behavior an enemy gets by listing the asset in UEnemyData::Traits.
 * Assets are stateless config: per-enemy state goes through AEnemyCharacterBase::FindOrAddTraitRuntime.
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class DEVKIT_API UEnemyTrait : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual void ActivateTrait(AEnemyCharacterBase* Owner) const;
	virtual void DeactivateTrait(AEnemyCharacterBase* Owner) const;

	/** Called when the owner's AI controller moves into Combat from another state. */
	virtual void OnOwnerEnteredCombat(AEnemyCharacterBase* /*Owner*/) const {}

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Trait", meta = (DisplayName = "On Activated"))
	void K2_OnActivated(AEnemyCharacterBase* Owner) const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Trait", meta = (DisplayName = "On Deactivated"))
	void K2_OnDeactivated(AEnemyCharacterBase* Owner) const;
};
