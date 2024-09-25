// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GSCTypes.h"
#include "Abilities/GameplayAbility.h"
#include "GSCGameplayAbility.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAbilityEnded);

/**
 * GameplayAbility Parent class that is recommended to use with GAS Companion.
 *
 * Added functionality compared to regular UGameplayAbility includes:
 *
 * - GameplayEffect containers: https://github.com/tranek/GASDocumentation#concepts-ge-containers
 * - Ability Queue System support: If you intend to use Ability Queueing, you should rely on this class for your Abilities
 * - Loosely Check for Cost: If you'd like your abilities to activate regardless of cost attribute going into negative values,
 * and only checking if cost attribute is not below or equal to 0 already.
 * - End delegate: A blueprint assignable delegate is exposed which is triggered on ability end. Particularly useful for AI Behavior Tree tasks.
 * - Activate On Granted: Support for "Passive" abilities, an ability that automatically activates and run continuously (eg. not calling EndAbility).
 */
UCLASS()
class GASCOMPANION_API UGSCGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UGSCGameplayAbility();

	/** if true, the ability can be activated even it the cost is going into negative values, preventing activation only if cost is below or equal to 0 already */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Ability")
	bool bLooselyCheckAbilityCost = false;

	/**
	 * If true, the ability will be automatically activated as soon as it is granted.
	 *
	 * You can either implement one-off Abilities that are meant to be activated right away when granted,
	 * or "Passive Abilities" with this, an ability that automatically activates and run continuously (eg. not calling EndAbility)
	 *
	 * In both case, GameplayAbilities configured to be activated on granted will only activate on server and typically have
	 * a Net Execution Policy of Server Only.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Ability")
	bool bActivateOnGranted  = false;

	/**
	 * Enable other abilities to be queued and activated when this ability ends.
	 *
	 * It is recommended to leave this variable to false, and instead use the AbilityQueueNotifyState (AbilityQueueWindow)
	 * within montages to further tune when the Queue System is opened and closed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS Companion|Ability")
	bool bEnableAbilityQueue = false;

    /** Map of gameplay tags to gameplay effect containers */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffects)
    TMap<FGameplayTag, FGSCGameplayEffectContainer> EffectContainerMap;

    /** Make gameplay effect container spec to be applied later, using the passed in container */
    UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability", meta=(AutoCreateRefTerm = "EventData"))
    virtual FGSCGameplayEffectContainerSpec MakeEffectContainerSpecFromContainer(const FGSCGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

    /** Search for and make a gameplay effect container spec to be applied later, from the EffectContainerMap */
    UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability", meta = (AutoCreateRefTerm = "EventData"))
    virtual FGSCGameplayEffectContainerSpec MakeEffectContainerSpec(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

    /** Applies a gameplay effect container spec that was previously created */
    UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability")
    virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainerSpec(const FGSCGameplayEffectContainerSpec& ContainerSpec);

    /** Applies a gameplay effect container, by creating and then applying the spec */
    UFUNCTION(BlueprintCallable, Category = "GAS Companion|Ability", meta = (AutoCreateRefTerm = "EventData"))
    virtual TArray<FActiveGameplayEffectHandle> ApplyEffectContainer(FGameplayTag ContainerTag, const FGameplayEventData& EventData, int32 OverrideGameplayLevel = -1);

	//~Begin UGameplayAbility interface

	/** To handle "passive" abilities, activating abilities marked as "Activate On Granted" automatically */
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

    /**
	 * Checks cost. returns true if we can pay for the ability. False if not
	 *
	 * If the Ability is set to ignore ability cost via bIgnoreAbilityCost, returns true
	 */
    virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const override;

	/**
	 * Returns true if this ability can be activated right now. Has no side effects
	 *
	 * This optionally loose Cost check if the ability is marked as ignore cost,
	 * meaning cost attributes are only checked to be < 0 and prevented if 0 or below.
	 *
	 * If Blueprints implements the CanActivateAbility function, they are responsible for ability activation or not
	 */
    virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	//~End UGameplayAbility interface

    // Gameplay Abilities Delegates

	/**
	 * Called when the ability ends.
	 */
    UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Ability")
    FOnAbilityEnded OnAbilityEnded;

	/** Called on ability end */
	void AbilityEnded(UGameplayAbility* Ability);

protected:

	//~Begin UGameplayAbility interface
	/**
	 * Overrides Ability pre activation, namely to open queue system if the ability is flagged as is
	 */
    virtual void PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate, const FGameplayEventData* TriggerEventData) override;
	//~End UGameplayAbility interface

private:

	/** Loosely Check for cost attribute current value to be positive */
	bool CheckForPositiveCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, OUT FGameplayTagContainer* OptionalRelevantTags) const;

	/** Does the actual check for attribute modifiers, only checking it their current value is <= 0 */
	bool CanApplyPositiveAttributeModifiers(const UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo* ActorInfo, float Level, const FGameplayEffectContextHandle& EffectContext) const;

	/** Returns spawned attribute set from passed in ASC based on provided AttributeClass (mainly because GetAttributeSuboject on AbilitySystemComponent is protected) */
    static const UAttributeSet* GetAttributeSubobjectForASC(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UAttributeSet> AttributeClass);
};
