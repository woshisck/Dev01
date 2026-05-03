#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GEExec_PoisonDamage.generated.h"

/**
 * Periodic poison execution.
 *
 * Per tick:
 *   HealthDamage = MaxHealth * PercentPerStack * StackCount + Data.Damage.
 *   ArmorDamage  = MaxHealth * ArmorPercentPerStack * StackCount when target has Buff.Status.Armored.
 *
 * Defaults:
 *   PercentPerStack = 0.02 (2% max health per poison stack).
 *   ArmorPercentPerStack = 0.08.
 *
 * Optional SetByCaller inputs:
 *   Data.Damage: flat extra poison damage for this tick.
 *   Data.Poison.PercentPerStack: override health percent per stack.
 *   Data.Poison.ArmorPercentPerStack: override armor percent per stack.
 *
 * GE_Poison should be HasDuration, Period=1s, GrantedTags=Buff.Status.Poisoned.
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
