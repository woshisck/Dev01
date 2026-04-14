#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "GA_Bleed.generated.h"

/**
 * 流血 GA（GameplayEvent 触发，FA 通过 Send Gameplay Event 传入伤害数值）
 *
 * 工作原理：
 *   - FA_Rune_Bleed 用 Apply Attribute Modifier 授予此 GA 并添加 Buff.Status.Bleeding Tag
 *   - FA 紧接着发送 Buff.Event.Bleed 事件（EventMagnitude = 每秒伤害量）激活此 GA
 *   - 激活后每隔 BleedTickInterval 秒扣一次血：每次扣血 = DamagePerSecond × BleedTickInterval
 *   - Buff.Status.Bleeding Tag 到期 → OnBleedingTagChanged 回调 → EndAbility
 *
 * FA 配置示例：
 *   [Apply Attribute Modifier] GrantedTagsToASC=Buff.Status.Bleeding, GrantedAbilities=GA_Bleed
 *       ↓ Out
 *   [Send Gameplay Event] EventTag=Buff.Event.Bleed, Magnitude=10.0（每秒伤害）
 *
 * 注意：
 *   BleedTick 内使用 ApplyModToAttributeUnsafe 直接扣 Health，
 *   会触发 HealthChanged 委托（进而发送 Action.HitReact 事件）。
 *   若不希望流血 Tick 播放受击动画，可在 YogCharacterBase::HealthChanged 添加：
 *     if (AbilitySystemComponent->HasMatchingGameplayTag(BleedingTag)) return;
 */
UCLASS()
class DEVKIT_API UGA_Bleed : public UYogGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Bleed(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** 扣血 Tick 间隔（秒）*/
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float BleedTickInterval = 0.5f;

	/**
	 * 默认每秒伤害（FA 未发送 EventMagnitude 时的 fallback）。
	 * 正常流程由 FA 的 Send Gameplay Event 节点的 Magnitude 覆盖此值。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bleed")
	float DefaultDamagePerSecond = 5.f;

private:
	/** 运行时每秒伤害，从 TriggerEventData.EventMagnitude 读取，fallback 为 DefaultDamagePerSecond */
	float DamagePerSecond = 0.f;

	FTimerHandle BleedTimerHandle;
	FDelegateHandle TagChangeDelegateHandle;

	/** 缓存发起人 ASC（用于伤害日志，指向玩家的 ASC） */
	TWeakObjectPtr<UYogAbilitySystemComponent> InstigatorASC;

	void BleedTick();
	void OnBleedingTagChanged(const FGameplayTag Tag, int32 NewCount);
};
