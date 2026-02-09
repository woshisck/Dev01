// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/GameplayEffect/MontageDilationEffect.h"

UMontageDilationEffect::UMontageDilationEffect()
{
    FGameplayEffectExecutionDefinition ExecutionDefinition;

    // Set as duration-based
    //DurationPolicy = EGameplayEffectDurationType::HasDuration;

    //// Set duration to 1 second
    //FSetByCallerFloat Duration;
    //Duration.DataName = FGameplayTag::RequestGameplayTag("Data.Duration");
    //Duration.Value = 1.0f;
    //DurationPolicy = EGameplayEffectDurationType::HasDuration;
    //DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);

    // Add slow tag
    
    //InheritableGameplayEffectTags.Added.AddTag(
    //    FGameplayTag::RequestGameplayTag("Effect.Slow.MontageRate")
    //);
}
