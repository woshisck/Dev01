#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_MeleeAttack.generated.h"

/**
 * 通用近战攻击 GA（敌人 + 玩家均可使用）。
 *
 * 策划工作流：
 * 1. 新建 Blueprint GA，Parent Class 选 GA_MeleeAttack
 * 2. Class Defaults → AbilityTags 填写对应 Tag（须与角色 CharacterData.AbilityData 表格行 Key 一致）
 * 3. 玩家 GA：勾选 bRequireCommit，StatBeforeATKEffect 填写 GE_StatBeforeATK
 *    敌人 GA：保持默认（bRequireCommit=false，StatBeforeATKEffect 留空）
 * 4. 清空 Event Graph（所有流程已在 C++ 中实现）
 *
 * 蒙太奇和命中框自动从 角色.CharacterData.AbilityData.AbilityMap[AbilityTags[0]] 读取，
 * 无需在 GA 上单独配置。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_MeleeAttack : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_MeleeAttack();

	/**
	 * 是否在激活时调用 CommitAbility（扣除消耗 + 触发冷却）。
	 * 玩家近战 GA 填 true；敌人近战 GA 保持 false（默认）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	bool bRequireCommit = false;

	/**
	 * 激活时施加到 Self 的"攻击前摇"GameplayEffect（例如 GE_StatBeforeATK）。
	 * EndAbility 时自动移除，无需手动调用。
	 * 玩家近战 GA 填写；敌人 GA 留空。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSubclassOf<UGameplayEffect> StatBeforeATKEffect;

	/**
	 * 技能正常结束时（非 Cancel/Interrupt）施加的"攻击后摇"GameplayEffect（例如 GE_StatAfterATK）。
	 * 该 GE 自身带 Duration，自动到期，无需手动移除。
	 * 玩家近战 GA 填写；敌人 GA 留空。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Melee")
	TSubclassOf<UGameplayEffect> StatAfterATKEffect;

	/** 从 CharacterData.AbilityData.AbilityMap[AbilityTags[0]] 读取当前技能的 ActionData */
	virtual FActionData GetAbilityActionData_Implementation() const override;

protected:
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

private:
	/** GE_StatBeforeATK 的激活句柄，EndAbility 时自动移除 */
	FActiveGameplayEffectHandle StatBeforeATKHandle;

	UFUNCTION()
	void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);

	UFUNCTION()
	void OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData);
};
