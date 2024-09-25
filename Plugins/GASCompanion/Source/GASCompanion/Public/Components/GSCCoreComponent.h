// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "UI/GSCUWHud.h"
#include "GSCCoreComponent.generated.h"

class UGSCGameplayAbility;
class UGameplayAbility;
class UGameplayEffect;
class UAbilitySystemComponent;
class UGSCAttributeSetBase;
struct FGameplayAbilitySpecHandle;

/** Structure passed down to Actors Blueprint with PostGameplayEffectExecute Event */
USTRUCT(BlueprintType)
struct FGSCGameplayEffectExecuteData
{
	GENERATED_BODY()

	/** The owner AttributeSet from which the event was invoked */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	TObjectPtr<UAttributeSet> AttributeSet = nullptr;

	/** The owner AbilitySystemComponent for this AttributeSet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

	/** Calculated DeltaValue from OldValue to NewValue */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	float DeltaValue = 0.f;

	/** The configured Clamp MinimumValue for this Attribute, if defined */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = AttributeSetPayload)
	float ClampMinimumValue = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSCOnDeath);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGSCOnInitAbilityActorInfoCore);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSCOnDefaultAttributeChange, float, DeltaValue, const struct FGameplayTagContainer, EventTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSCOnAttributeChange, FGameplayAttribute, Attribute, float, DeltaValue, const struct FGameplayTagContainer, EventTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSCOnPreAttributeChange, UGSCAttributeSetBase*, AttributeSet, FGameplayAttribute, Attribute, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FGSCOnPostGameplayEffectExecute, FGameplayAttribute, Attribute, AActor*, SourceActor, AActor*, TargetActor, const FGameplayTagContainer&, SourceTags, const FGSCGameplayEffectExecuteData, Payload);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSCOnAbilityActivated, const UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSCOnAbilityEnded, const UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSCOnAbilityFailed, const UGameplayAbility*, Ability, const FGameplayTagContainer&, ReasonTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FGSCOnGameplayEffectStackChange, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle, int32, NewStackCount, int32, OldStackCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSCOnGameplayEffectAdded, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSCOnGameplayEffectRemoved, FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGSCOnGameplayTagStackChange, FGameplayTag, GameplayTag, int32, NewTagCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGSCOnAbilityCommit, UGameplayAbility*, Ability);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FGSCOnCooldownChanged, UGameplayAbility*, Ability, const FGameplayTagContainer, CooldownTags, float, TimeRemaining, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSCOnCooldownEnd, UGameplayAbility*, Ability, FGameplayTag, CooldownTag, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FGSCOnDamage, float, DamageAmount, AActor*, SourceCharacter, const struct FGameplayTagContainer&, DamageTags);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FGSCOnGameplayEffectTimeChange,  FGameplayTagContainer, AssetTags, FGameplayTagContainer, GrantedTags, FActiveGameplayEffectHandle, ActiveHandle, float, NewStartTime, float, NewDuration);

