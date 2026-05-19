#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_RangeAttack.generated.h"

/**
 * Stub远程攻击 GA。
 * 与 UGA_MeleeAttack 平级（同样继承自 UYogGameplayAbility）。
 *
 * 目前仅作为占位，使 ComboGraph 节点可以通过 AttackType 选择
 * 触发近战 / 远程 GA。后续实现弹道、HitScan 或抛物线类技能时再补全。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_RangeAttack : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_RangeAttack();

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
