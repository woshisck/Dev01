// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayEffect/MusketBulletDamageExecution.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

UMusketBulletDamageExecution::UMusketBulletDamageExecution()
{
    // 无需 Capture — 只输出 DmgTaken 增量，不读取目标属性
}

void UMusketBulletDamageExecution::Execute_Implementation(
    const FGameplayEffectCustomExecutionParameters& ExecutionParams,
    FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
    static const FGameplayTag TAG_ActDamage =
        FGameplayTag::RequestGameplayTag(FName("Attribute.ActDamage"));

    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const float Damage = Spec.GetSetByCallerMagnitude(TAG_ActDamage, false, 0.f);

    if (Damage > 0.f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UBaseAttributeSet::GetDmgTakenAttribute(),
            EGameplayModOp::Additive,
            Damage));
    }
}
