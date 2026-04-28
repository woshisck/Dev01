// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayEffect/MusketBulletDamageExecution.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "AbilitySystem/Attribute/DamageAttributeSet.h"

UMusketBulletDamageExecution::UMusketBulletDamageExecution()
{
    // 无需 Capture — 只输出 DamagePhysical 数值，不读取目标属性
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
        // 写入 DamagePhysical（DamageAttributeSet::PostGameplayEffectExecute 会扣血并广播 ReceiveDamage）
        // 不能写到 DmgTaken：那是受击倍率属性，Additive 会让目标永久变得更脆
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
            UDamageAttributeSet::GetDamagePhysicalAttribute(),
            EGameplayModOp::Override,
            Damage));
    }
}
