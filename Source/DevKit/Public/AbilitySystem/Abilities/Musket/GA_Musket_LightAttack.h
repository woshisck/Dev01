// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "GA_Musket_LightAttack.generated.h"

/**
 * 轻攻击速射 GA。
 *
 * 工作流：
 *   检查弹药 → 阻断移动 → 播放 FireMontage
 *   → 若 FireEventTag 有效则等 AnimNotify 事件，否则立即生成子弹
 *   → 蒙太奇结束 → 恢复移动 → 结束
 *
 * Blueprint 子类 Class Defaults 需填写（除基类字段外）：
 *   - FireMontage        轻攻击动画蒙太奇
 *   - FireEventTag       AnimNotify 发送的 GameplayEvent Tag（空则立即开枪）
 *   - DamageMultiplier   伤害倍率（默认 0.8）
 *   - HalfAngleDeg       速射随机散布半角（默认 15°）
 *
 * 取消换弹：构造函数中已设 CancelAbilitiesWithTag = Ability.Musket.Reload。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Musket_LightAttack : public UGA_MusketBase
{
    GENERATED_BODY()

public:
    UGA_Musket_LightAttack();

    /** 轻攻击动画蒙太奇（Blueprint 填写） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|LightAtk")
    TObjectPtr<UAnimMontage> FireMontage;

    /**
     * AnimNotify 发送的 GameplayEvent Tag，触发开枪。
     * 留空则在 ActivateAbility 时立即生成子弹（蒙太奇仅作视觉）。
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|LightAtk")
    FGameplayTag FireEventTag;

    /** 伤害倍率：Damage = BaseAttack × DamageMultiplier */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|LightAtk",
              meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float DamageMultiplier = 0.8f;

    /** 速射随机散布半角（±HalfAngleDeg 内均匀随机） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|LightAtk",
              meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float HalfAngleDeg = 15.f;

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
    bool bFired = false;

    void DoFire();

    UFUNCTION() void OnFireEvent(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnMontageComplete(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
