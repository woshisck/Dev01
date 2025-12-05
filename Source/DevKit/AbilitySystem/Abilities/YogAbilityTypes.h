// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "YogAbilityTypes.generated.h"

class UYogAbilitySystemComponent;
class UGameplayEffect;
class UYogTargetType;


/**
 * Struct defining a list of gameplay effects, a tag, and targeting info
 * These containers are defined statically in blueprints or assets and then turn into Specs at runtime
 */

USTRUCT(BlueprintType)
struct DEVKIT_API FYogEffectPorperty
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	TSubclassOf<UGameplayEffect> GameplayEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	int EffectLevel;
};


USTRUCT(BlueprintType)
struct DEVKIT_API FYogGameplayEffectContainer
{
	GENERATED_BODY()

public:
	FYogGameplayEffectContainer() {}

	/** Sets the way that type of target */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	TSubclassOf<UYogTargetType> TargetType;

	/** List of GE apply to the targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	TArray<FYogEffectPorperty> EffectClasses;
};

/** instance version -- struct FYogGameplayEffectContainer  */
USTRUCT(BlueprintType)
struct FYogGameplayEffectContainerSpec
{
	GENERATED_BODY()

public:
	FYogGameplayEffectContainerSpec() {}

	/** Computed target data */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	FGameplayAbilityTargetDataHandle TargetData;

	/** List of gameplay effects to apply to the targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayEffectContainer)
	TArray<FGameplayEffectSpecHandle> TargetGameplayEffectSpecs;

	/** Returns true if this has any valid effect specs */
	bool HasValidEffects() const;

	/** Returns true if this has any valid targets */
	bool HasValidTargets() const;

	/** Adds new targets to target data */
	void AddTargets(const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors);
};
