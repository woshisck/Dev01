// Copyright 2022 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameFramework/DamageType.h"
#include "ComboGraphRuntimeTypes.generated.h"

class UNiagaraSystem;
class UComboGraphNodeAnimBase;
class UComboGraph;
class UComboGraphNodeBase;
class UFXSystemAsset;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EComboGraphIconType : uint8
{
	PS4,
	PS5,
	XboxOne,
	XboxSeries,
	Keyboard
};

UENUM(BlueprintType)
enum class EComboGraphTransitionBehavior : uint8
{
	/** Trigger combo transition immediately, as soon as input trigger is registered */
	Immediately UMETA(DisplayName = "Transition Immediately"),

	/**
	 * Trigger combo transition based on a specific Anim Notify.
	 *
	 * By the time an input trigger is received, combo transition will happen when the animation
	 * reaches this notify state or immediately if animation current time frame is passed the notify state.
	 */
	OnAnimNotifyClass UMETA(DisplayName = "Transition on a specific Anim Notify frame"),

	/**
	 * Trigger combo transition based on an Anim Notify Name (skeleton notify).
	 *
	 * By the time an input trigger is received, combo transition will happen when the animation
	 * reaches this notify state or immediately if animation current time frame is passed the notify state.
	 */
	OnAnimNotifyName UMETA(DisplayName = "Transition on a specific Skeleton Notify frame"),

	/** Trigger combo transition when combo window notify state ends */
	OnComboWindowEnd UMETA(DisplayName = "Wait for Combo Window End"),
};

/** Enum listing all possible ability activation input trigger event. */
UENUM(BlueprintType)
enum class EComboGraphTransitionInputEvent : uint8
{
	/**
	 * The most common trigger types are likely to be Started for actions that happen once, immediately upon pressing a button.
	 *
	 * This is the event to use for Input Action with default configuration (no triggers) or at least one trigger with the "Down" value.
	 */
	Started UMETA(DisplayName = "Activate on Action Started"),

	/**
	 * Triggered for continuous actions after one or more processing ticks.
	 *
	 * For action triggers that are triggered every frame, only the first trigger happening within a combo window will be considered.
	 */
	Triggered UMETA(DisplayName = "Activate on Action Triggered"),

	/**
	 * Triggered when input action has been canceled (eg. not fulfilling the trigger configuration). For instance, an hold and release trigger where hold time threshold has not been satisfied.
	 */
	Canceled UMETA(DisplayName = "Activate on Action Canceled"),
};

USTRUCT(BlueprintType)
struct FComboGraphNotifyStateAutoSetup
{
	GENERATED_BODY()

	FComboGraphNotifyStateAutoSetup() {}

	FComboGraphNotifyStateAutoSetup(const float StartPercent, const float EndPercent)
		: StartPercent(StartPercent),
		  EndPercent(EndPercent)
	{
	}

	/** At which point in the animation sequence or montage this notifier should start (ex. for 1s sequence length, a value of 0.25 notify begin would trigger at 0.25s)*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Auto Setup",  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StartPercent = 0.25f;

	/** At which point in the animation sequence or montage this notifier should end (ex. for 1s sequence length, a value of 0.75 notify begin would trigger at 0.75s)*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Auto Setup",  meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float EndPercent = 0.75f;
};

/**
 * Class that is used to determine targeting for abilities
 *
 * It is meant to be blueprinted to run target logic
 *
 * This does not subclass GameplayAbilityTargetActor because this class is never instanced into the world
 *
 * This can be used as a basis for a game-specific targeting blueprint. If your targeting is more complicated
 * you may need to instance into the world once or as a pooled actor
 */
UCLASS(Blueprintable, meta = (ShowWorldContextPin))
class COMBOGRAPH_API UComboGraphTargetType : public UObject
{
	GENERATED_BODY()

public:
	// Constructor and overrides
	UComboGraphTargetType() {}

	/** Called to determine targets to apply gameplay effects to */
	UFUNCTION(BlueprintNativeEvent)
	void GetTargets(AActor* TargetingActor, FGameplayEventData EventData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const;
};

UENUM(BlueprintType)
enum class EComboGraphCueSourceObjectType : uint8
{
	None,

	Niagara UMETA(DisplayName="Use Niagara Particle System"),
	Cascade UMETA(DisplayName="Use Cascade Particle System"),
	Sound UMETA(DisplayName="Use Sound Effect"),
};

/**
 * Struct defining a list of gameplay effects, a tag, and targeting info
 *
 * These containers are defined statically in blueprints or assets and then turn into Specs at runtime
 */
USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboGraphGameplayEffectContainer
{
	GENERATED_BODY()

public:
	FComboGraphGameplayEffectContainer() {}

	/** Sets the way that targeting happens */
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TSubclassOf<UComboGraphTargetType> TargetType;

	/** List of gameplay effects to apply to the targets */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TArray<TSubclassOf<UGameplayEffect>> TargetGameplayEffectClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	bool bUseSetByCallerMagnitude = false;

	// Edit Condition Hides may cause crashes in 4.26 (https://issues.unrealengine.com/issue/UE-117661)

	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer, meta = (EditCondition = "bUseSetByCallerMagnitude"))
	// FGameplayTag SetByCallerDataTag;
	//
	// UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer, meta = (EditCondition = "bUseSetByCallerMagnitude"))
	// float SetByCallerMagnitude = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer, meta = (EditCondition = "bUseSetByCallerMagnitude", EditConditionHides))
	FGameplayTag SetByCallerDataTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer, meta = (EditCondition = "bUseSetByCallerMagnitude", EditConditionHides))
	float SetByCallerMagnitude = 1.0f;
};

/** A "processed" version of GSCGameplayEffectContainer that can be passed around and eventually applied */
USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboGraphGameplayEffectContainerSpec
{
	GENERATED_BODY()

public:
	FComboGraphGameplayEffectContainerSpec() {}

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

/**
 * Unreal's Damage System Container fallback.
 *
 * Struct defining a list of gameplay effects, a tag, and targeting info
 *
 */
USTRUCT(BlueprintType)
struct COMBOGRAPH_API FComboGraphDamageSystemContainer
{
	GENERATED_BODY()

public:
	FComboGraphDamageSystemContainer()
	{
	}

	/** Amount of Damage to pass down to ApplyPointDamage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	float BaseDamage = 0.f;

	/** Damage Type to use with ApplyPointDamage */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
	TSubclassOf<UDamageType> DamageTypeClass;
};
