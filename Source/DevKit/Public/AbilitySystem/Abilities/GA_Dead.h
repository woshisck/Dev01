#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_Dead.generated.h"

class UAbilityTask_PlayMontageAndWait;

/**
 * GA_Dead
 *
 * 死亡 GA，所有角色通用。
 *
 * 触发方式：YogCharacterBase::Die() 在血量归零时自动调用，
 *           通过 ASC->HandleGameplayEvent(Action.Dead) 触发本 GA。
 *           不需要任何手动操作。
 *
 * 动画来源：CharacterData → AbilityData → PassiveMap["Action.Dead"].Montage
 *
 * 时序：
 *   有蒙太奇 → 播放蒙太奇 → 蒙太奇结束 → 等 2s → 触发消解 GC → Destroy
 *   无蒙太奇 →                              等 2s → 触发消解 GC → Destroy
 *
 * 消解特效配置：AbilityData → PassiveMap["Action.Dead"].DissolveGameplayCueTag
 *   填写 GameplayCue Tag，在对应 GC BP 里配 Niagara/材质消解等效果（世界坐标生成）
 */
UCLASS()
class DEVKIT_API UGA_Dead : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Dead(const FObjectInitializer& ObjectInitializer);

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
    UPROPERTY()
    TObjectPtr<UAbilityTask_PlayMontageAndWait> MontageTask;

    // 蒙太奇结束或无蒙太奇后的延迟计时器（2 秒），用于播放消解特效窗口
    FTimerHandle DeathDelayTimer;

    // 从 AbilityData 缓存的消解 GC Tag
    FGameplayTag CachedDissolveTag;

    UFUNCTION()
    void OnDeathMontageCompleted();

    UFUNCTION()
    void OnDeathMontageBlendOut();

    UFUNCTION()
    void OnDeathMontageCancelled();

    // 蒙太奇结束后开始 2s 等待
    void StartDeathDelay();

    // 2s 等待结束 → 触发消解 GC → FinishDying → EndAbility
    UFUNCTION()
    void OnDeathDelayExpired();
};
