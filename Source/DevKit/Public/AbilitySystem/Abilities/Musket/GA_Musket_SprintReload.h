// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "GA_Musket_SprintReload.generated.h"

/**
 * 冲刺换弹 GA（冲刺中清空再补满弹药，不中断冲刺）。
 *
 * 工作流：
 *   ActivationRequiredTags = Buff.Status.DashInvincible（必须在冲刺状态）
 *   → ClearAllAmmo() → ExecuteReloadCue（弹夹掉落音效）
 *   → 等待 RefillDelay 秒（Timer）
 *   → SetAmmoToMax() → ExecuteReloadCue（补满音效）
 *   → 结束（冲刺 GA 继续运行）
 *
 * 此 GA 的 AbilityTags = Ability.Musket.SprintReload，
 * 不包含 PlayerState.AbilityCast，冲刺 GA 的 CancelAbilitiesWithTag 不会取消它。
 *
 * Blueprint 子类 Class Defaults 需填写：
 *   - RefillDelay   清空到补满之间的延迟（默认 0.3s，匹配冲刺动画中点）
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Musket_SprintReload : public UGA_MusketBase
{
    GENERATED_BODY()

public:
    UGA_Musket_SprintReload();

    /** 清空弹药到补满之间的延迟（秒） */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|SprintReload",
              meta = (ClampMin = "0.0", ClampMax = "2.0"))
    float RefillDelay = 0.3f;

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
    FTimerHandle RefillTimerHandle;

    void OnRefillTimer();
};
