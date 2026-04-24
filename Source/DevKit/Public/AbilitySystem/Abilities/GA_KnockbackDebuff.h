#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_KnockbackDebuff.generated.h"

/**
 * 击退 Debuff GA（施加在目标身上）
 *
 * 目标受到攻击时，发送 Action.Knockback 事件激活 GA_Knockback（物理击退）。
 * 若目标有护甲：还对护甲施加 15% 额外伤害（通过 ArmorDamageEffect）。
 *
 * Blueprint 子类需配置：
 *   ArmorDamageEffect = 一个 Instant GE，用 SetByCaller 减少 ArmorHP（Tag: Data.Damage）
 */
UCLASS()
class DEVKIT_API UGA_KnockbackDebuff : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_KnockbackDebuff(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	/** 护甲额外伤害 GE（Instant，SetByCaller 减少 ArmorHP） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KnockbackDebuff")
	TSubclassOf<UGameplayEffect> ArmorDamageEffect;

	/** 护甲额外伤害比率（默认 0.15 = 15%，基于攻击伤害量） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "KnockbackDebuff")
	float ArmorDamagePct = 0.15f;

private:
	FDelegateHandle TagChangeDelegateHandle;

	UFUNCTION()
	void OnDamageTaken(FGameplayEventData Payload);

	void OnKnockbackDebuffTagChanged(const FGameplayTag Tag, int32 NewCount);
};
