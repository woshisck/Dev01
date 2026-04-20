// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/GameplayEffect/GE_MusketBullet_Damage.h"
#include "AbilitySystem/GameplayEffect/MusketBulletDamageExecution.h"

UGE_MusketBullet_Damage::UGE_MusketBullet_Damage()
{
    DurationPolicy = EGameplayEffectDurationType::Instant;

    // 使用 ExecutionCalculation 代替 Modifier，
    // 从而规避 FGameplayEffectModifierMagnitude 受保护成员的访问限制。
    FGameplayEffectExecutionDefinition ExecDef;
    ExecDef.CalculationClass = UMusketBulletDamageExecution::StaticClass();
    Executions.Add(ExecDef);
}
