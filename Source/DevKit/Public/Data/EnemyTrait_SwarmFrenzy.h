// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/EnemyTrait.h"
#include "GameplayTagContainer.h"
#include "EnemyTrait_SwarmFrenzy.generated.h"

class UGameplayEffect;

UENUM(BlueprintType)
enum class EEnemyTraitCountFilter : uint8
{
	/** Same EnemyId; falls back to the same EnemyData asset when EnemyId is None. */
	SameEnemyId,
	/** Same FactionTag as the owner. */
	FactionTag,
	AnyEnemy,
};

USTRUCT(BlueprintType)
struct DEVKIT_API FEnemyFrenzyTier
{
	GENERATED_BODY()

	/** Tier is active while the counted enemies are at least this many. The highest matching tier wins. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy", meta = (ClampMin = "1"))
	int32 MinCount = 5;

	/** Applied while this tier is active. Use an Infinite, Stack Limit 1 effect. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy")
	TSubclassOf<UGameplayEffect> Effect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy", meta = (Categories = "GameplayCue"))
	FGameplayTag CueTag;

	/**
	 * Enters Combat and sends Enemy.Event.Frenzy when this tier is entered, so the StateTree can jump to Attack.
	 * The Combat pin itself applies for as long as any tier is active.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy")
	bool bForceAttackOnEnter = true;
};

/**
 * While enough matching enemies are alive, the owner is frenzied: it is pinned to the Combat AI state
 * and the StateTree is told to jump to its Attack state. Recounts on every enemy spawn and death.
 */
UCLASS()
class DEVKIT_API UEnemyTrait_SwarmFrenzy : public UEnemyTrait
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy")
	TArray<FEnemyFrenzyTier> Tiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy")
	EEnemyTraitCountFilter CountFilter = EEnemyTraitCountFilter::SameEnemyId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frenzy")
	bool bCountSelf = true;

	UEnemyTrait_SwarmFrenzy();

	virtual void ActivateTrait(AEnemyCharacterBase* Owner) const override;
	virtual void DeactivateTrait(AEnemyCharacterBase* Owner) const override;

	void Recount(AEnemyCharacterBase* Owner) const;

private:
	void ScheduleRecount(AEnemyCharacterBase* Owner) const;
	void SetTier(AEnemyCharacterBase* Owner, FEnemyTraitRuntime& Runtime, int32 NewTier) const;
};
