#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEExec_PoisonDamage.generated.h"

/**
 * 中毒伤害执行计算
 *
 * 每次周期性执行：
 *   1. 对 Health 造成 MaxHealth × 7% 的伤害（不至死，最低保留 1HP）
 *   2. 若目标有护甲（Buff.Status.Armored）：额外对 ArmorHP 造成 MaxHealth × 25% 的伤害
 *
 * 输出到 DamageBuff（绕过护甲吸收，由 DamageAttributeSet 直接扣血）
 * 和 ArmorHP（直接扣护甲）。
 *
 * 在 GE_Poison 的 ExecutionCalculations 数组里引用此类。
 * GE_Poison：HasDuration, Period=1s, GrantedTags=Buff.Status.Poisoned
 */
UCLASS()
class DEVKIT_API UGEExec_PoisonDamage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGEExec_PoisonDamage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