/**
 * This Actor Component provides abstraction towards Ability System Component.
 * 
 * For PlayerStates characters, the Ability System Component is not directly accessible as the owner actor in this case is actually the Player State.
 *
 * Provides commonly shared functionality and API / Events for all Actors that have an Ability System Component (abstracting away Owner / Avatar actor setup for ASC)
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup=("GASCompanion"), meta=(BlueprintSpawnableComponent))
class GASCOMPANION_API UGSCCoreComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	// Sets default values for this component's properties
	UGSCCoreComponent();

	UPROPERTY()
	TObjectPtr<AActor> OwnerActor;

	UPROPERTY()
	TObjectPtr<APawn> OwnerPawn;

	UPROPERTY()
	TObjectPtr<ACharacter> OwnerCharacter;

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> OwnerAbilitySystemComponent;

	/** Setup GetOwner to character and sets references for ability system component and the owner itself. */
	void SetupOwner();

	/** Register Ability System delegates to mainly broadcast blueprint assignable event to BPs */
	void RegisterAbilitySystemDelegates(UAbilitySystemComponent* ASC);

	/** Clean up any bound delegates to Ability System delegates */
	void ShutdownAbilitySystemDelegates(UAbilitySystemComponent* ASC);

	// Called from AttributeSet, and trigger BP events
	virtual void HandleDamage(float DamageAmount, const FGameplayTagContainer& DamageTags, AActor* SourceActor);
	virtual void HandleHealthChange(float DeltaValue, const FGameplayTagContainer& EventTags);
	virtual void HandleStaminaChange(float DeltaValue, const FGameplayTagContainer& EventTags);
	virtual void HandleManaChange(float DeltaValue, const FGameplayTagContainer& EventTags);
	virtual void HandleAttributeChange(FGameplayAttribute Attribute, float DeltaValue, const FGameplayTagContainer& EventTags);
	
	/**
	 * Event called just after InitAbilityActorInfo on ASC, once abilities and attributes have been granted.
	 *
	 * This will happen multiple times for both client / server:
	 *
	 * - Once for Server after component initialization
	 * - Once for Server after replication of owning actor (Possessed by for Player State)
	 * - Once for Client after component initialization
	 * - Once for Client after replication of owning actor (Once more for Player State OnRep_PlayerState)
	 *
	 * Also depends on whether ASC lives on Pawns or Player States.
	 *
	 * Note: This event is also exposed on GSCAbilitySystemComponent itself, but Pawns using ASC on PlayerState might want
	 * to have the ability to react to this event from Pawns too.
	 */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnInitAbilityActorInfoCore OnInitAbilityActorInfo;

	/**
	* Called when character takes damage, which may have killed them
	*
	* @param DamageAmount Amount of damage that was done, not clamped based on current health
	* @param SourceCharacter The actual actor that did the damage
	* @param DamageTags The gameplay tags of the event that did the damage
	*/
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnDamage OnDamage;

    /**
    * Called when health is changed, either from healing or from being damaged
    * For damage this is called in addition to OnDamaged/OnDeath
    *
    * @param DeltaValue Change in health value, positive for heal, negative for cost. If 0 the delta is unknown
    * @param EventTags The gameplay tags of the event that changed mana
    */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnDefaultAttributeChange OnHealthChange;

    /**
    * Called when stamina is changed, either from regen  or from being used as a cost
    *
    * @param DeltaValue Change in stamina value, positive for heal, negative for cost. If 0 the delta is unknown
    * @param EventTags The gameplay tags of the event that changed mana
    */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnDefaultAttributeChange OnStaminaChange;


    /**
    * Called when mana is changed, either from healing or from being used as a cost
    *
    * @param DeltaValue Change in mana value, positive for heal, negative for cost. If 0 the delta is unknown
    * @param EventTags The gameplay tags of the event that changed mana
    */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnDefaultAttributeChange OnManaChange;

    /**
    * Called when any of the attributes owned by this character are changed
    *
    * @param Attribute The Attribute that was changed
    * @param DeltaValue It it was an additive operation, returns the modifier value.
    *                   Or if it was a change coming from damage meta attribute (for health),
    *                   returns the damage done.
    * @param EventTags The gameplay tags of the event that changed this attribute
    */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Abilities")
	FGSCOnAttributeChange OnAttributeChange;


	// Generic Attribute change callback for attributes
	virtual void OnAttributeChanged(const FOnAttributeChangeData& Data);

	// Generic "meta" Attribute change callback for damage attributes
	virtual void OnDamageAttributeChanged(const FOnAttributeChangeData& Data);

	/**
	* Called when character dies
	*/
	UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Actor")
	FGSCOnDeath OnDeath;

	/**
	* Triggers death events for the owner actor
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Actor")
	virtual void Die();

	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetHealth() const;

	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetMaxHealth() const;

	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetStamina() const;

	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetMaxStamina() const;

	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetMana() const;

	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetMaxMana() const;

	/** Returns the current value of an attribute (base value). That is, the value of the attribute with no stateful modifiers */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetAttributeValue(FGameplayAttribute Attribute) const;

	/** Returns current (final) value of an attribute */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual float GetCurrentAttributeValue(FGameplayAttribute Attribute) const;

	/** Returns whether the Actor is considered alive (Health > 0) */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual bool IsAlive() const;

	/**
	* Grants the Actor with the given ability, making it available for activation
	*
	* @param Ability The Gameplay Ability to give to the character
	* @param Level The Gameplay Ability Level (defaults to 1)
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GAS Companion|Abilities")
	virtual void GrantAbility(TSubclassOf<UGameplayAbility> Ability, int32 Level = 1);

	/** Remove an ability from the Character's ASC */

	/**
	* Remove an ability from the Actor's Ability System Component
	*
	* @param Ability The Gameplay Ability Class to remove
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GAS Companion|Abilities")
	virtual void ClearAbility(TSubclassOf<UGameplayAbility> Ability);

	/**
	* Remove an set of abilities from the Actor's Ability System Component
	*
	* @param Abilities Array of Ability Class to remove
	*/
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "GAS Companion|Abilities")
	virtual void ClearAbilities(TArray<TSubclassOf<UGameplayAbility>> Abilities);

	/**
	* Returns true if Actor's AbilitySystemComponent has any of the matching tags (expands to include parents of asset tags)
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|GameplayTags")
	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer TagContainer) const;

	/**
	* Returns true if Actor's AbilitySystemComponent has a gameplay tag that matches against the specified tag (expands to include parents of asset tags)
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|GameplayTags")
	virtual bool HasMatchingGameplayTag(const FGameplayTag TagToCheck) const;

	/** Returns whether one of the actors's active abilities are matching the provided Ability Class */
	UFUNCTION(BlueprintPure, Category="GAS Companion|Abilities")
    bool IsUsingAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass);

	/** Returns whether one of the character's active abilities are matching the provided tags */
	UFUNCTION(BlueprintPure, Category="GAS Companion|Abilities")
    bool IsUsingAbilityByTags(FGameplayTagContainer AbilityTags);

	/**
	* Returns a list of currently active ability instances that match the given class
	*
	* @param AbilityToSearch The Gameplay Ability Class to search for
	*/
	UFUNCTION(BlueprintCallable, Category="GAS Companion|Abilities")
	TArray<UGameplayAbility*> GetActiveAbilitiesByClass(TSubclassOf<UGameplayAbility> AbilityToSearch) const;

	/**
	* Returns a list of currently active ability instances that match the given tags
	*
	* This only returns if the ability is currently running
	*
	* @param GameplayTagContainer The Ability Tags to search for
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities")
	virtual TArray<UGameplayAbility*> GetActiveAbilitiesByTags(const FGameplayTagContainer GameplayTagContainer) const;

	/**
	* Attempts to activate the ability that is passed in. This will check costs and requirements before doing so.
	*
	* Returns true if it thinks it activated, but it may return false positives due to failure later in activation.
	*
	* @param AbilityClass Gameplay Ability Class to activate
	* @param ActivatedAbility The Gameplay Ability that was triggered on success (only returned if it is a GSCGameplayAbility)
	* @param bAllowRemoteActivation If true, it will remotely activate local/server abilities, if false it will only try to locally activate abilities.
	* @return bSuccess Returns true if it thinks it activated, but it may return false positives due to failure later in activation.
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities")
	virtual bool ActivateAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass, UGSCGameplayAbility*& ActivatedAbility, bool bAllowRemoteActivation = true);

	/**
	* Attempts to activate a **single** gameplay ability that matches the given tag and DoesAbilitySatisfyTagRequirements().
	*
	* It differs from GAS ASC TryActivateAbilitiesByTag which tries to activate *every* ability, whereas this version will pick a
	* random one and attempt to activate it.
	*
	* Returns true if the ability attempts to activate, and the reference to the Activated Ability if any.
	*
	* @param AbilityTags Set of Gameplay Tags to search for
	* @param ActivatedAbility The Gameplay Ability that was triggered on success (only returned if it is a GSCGameplayAbility)
	* @param bAllowRemoteActivation If true, it will remotely activate local/server abilities, if false it will only try to locally activate abilities.
	* @return bSuccess Returns true if it thinks it activated, but it may return false positives due to failure later in activation.
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Abilities")
	virtual bool ActivateAbilityByTags(const FGameplayTagContainer AbilityTags, UGSCGameplayAbility*& ActivatedAbility, const bool bAllowRemoteActivation = true);

	/* AttributeSet Events */

	/** Sets the base value of an attribute. Existing active modifiers are NOT cleared and will act upon the new base value. */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual void SetAttributeValue(FGameplayAttribute Attribute, float NewValue);

	/** Clamps the Attribute from MinValue to MaxValue */
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual void ClampAttributeValue(FGameplayAttribute Attribute, float MinValue, float MaxValue);

	/**
	* Helper function to proportionally adjust the value of an attribute when it's associated max attribute changes.
	* (e.g. When MaxHealth increases, Health increases by an amount that maintains the same percentage as before)
	*
	* @param AttributeSet The AttributeSet owner for the affected attributes
	* @param AffectedAttributeProperty The affected Attribute property
	* @param MaxAttribute The related MaxAttribute
	* @param NewMaxValue The new value for the MaxAttribute
	*/
	UFUNCTION(BlueprintCallable, Category = "GAS Companion|Attributes")
	virtual void AdjustAttributeForMaxChange(UPARAM(ref) UGSCAttributeSetBase* AttributeSet, const FGameplayAttribute AffectedAttributeProperty, const FGameplayAttribute MaxAttribute, float NewMaxValue);

	/* AttributeSet Events */

	virtual void PreAttributeChange(UGSCAttributeSetBase* AttributeSet, const FGameplayAttribute& Attribute, float NewValue);
	virtual void PostGameplayEffectExecute(UGSCAttributeSetBase* AttributeSet, const FGameplayEffectModCallbackData& Data);

	/**
	* PostGameplayEffectExecute event fired off from native AttributeSets, define here
	* any attribute change specific management you are not doing in c++ (like clamp)
	*
	* Only triggers after changes to the BaseValue of an Attribute from a GameplayEffect.
	*
	* @param Attribute The affected GameplayAttribute
	* @param SourceActor The instigator Actor that started the whole chain (in case of damage, that would be the damage causer)
	* @param TargetActor The owner Actor to which the Attribute change is applied
	* @param SourceTags The aggregated SourceTags for this EffectSpec
	* @param Payload Payload information with the original AttributeSet, the owning AbilitySystemComponent, calculated DeltaValue and the ClampMinimumValue from config if defined
	*/
	UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Attributes")
	FGSCOnPostGameplayEffectExecute OnPostGameplayEffectExecute;

	/**
	* PreAttributeChange event fired off from native AttributeSets, react here to
	* changes of Attributes CurrentValue before the modification to the BaseValue
	* happens.
	*
	* Called just before any modification happens to an attribute, whether using
	* Attribute setters or using GameplayEffect.
	*
	* @param AttributeSet The AttributeSet that started the change
	* @param Attribute The affected GameplayAttribute
	* @param NewValue The new value
	*/
	UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Attributes")
	FGSCOnPreAttributeChange OnPreAttributeChange;

	/**
	 * Notify component that ASC abilities have been granted or not.
	 *
	 * bStartupAbilitiesGranted is used to gate the triggering of some events like HealthChange or OnDeath.
	 */
	void SetStartupAbilitiesGranted(bool bGranted);

	// ~ Ability Events ~

    /**
    * Called when an ability is activated for the owner actor
    */
	UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Ability")
	FGSCOnAbilityActivated OnAbilityActivated;

    /**
    * Called when an ability is ended for the owner actor.
    */
	UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Ability")
	FGSCOnAbilityEnded OnAbilityEnded;

    /**
    * Called when an ability failed to activated for the owner actor, passes along the failed ability
    * and a tag explaining why.
    */
	UPROPERTY(BlueprintAssignable, Category = "GAS Companion|Ability")
	FGSCOnAbilityFailed OnAbilityFailed;

	/**
	* Called when a GameplayEffect is added or removed.
	*/
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnGameplayEffectStackChange OnGameplayEffectStackChange;

	/**
	* Called when a GameplayEffect duration is changed (for instance when duration is refreshed)
	*/
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnGameplayEffectTimeChange OnGameplayEffectTimeChange;

	/**
	* Called when a GameplayEffect is added.
	*/
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnGameplayEffectAdded OnGameplayEffectAdded;

	/**
	* Called when a GameplayEffect is removed.
	*/
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnGameplayEffectRemoved OnGameplayEffectRemoved;

	/** Called whenever a tag is added or removed (but not if just count is increased. Only for 'new' and 'removed' events) */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnGameplayTagStackChange OnGameplayTagChange;

	/** Called whenever an ability is committed (cost / cooldown are applied) */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnAbilityCommit OnAbilityCommit;

	/** Called when an ability with a valid cooldown is committed and cooldown is applied */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnCooldownChanged OnCooldownStart;

	/** Called when a cooldown gameplay tag is removed, meaning cooldown expired */
	UPROPERTY(BlueprintAssignable, Category="GAS Companion|Ability")
	FGSCOnCooldownEnd OnCooldownEnd;

