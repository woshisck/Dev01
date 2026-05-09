#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_FinisherCharge.generated.h"

class UAbilityTask_WaitGameplayEvent;
class AYogCharacterBase;

/**
 * 终结技充能窗口管理 GA
 * - 预授予：玩家（DA_Base_AbilitySet）
 * - 激活方式：Action.FinisherCharge.Activate GameplayEvent
 * - 职责：5次窗口计数、每次命中发送击退+印记、窗口结束清除印记
 *
 * Blueprint 必须配置：
 *   AbilityTags             = PlayerState.AbilityCast.FinisherCharge
 *   ActivationOwnedTags     = PlayerState.AbilityCast.FinisherCharge   ← 阻断自身重复激活
 *   ActivationBlockedTags   = PlayerState.AbilityCast.FinisherCharge
 *   FinisherChargeGEClass   = GE_FinisherCharge
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_FinisherCharge : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_FinisherCharge(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

    /** 窗口内最多处理的击退+印记次数（对应GE最大层数）*/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    int32 MaxCharges = 5;

    /** 用于查找和移除充能GE的类（Blueprint填 GE_FinisherCharge）*/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    TSubclassOf<UGameplayEffect> FinisherChargeGEClass;

private:
    UFUNCTION()
    void OnAttackHit(FGameplayEventData EventData);

    void OnFinisherChargeTagChanged(const FGameplayTag Tag, int32 NewCount);

    // 延迟一帧结束，确保 AN_TriggerFinisherAbility 能在同帧检测到 FinisherWindowOpen
    void EndAbilityDeferred();

    void ClearAllMarks();

    int32 RemainingCharges = 0;

    FActiveGameplayEffectHandle ChargeGEHandle;

    FDelegateHandle TagChangedHandle;

    FTimerHandle DeferredEndHandle;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitGameplayEvent> WaitHitTask;
};
