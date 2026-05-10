#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "Containers/Ticker.h"
#include "GA_Player_FinisherAttack.generated.h"

class UYogAbilityTask_PlayMontageAndWaitForEvent;
class AYogCharacterBase;
class UNiagaraComponent;
class UNiagaraSystem;

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

    /** Notify 缺失时使用的默认命中框数据；有 AN_MeleeDamage 时优先使用 Notify 自身配置。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback")
    FActionData FallbackFinisherActionData;

    /** 蒙太奇没有 ANS_FinisherTimeDilation 时，是否由 GA 自动开 QTE 慢动作窗口。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback")
    bool bEnableFallbackTimeDilationWindow = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback", meta = (ClampMin = "0.0"))
    float FallbackQTEWindowStartTime = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback", meta = (ClampMin = "0.05"))
    float FallbackQTEWindowDuration = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback", meta = (ClampMin = "0.001", ClampMax = "1.0"))
    float FallbackQTESlowDilation = 0.15f;

    /** 蒙太奇没有 Ability.Event.Finisher.HitFrame 的 AN_MeleeDamage 时，是否由 GA 自动触发一次命中帧。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback")
    bool bEnableFallbackHitFrame = true;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|Fallback", meta = (ClampMin = "0.0"))
    float FallbackHitFrameTime = 0.65f;

    /** 终结技动作开始前/开始瞬间播放的可配置 Niagara 周身特效。为空时仍会播放角色金色 Overlay。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX")
    TObjectPtr<UNiagaraSystem> PreFinisherAuraNiagara;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX")
    FName PreFinisherAuraAttachSocketName = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX")
    FVector PreFinisherAuraLocationOffset = FVector(0.f, 0.f, 45.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX")
    FRotator PreFinisherAuraRotationOffset = FRotator::ZeroRotator;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX")
    FVector PreFinisherAuraScale = FVector(1.2f, 1.2f, 1.2f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX", meta = (ClampMin = "0.0"))
    float PreFinisherAuraDuration = 0.85f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher|VFX")
    bool bDestroyPreFinisherAuraOnAbilityEnd = true;

    virtual FActionData GetAbilityActionData_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category = "Finisher|VFX")
    void StartPreFinisherAura();

    UFUNCTION(BlueprintCallable, Category = "Finisher|VFX")
    void StopPreFinisherAura();

    UFUNCTION(BlueprintImplementableEvent, Category = "Finisher|VFX")
    void BP_OnPreFinisherAuraStarted(UNiagaraComponent* AuraComponent);

    UFUNCTION(BlueprintImplementableEvent, Category = "Finisher|VFX")
    void BP_OnPreFinisherAuraEnded();

private:
    UFUNCTION()
    void OnMontageEvent(FGameplayTag EventTag, FGameplayEventData EventData);

    UFUNCTION()
    void OnMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData);

    UFUNCTION()
    void OnMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);

    void DetonateMarks(bool bConfirmed);

    void ApplyFinisherHitbox(const FGameplayEventData& EventData);
    void HandleFinisherHitFrame(const FGameplayEventData& EventData);
    void RestoreTimeDilation(bool bEndExternalVisual = false);
    void OpenFallbackQTEWindow();
    void CloseFallbackQTEWindow();
    void TriggerFallbackHitFrame();
    void ScheduleTicker(FTSTicker::FDelegateHandle& Handle, float DelaySeconds, TFunction<void()> Callback);
    void ClearTicker(FTSTicker::FDelegateHandle& Handle);
    void ClearFallbackTickers();
    bool MontageHasFinisherTimeDilationNotify() const;
    bool MontageHasFinisherHitFrameNotify() const;

    bool bPlayerConfirmed      = false;
    bool bTimeDilationRestored = false;
    bool bDetonated            = false;
    bool bReceivedHitFrame     = false;
    bool bFallbackQTEOpened    = false;
    bool bPreFinisherAuraActive = false;
    bool bTimeDilationVisualActive = false;

    UPROPERTY()
    TObjectPtr<UYogAbilityTask_PlayMontageAndWaitForEvent> MontageTask;

    UPROPERTY()
    TObjectPtr<UNiagaraComponent> PreFinisherAuraComponent;

    FTSTicker::FDelegateHandle FallbackQTEStartTickerHandle;
    FTSTicker::FDelegateHandle FallbackQTEEndTickerHandle;
    FTSTicker::FDelegateHandle FallbackHitFrameTickerHandle;
    FTSTicker::FDelegateHandle PreFinisherAuraTickerHandle;
};