protected:
	//~ Begin UActorComponent interface
	virtual void BeginPlay() override;
	//~ End UActorComponent interface

	//~ Begin UObject interface
	virtual void BeginDestroy() override;
	//~ End UObject interface

    bool bStartupAbilitiesGranted = false;

	/** Triggered by ASC when GEs are added */
	virtual void OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

	/** Triggered by ASC when GEs stack count changes */
	virtual void OnActiveGameplayEffectStackChanged(FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 PreviousStackCount);

	/** Triggered by ASC when GEs stack count changes */
	virtual void OnActiveGameplayEffectTimeChanged(FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration);

	/** Triggered by ASC when any GEs are removed */
	virtual void OnAnyGameplayEffectRemoved(const FActiveGameplayEffect& EffectRemoved);

	/** Trigger by ASC when any gameplay tag is added or removed (but not if just count is increased. Only for 'new' and 'removed' events) */
	void OnAnyGameplayTagChanged(FGameplayTag GameplayTag, int32 NewCount) const;

	/** Trigger by ASC when an ability is committed (cost / cooldown are applied)  */
	void OnAbilityCommitted(UGameplayAbility *ActivatedAbility);

	/** Trigger by ASC when a cooldown tag is changed (new or removed)  */
	virtual void OnCooldownGameplayTagChanged(const FGameplayTag GameplayTag, int32 NewCount, FGameplayAbilitySpecHandle AbilitySpecHandle, float Duration);

	/** Manage cooldown events trigger when an ability is committed */
	void HandleCooldownOnAbilityCommit(UGameplayAbility* ActivatedAbility);

private:
	/** Array of active GE handle bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FActiveGameplayEffectHandle> GameplayEffectAddedHandles;

	/** Array of tags bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FGameplayTag> GameplayTagBoundToDelegates;
};
