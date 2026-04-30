#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Data/AbilityData.h"
#include "Animation/AN_MeleeDamage.h"
#include "MontageAttackDataAsset.generated.h"

class URuneDataAsset;

/**
 * Attack parameters that can be shared by montage notifies and combo configs.
 *
 * This is the DA version of the fields that currently live on AN_MeleeDamage.
 * V1 keeps AN_MeleeDamage as the frame trigger, but lets it reference this DA
 * so different combo branches can reuse a montage while selecting different
 * damage, hitbox, events, and rune effects.
 */
UCLASS(BlueprintType, Blueprintable, DisplayName = "Montage Attack Data")
class DEVKIT_API UMontageAttackDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UMontageAttackDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActDamage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActRange = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActResilience = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attack")
	float ActDmgReduce = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitBox")
	TArray<FYogHitboxType> HitboxTypes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop")
	EHitStopMode HitStopMode = EHitStopMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Freeze", EditConditionHides, ClampMin = 0.0f, ClampMax = 0.3f))
	float HitStopFrozenDuration = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 0.0f, ClampMax = 0.5f))
	float HitStopSlowDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 0.01f, ClampMax = 1.0f))
	float HitStopSlowRate = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HitStop",
		meta = (EditCondition = "HitStopMode == EHitStopMode::Slow", EditConditionHides, ClampMin = 1.01f, ClampMax = 5.0f))
	float HitStopCatchUpRate = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Events")
	TArray<FGameplayTag> OnHitEventTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects")
	TArray<TObjectPtr<URuneDataAsset>> AdditionalRuneEffects;

	UFUNCTION(BlueprintPure, Category = "Attack")
	FActionData BuildActionData() const;
};
