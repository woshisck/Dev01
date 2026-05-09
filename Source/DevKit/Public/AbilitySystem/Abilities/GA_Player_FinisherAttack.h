#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_Player_FinisherAttack.generated.h"

class UYogAbilityTask_PlayMontageAndWaitForEvent;
class AYogCharacterBase;

/**
 * 终结技主 GA（状态机职责，不直接施加伤害）
 * - 激活方式：Action.Player.FinisherAttack GameplayEvent（由 AN_TriggerFinisherAbility 触发）
 * - 职责：蒙太奇播放、子弹时间控制、确认输入检测、遍历印记目标并派发引爆事件
 * - 伤害/击退/割裂由 FA_Finisher_Detonate 通过 WaitGameplayEvent(DetonateTarget) 处理
 *
 * Blueprint 必须配置：
 *   AbilityTags             = PlayerState.AbilityCast.Finisher
 *   ActivationOwnedTags     = Buff.Status.FinisherExecuting
 *   ActivationRequiredTags  = Buff.Status.FinisherWindowOpen
 *   ActivationBlockedTags   = Buff.Status.Dead, Buff.Status.FinisherExecuting
 *   CancelAbilitiesWithTag  = PlayerState.AbilityCast.LightAtk | HeavyAtk | Dash（排除 FinisherCharge）
 *   FinisherMontage         = AM_Player_FinisherAttack
 *
 * 蒙太奇 Notify 配置：
 *   [ANS_FinisherTimeDilation]  → 覆盖子弹时间区间
 *   [AN_MeleeDamage]            → 攻击判定帧，EventTag = Ability.Event.Finisher.HitFrame
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Player_FinisherAttack : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Player_FinisherAttack(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

    /** 终结技蒙太奇 */
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    TObjectPtr<UAnimMontage> FinisherMontage;

    /**
     * 玩家确认时的伤害倍率，写入 DetonateTarget 事件的 EventMagnitude
     * （未确认 = 1.0，确认 = 此值）
     * 伤害基础值在卡牌 DA 数值表中配置（Key: DetonationDamage）
     */
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    float ConfirmedDamageMultiplier = 2.0f;

private:
    UFUNCTION()
    void OnMontageEvent(FGameplayTag EventTag, FGameplayEventData EventData);

    UFUNCTION()
    void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

    UFUNCTION()
    void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

    void DetonateMarks(bool bConfirmed);

    void RestoreTimeDilation();

    bool bPlayerConfirmed      = false;
    bool bTimeDilationRestored = false;
    bool bDetonated            = false;

    UPROPERTY()
    TObjectPtr<UYogAbilityTask_PlayMontageAndWaitForEvent> MontageTask;
};
