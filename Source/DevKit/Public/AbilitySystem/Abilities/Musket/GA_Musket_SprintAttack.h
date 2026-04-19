// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "GA_Musket_SprintAttack.generated.h"

/**
 * 冲刺攻击 GA（全弹扇形射出，带击退）。
 *
 * 工作流：
 *   检查弹药 > 0，且处于冲刺状态（ActivationRequiredTags = Buff.Status.DashInvincible）
 *   → 取消当前冲刺（CancelAbilitiesWithTag = PlayerState.AbilityCast.Dash）
 *   → 立即将所有子弹按扇形均匀发射（含击退 GE）
 *   → CurrentAmmo = 0
 *   → 播放 SprintAtkMontage → 蒙太奇结束 → 结束
 *
 * 每颗子弹独立计算伤害，命中时对目标施加 KnockbackEffectClass。
 *
 * Blueprint 子类 Class Defaults 需填写：
 *   - SprintAtkMontage      冲刺攻击动画蒙太奇
 *   - KnockbackEffectClass  击退 GE（复用 GameplayCue.Character.KnockBack 已有 Cue）
 *   - HalfFanAngle          散射扇形半角（默认 30°）
 *   - DamageMultiplier      每颗子弹伤害倍率（默认 0.6）
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Musket_SprintAttack : public UGA_MusketBase
{
    GENERATED_BODY()

public:
    UGA_Musket_SprintAttack();

    /** 冲刺攻击动画蒙太奇 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|SprintAtk")
    TObjectPtr<UAnimMontage> SprintAtkMontage;

    /** 击退效果 GE（施加到命中目标） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|SprintAtk")
    TSubclassOf<UGameplayEffect> KnockbackEffectClass;

    /** 散射扇形半角（±度），子弹在 [-HalfFanAngle, +HalfFanAngle] 均匀分布 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|SprintAtk",
              meta = (ClampMin = "1.0", ClampMax = "90.0"))
    float HalfFanAngle = 30.f;

    /** 每颗子弹伤害倍率：Damage = BaseAttack × DamageMultiplier */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|SprintAtk",
              meta = (ClampMin = "0.1"))
    float DamageMultiplier = 0.6f;

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
    UFUNCTION() void OnMontageBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnMontageCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
