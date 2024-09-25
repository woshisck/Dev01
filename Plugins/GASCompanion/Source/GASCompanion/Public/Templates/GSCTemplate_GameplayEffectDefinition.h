// Copyright 2020 Mickael Daniel. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GSCTemplate_GameplayEffectDefinition.generated.h"

/**
 * Parent class for Gameplay Effect Templates.
 *
 * Templates are one of special kind. They are only meant to be used to create other Gameplay Effect
 * based on their Class Default Object, and not meant to be used directly or inherited.
 *
 * These are not child of UGameplayEffect, but rather an UObject sharing the exact same properties as UGameplayEffect.
 *
 * This class exists only to allow creation of GE templates Blueprint, that can be configured in Project Settings, without
 * having them interfere with standard Gameplay Effects dropdown in properties and nodes like ApplyGameplayEffect.
 *
 * When a new GE is created from a template, a real UGameplayEffect Blueprint is created based on the properties defined by the template.
 */
UCLASS(Abstract, Blueprintable)
class GASCOMPANION_API UGSCTemplate_GameplayEffectDefinition : public UObject
{
	GENERATED_BODY()

public:
	UGSCTemplate_GameplayEffectDefinition();

	//~ Begin UObject interface
	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End UObject interface
	
	/** Policy for the duration of this effect */
	UPROPERTY(EditDefaultsOnly, Category=GameplayEffect)
	EGameplayEffectDurationType DurationPolicy;

	/** Duration in seconds. 0.0 for instantaneous effects; -1.0 for infinite duration. */
	UPROPERTY(EditDefaultsOnly, Category=GameplayEffect)
	FGameplayEffectModifierMagnitude DurationMagnitude;

