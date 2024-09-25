// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTagContainer.h"
#include "GSCTypes.generated.h"

class UGSCTargetType;
class UGameplayEffect;

/** Enum listing all possible ability activation input trigger event. */
UENUM(BlueprintType)
enum class EGSCAbilityTriggerEvent : uint8
{
	/** The most common trigger types are likely to be Started for actions that happen once, immediately upon pressing a button. */
	Started UMETA(DisplayName="Activate on Action Started (recommended)"),

	/**
	 * Triggered for continuous actions that happen every frame while holding an input
	 *
	 * Warning: This value should only be used for Input Actions that you know only trigger once. If your action
	 * triggered event happens on every tick, this might lead to issues with ability activation (since you'll be
	 * trying to activate abilities every frame). When in doubt, use the default Started value.
	 */
	Triggered UMETA(DisplayName="Activate on Action Triggered (use with caution)"),
};

/**
* Struct defining a list of gameplay effects, a tag, and targeting info
*
* These containers are defined statically in blueprints or assets and then turn into Specs at runtime
*/
USTRUCT(BlueprintType)
struct FGSCGameplayEffectContainer
{
	GENERATED_BODY()

public:
	FGSCGameplayEffectContainer() {}

	/** Sets the way that targeting happens */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TSubclassOf<UGSCTargetType> TargetType;

	/** List of gameplay effects to apply to the targets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TArray<TSubclassOf<UGameplayEffect>> TargetGameplayEffectClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	bool bUseSetByCallerMagnitude = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	FGameplayTag SetByCallerDataTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	float SetByCallerMagnitude = 1.0f;
};

/** A "processed" version of GSCGameplayEffectContainer that can be passed around and eventually applied */
USTRUCT(BlueprintType)
struct GASCOMPANION_API FGSCGameplayEffectContainerSpec
{
	GENERATED_BODY()

public:
	FGSCGameplayEffectContainerSpec() {}

	/** Computed target data */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	FGameplayAbilityTargetDataHandle TargetData;

	/** List of gameplay effects to apply to the targets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TArray<FGameplayEffectSpecHandle> TargetGameplayEffectSpecs;

	/** Returns true if this has any valid effect specs */
	bool HasValidEffects() const;

	/** Returns true if this has any valid targets */
	bool HasValidTargets() const;

	/** Adds new targets to target data */
	void AddTargets(const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors);
};
