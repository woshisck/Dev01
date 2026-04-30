#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Animation/AN_MeleeDamage.h"
#include "Data/RuneDataAsset.h"
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

	/** 从缓存的 AN_MeleeDamage 读取当前技能的 ActionData（激活时缓存，结束时仍有效）*/
	virtual FActionData GetAbilityActionData_Implementation() const override;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	ECardRequiredAction GetCombatDeckActionType() const;

	UFUNCTION(BlueprintPure, Category = "Combat|Deck")
	bool IsCombatDeckComboFinisher() const;

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

	void SetNextActivationFromDashSave(bool bFromDashSave);

private:
	/** GE_StatBeforeATK 的激活句柄，EndAbility 时自动移除 */
	FActiveGameplayEffectHandle StatBeforeATKHandle;

	/**
	 * 激活时缓存蒙太奇第一个 AN_MeleeDamage，供 StatBeforeATK / GetAbilityActionData 使用。
	 * 不使用 UPROPERTY 是因为 AnimNotify 不参与 GC，生命周期随蒙太奇资产。
	 */
	TObjectPtr<UAN_MeleeDamage> CachedDamageNotify;

	/**
	 * 每次 OnEventReceived 时更新：记录最后一次命中触发的 AN_MeleeDamage。
	 * StatAfterATK 优先用此值（代表最后一击），未命中过时 fallback 到 CachedDamageNotify。
	 * EventData.OptionalObject 是 const UObject*，故用原始 const 指针；AnimNotify 不参与 GC。
	 */
	const UAN_MeleeDamage* LastFiredDamageNotify = nullptr;

	bool bCombatDeckCardResolvedThisActivation = false;
	bool bCombatDeckFromDashSave = false;
	bool bNextActivationFromDashSave = false;

	/** 扫描蒙太奇 Notifies 找到第一个 AN_MeleeDamage。*/
	static UAN_MeleeDamage* GetFirstDamageNotify(UAnimMontage* Montage);

	void TryResolveCombatDeckOnHit();

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