	/** Period in seconds. 0.0 for non-periodic effects */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Period)
	FScalableFloat	Period;
	
	/** If true, the effect executes on application and then at every period interval. If false, no execution occurs until the first period elapses. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Period)
	bool bExecutePeriodicEffectOnApplication;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Period)
	EGameplayEffectPeriodInhibitionRemovedPolicy PeriodicInhibitionPolicy;

	/** Array of modifiers that will affect the target of this effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=GameplayEffect, meta=(TitleProperty=Attribute))
	TArray<FGameplayModifierInfo> Modifiers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffect)
	TArray<FGameplayEffectExecutionDefinition> Executions;

	/** Probability that this gameplay effect will be applied to the target actor (0.0 for never, 1.0 for always) */
	UPROPERTY(EditDefaultsOnly, Category=Application, meta=(GameplayAttribute="True"))
	FScalableFloat ChanceToApplyToTarget;

	UPROPERTY(EditDefaultsOnly, Category=Application, DisplayName="Application Requirement")
	TArray<TSubclassOf<UGameplayEffectCustomApplicationRequirement> > ApplicationRequirements;

	/** other gameplay effects that will be applied to the target of this effect if this effect applies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffect)
	TArray<FConditionalGameplayEffect> ConditionalGameplayEffects;

	/** Effects to apply when a stacking effect "overflows" its stack count through another attempted application. Added whether the overflow application succeeds or not. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Overflow)
	TArray<TSubclassOf<UGameplayEffect>> OverflowEffects;

	/** If true, stacking attempts made while at the stack count will fail, resulting in the duration and context not being refreshed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Overflow)
	bool bDenyOverflowApplication;

	/** If true, the entire stack of the effect will be cleared once it overflows */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Overflow, meta=(EditCondition="bDenyOverflowApplication"))
	bool bClearStackOnOverflow;

	/** Effects to apply when this effect is made to expire prematurely (like via a forced removal, clear tags, etc.); Only works for effects with a duration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Expiration)
	TArray<TSubclassOf<UGameplayEffect>> PrematureExpirationEffectClasses;

	/** Effects to apply when this effect expires naturally via its duration; Only works for effects with a duration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Expiration)
	TArray<TSubclassOf<UGameplayEffect>> RoutineExpirationEffectClasses;

	/** If true, cues will only trigger when GE modifiers succeed being applied (whether through modifiers or executions) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
	bool bRequireModifierSuccessToTriggerCues;

	/** If true, GameplayCues will only be triggered for the first instance in a stacking GameplayEffect. */
	UPROPERTY(EditDefaultsOnly, Category = Display)
	bool bSuppressStackingCues;

	/** Cues to trigger non-simulated reactions in response to this GameplayEffect such as sounds, particle effects, etc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
	TArray<FGameplayEffectCue>	GameplayCues;

	/** Data for the UI representation of this effect. This should include things like text, icons, etc. Not available in server-only builds. */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = Display)
	TObjectPtr<class UGameplayEffectUIData> UIData;

	// ----------------------------------------------------------------------
	//	Tag Containers
	// ----------------------------------------------------------------------
	
	/** The GameplayEffect's Tags: tags the the GE *has* and DOES NOT give to the actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta = (DisplayName = "GameplayEffectAssetTag", Categories="GameplayEffectTagsCategory"))
	FInheritedTagContainer InheritableGameplayEffectTags;
	
	/** "These tags are applied to the actor I am applied to" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta=(DisplayName="GrantedTags", Categories="OwnedTagsCategory"))
	FInheritedTagContainer InheritableOwnedTagsContainer;
	
	/** Once Applied, these tags requirements are used to determined if the GameplayEffect is "on" or "off". A GameplayEffect can be off and do nothing, but still applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta=(Categories="OngoingTagRequirementsCategory"))
	FGameplayTagRequirements OngoingTagRequirements;

	/** Tag requirements for this GameplayEffect to be applied to a target. This is pass/fail at the time of application. If fail, this GE fails to apply. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta=(Categories="ApplicationTagRequirementsCategory"))
	FGameplayTagRequirements ApplicationTagRequirements;

	/** Tag requirements that if met will remove this effect. Also prevents effect application. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta=(Categories="ApplicationTagRequirementsCategory"))
	FGameplayTagRequirements RemovalTagRequirements;

	/** GameplayEffects that *have* tags in this container will be cleared upon effect application. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta=(Categories="RemoveTagRequirementsCategory"))
	FInheritedTagContainer RemoveGameplayEffectsWithTags;

	/** Grants the owner immunity from these source tags.  */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Immunity, meta = (DisplayName = "GrantedApplicationImmunityTags", Categories="GrantedApplicationImmunityTagsCategory"))
	FGameplayTagRequirements GrantedApplicationImmunityTags;

	/** Grants immunity to GameplayEffects that match this query. Queries are more powerful but slightly slower than GrantedApplicationImmunityTags. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Immunity)
	FGameplayEffectQuery GrantedApplicationImmunityQuery;

	/** On Application of an effect, any active effects with this this query that matches against the added effect will be removed. Queries are more powerful but slightly slower than RemoveGameplayEffectsWithTags. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta = (DisplayAfter = "RemovalTagRequirements"))
	FGameplayEffectQuery RemoveGameplayEffectQuery;

	// ----------------------------------------------------------------------
	//	Stacking
	// ----------------------------------------------------------------------
	
	/** How this GameplayEffect stacks with other instances of this same GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingType	StackingType;

	/** Stack limit for StackingType */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	int32 StackLimitCount;

	/** Policy for how the effect duration should be refreshed while stacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy;

	/** Policy for how the effect period should be reset (or not) while stacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingPeriodPolicy StackPeriodResetPolicy;

	/** Policy for how to handle duration expiring on this gameplay effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingExpirationPolicy StackExpirationPolicy;

	// ----------------------------------------------------------------------
	//	Granted abilities
	// ----------------------------------------------------------------------
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Granted Abilities")
	TArray<FGameplayAbilitySpecDef>	GrantedAbilities;

	/** Copy over properties from TemplateCDO to GameplayEffect target CDO */
	static void CopyProperties(UGameplayEffect* CDO, UGSCTemplate_GameplayEffectDefinition* TemplateCDO);
};
