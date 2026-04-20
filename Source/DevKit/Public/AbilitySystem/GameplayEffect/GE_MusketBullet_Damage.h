// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GE_MusketBullet_Damage.generated.h"

/**
 * 火绳枪子弹伤害 GE。
 * Duration=Instant，Modifier: BaseAttributeSet.DmgTaken Additive SetByCaller(Attribute.ActDamage)
 * 所有参数硬编码在构造函数中，无需创建 Blueprint 资产。
 */
UCLASS()
class DEVKIT_API UGE_MusketBullet_Damage : public UGameplayEffect
{
    GENERATED_BODY()

public:
    UGE_MusketBullet_Damage();
};
