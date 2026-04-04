// Fill out your copyright notice in the Description page of Project Settings.

#include "Data/YogBuffDefinition.h"
#include "GameplayEffect.h"

UGameplayEffect* UYogBuffDefinition::CreateTransientGE(UObject* Outer) const
{
	UGameplayEffect* GE = NewObject<UGameplayEffect>(Outer ? Outer : GetTransientPackage(), FName(*FString::Printf(TEXT("BuffGE_%s"), *GetName())));

	GE->DurationPolicy = DurationPolicy;

	if (DurationPolicy == EGameplayEffectDurationType::HasDuration)
	{
		GE->DurationMagnitude = DurationMagnitude;
	}

	if (DurationPolicy != EGameplayEffectDurationType::Instant)
	{
		GE->Period = Period;
		GE->bExecutePeriodicEffectOnApplication = bExecutePeriodicEffectOnApplication;
		GE->PeriodicInhibitionPolicy = PeriodicInhibitionPolicy;
	}

	// Modifiers
	GE->Modifiers = Modifiers;

	// Executions
	GE->Executions = Executions;

	// Stacking
	GE->StackingType = StackingType;
	GE->StackLimitCount = StackLimitCount;
	GE->StackDurationRefreshPolicy = StackDurationRefreshPolicy;
	GE->StackPeriodResetPolicy = StackPeriodResetPolicy;
	GE->StackExpirationPolicy = StackExpirationPolicy;

	// Overflow
	GE->OverflowEffects = OverflowEffects;
	GE->bDenyOverflowApplication = bDenyOverflowApplication;
	GE->bClearStackOnOverflow = bClearStackOnOverflow;

	// Cues
	GE->bRequireModifierSuccessToTriggerCues = bRequireModifierSuccessToTriggerCues;
	GE->bSuppressStackingCues = bSuppressStackingCues;
	GE->GameplayCues = GameplayCues;

	// Tags granted to target while GE is active
	if (GrantedTagsToTarget.Num() > 0)
	{
		GE->InheritableOwnedTagsContainer.Added.AppendTags(GrantedTagsToTarget);
	}

	// BuffTag 加到 GE 自身 Asset Tags，便于通过 RemoveActiveEffectsWithTags 精确移除
	if (BuffTag.IsValid())
	{
		GE->InheritableGameplayEffectTags.Added.AddTag(BuffTag);
	}

	return GE;
}

