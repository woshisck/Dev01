// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MusketBulletDamageExecution.generated.h"

/**
 * 火绳枪子弹伤害执行计算。
 * 从 GE Spec 读取 SetByCaller(Attribute.ActDamage) 并叠加到目标 BaseAttributeSet.DmgTaken。
 */
UCLASS()
class DEVKIT_API UMusketBulletDamageExecution : public UGameplayEffectExecutionCalculation
{
    GENERATED_BODY()

public:
    UMusketBulletDamageExecution();

    virtual void Execute_Implementation(
        const FGameplayEffectCustomExecutionParameters& ExecutionParams,
        FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
