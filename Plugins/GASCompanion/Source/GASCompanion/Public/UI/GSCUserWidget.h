// Copyright 2021 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "GSCUserWidget.generated.h"

class UGSCCoreComponent;

USTRUCT(BlueprintType)
struct FGSCGameplayEffectUIData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS Companion|GameplayEffect")
	float StartTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS Companion|GameplayEffect")
	float TotalDuration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS Companion|GameplayEffect")
	float ExpectedEndTime;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS Companion|GameplayEffect")
	int32 StackCount;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS Companion|GameplayEffect")
	int32 StackLimitCount;

	FGSCGameplayEffectUIData(const float StartTime, const float TotalDuration, const float ExpectedEndTime, const int32 StackCount, const int32 StackLimitCount)
		: StartTime(StartTime),
		  TotalDuration(TotalDuration),
		  ExpectedEndTime(ExpectedEndTime),
		  StackCount(StackCount),
		  StackLimitCount(StackLimitCount)
	{
	}

	FGSCGameplayEffectUIData(): StartTime(0), TotalDuration(0), ExpectedEndTime(0), StackCount(0), StackLimitCount(0)
	{
	}
};

/**
 * Base user widget class to inherit from for UMG that needs to interact with an Ability System Component.
 *
 * For typical use case of Player HUD, use UGSCUWHud subclass instead.
 *
 * This class provides an event based API to react to Ability System related events:
 *
 * - Attribute value change
 * - Gameplay Tag change
 * - Gameplay Effect added / removed
 * - Cooldown start / expiration
 */
