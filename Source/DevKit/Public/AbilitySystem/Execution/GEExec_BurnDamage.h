#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEExec_BurnDamage.generated.h"

/**
 * 燃烧伤害执行计算
 *
 * 每次周期性执行：
 *   BaseDamage SetByCaller（Tag: Data.Damage.Burn）读
 *   × (1 + 0.15 if 目标无护
 *   × (1 + 0.15 if 目标有流血 Buff.Bleed)
 *
 * 输出DamagePure 经过护甲吸收 扣血
 *
 * GE_Burn ExecutionCalculations 里引用此类
 * GE_Burn：HasDuration, Period=1s, GrantedTags=Buff.Fire
 * GE_Burn SetByCaller 槽：Tag=Data.Damage.Burn, Value=每秒基础伤害
 */
UCLASS()
class DEVKIT_API UGEExec_BurnDamage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UGEExec_BurnDamage();

	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