UCLASS()
class GASCOMPANION_API UGSCUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="GAS Companion|UI", meta=(DeprecatedFunction, DeprecationMessage="Use GetOwningActor() instead."))
	TObjectPtr<AActor> OwnerActor;

	UPROPERTY(BlueprintReadOnly, Category="GAS Companion|UI", meta=(DeprecatedFunction, DeprecationMessage="Use GetOwningCoreComponent() instead."))
	TObjectPtr<UGSCCoreComponent> OwnerCoreComponent;
	
	/** Initialize or update references to owner actor and additional actor components (such as AbilitySystemComponent) and cache them for this instance of user widget. */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual void SetOwnerActor(AActor* Actor);

	/** Returns reference to OwnerActor for this user widget, if it has been initialized. */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual AActor* GetOwningActor() const { return OwnerActor; }

	/** Returns reference to OwnerCoreComponent for this user widget, if it has been initialized. */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual UGSCCoreComponent* GetOwningCoreComponent() const { return OwnerCoreComponent; }

	/** Returns reference to AbilitySystemComponent for this user widget, if it has been initialized. */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual UAbilitySystemComponent* GetOwningAbilitySystemComponent() const { return AbilitySystemComponent; }

	/**
	 * Runs initialization logic for this UserWidget related to interactions with Ability System Component.
	 *
	 * Setup AbilitySystemComponent delegates to react to various events.
	 *
	 * Usually called from NativeConstruct, but exposed there if other classes needs to run initialization logic again to update references.
	 */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual void InitializeWithAbilitySystem(UPARAM(ref) const UAbilitySystemComponent* AbilitySystemComponent);

	/** Clears off any ASC delegates and dispose AbilitySystemComponent pointer */
	void ResetAbilitySystem();
	
	/** Register listeners for AbilitySystemComponent (Attributes, GameplayEffects / Tags, Cooldowns, ...) */
	virtual void RegisterAbilitySystemDelegates();
	
	/** Clear all delegates for AbilitySystemComponent bound to this UserWidget */
	virtual void ShutdownAbilitySystemComponentListeners() const;

	/**
	 * Event triggered when this widget has been initialized with a valid ASC.
	 * 
	 * Run initialization logic that depends on Ability System here. (like setting initial state for user widgets based on attributes value)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnAbilitySystemInitialized();

	/** Event triggered whenever an attribute value is changed on Owner Actor's ASC */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnAttributeChange(FGameplayAttribute Attribute, float NewValue, float OldValue);
	
	/** Event triggered by Companion Core Component whenever a gameplay effect is added / removed */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnGameplayEffectStackChange(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 OldStackCount);

	/** Event triggered by Companion Core Component whenever a gameplay effect time is changed (for instance when duration is refreshed) */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnGameplayEffectTimeChange(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration);

	/** Event triggered by Companion Core Component whenever a gameplay effect is added */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnGameplayEffectAdded(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle, FGSCGameplayEffectUIData EffectData);

	/** Event triggered by Companion Core Component whenever a gameplay effect is removed */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnGameplayEffectRemoved(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle, FGSCGameplayEffectUIData EffectData);

	/** Event triggered by Companion Core Component whenever a tag is added or removed (but not if just count is increased. Only for 'new' and 'removed' events) */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnGameplayTagChange(FGameplayTag GameplayTag, int32 NewTagCount);

	/** Event triggered by Companion Core component when an ability with a valid cooldown is committed and cooldown is applied */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnCooldownStart(UGameplayAbility* Ability, const FGameplayTagContainer CooldownTags, float TimeRemaining, float Duration);

	/** Event triggered by Companion Core Component when a cooldown gameplay tag is removed, meaning cooldown expired */
	UFUNCTION(BlueprintImplementableEvent, Category = "GAS Companion|UI")
	void OnCooldownEnd(UGameplayAbility* Ability, FGameplayTag CooldownTag, float Duration);

	/** Helper function to return percentage from Attribute / MaxAttribute */
	UFUNCTION(BlueprintPure, Category="GAS Companion|UI")
	float GetPercentForAttributes(FGameplayAttribute Attribute, FGameplayAttribute MaxAttribute) const;

	/**
	 * Returns the current value of an attribute (final value) from owning actor Ability System, if it has the attribute set.
	 *
	 * Returns 0 otherwise.
	 */
	UFUNCTION(BlueprintCallable, Category="GAS Companion|UI")
	virtual float GetAttributeValue(const FGameplayAttribute Attribute) const;

	/** Triggered by ASC and handle / broadcast Attributes change */
	virtual void OnAttributeChanged(const FOnAttributeChangeData& Data);

	/** Triggered by ASC when GEs are added */
	virtual void OnActiveGameplayEffectAdded(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

	/** Triggered by ASC when GEs stack count changes */
	virtual void OnActiveGameplayEffectStackChanged(FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 PreviousStackCount);

	/** Triggered by ASC when GEs stack count changes */
	virtual void OnActiveGameplayEffectTimeChanged(FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration);

	/** Triggered by ASC when any GEs are removed */
	virtual void OnAnyGameplayEffectRemoved(const FActiveGameplayEffect& EffectRemoved);

	/** Trigger by ASC when any gameplay tag is added or removed (but not if just count is increased. Only for 'new' and 'removed' events) */
	virtual void OnAnyGameplayTagChanged(FGameplayTag GameplayTag, int32 NewCount);

	/** Trigger by ASC when an ability is committed (cost / cooldown are applied) */
	virtual void OnAbilityCommitted(UGameplayAbility *ActivatedAbility);

	/** Trigger by ASC when a cooldown tag is changed (new or removed)  */
	virtual void OnCooldownGameplayTagChanged(const FGameplayTag GameplayTag, int32 NewCount, FGameplayAbilitySpecHandle AbilitySpecHandle, float Duration);

	/** Post attribute change hook for subclass that needs further handling */
	virtual void HandleAttributeChange(FGameplayAttribute Attribute, float NewValue, float OldValue) {}

	/** Trigger from ASC whenever a gameplay effect is added or removed */
	virtual void HandleGameplayEffectStackChange(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle, int32 NewStackCount, int32 OldStackCount);

	/** Trigger from ASC whenever a gameplay effect time is changed */
	virtual void HandleGameplayEffectTimeChange(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle, float NewStartTime, float NewDuration);

	/** Trigger from ASC whenever a gameplay effect is added */
	virtual void HandleGameplayEffectAdded(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle);

	/** Trigger from ASC whenever a gameplay effect is removed */
	virtual void HandleGameplayEffectRemoved(FGameplayTagContainer AssetTags, FGameplayTagContainer GrantedTags, FActiveGameplayEffectHandle ActiveHandle);

	/** Trigger from ASC whenever a gameplay tag is added or removed */
	virtual void HandleGameplayTagChange(FGameplayTag GameplayTag, int32 NewTagCount);

	/** Trigger from ASC whenever an ability with valid cooldown tags is committed (cooldown start) */
	virtual void HandleCooldownStart(UGameplayAbility* Ability, const FGameplayTagContainer CooldownTags, float TimeRemaining, float Duration);

	/** Trigger from ASC whenever an cooldown tag stack changes, and stack count is 0 (cooldown end) */
	virtual void HandleCooldownEnd(UGameplayAbility* Ability, FGameplayTag CooldownTag, float Duration);
	
	FGSCGameplayEffectUIData GetGameplayEffectUIData(FActiveGameplayEffectHandle ActiveHandle);

protected:
	
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
private:
	
	/** Array of active GE handle bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FActiveGameplayEffectHandle> GameplayEffectAddedHandles;

	/** Array of tags bound to delegates that will be fired when the count for the key tag changes to or away from zero */
	TArray<FGameplayTag> GameplayTagBoundToDelegates;
};
