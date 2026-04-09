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
 *
 * 动画来源：CharacterData → AbilityData → PassiveMap["Action.Dead"].Montage
 *
 * 时序（有 DissolveTag）：
 *   有蒙太奇 → 播放 → BlendOut → 触发消解 GC → 等 2s（消解在活体角色上播放）→ Destroy
 *   无蒙太奇 →                  触发消解 GC → 等 2s → Destroy
 *
 * 时序（无 DissolveTag）：
 *   有蒙太奇 → 播放 → BlendOut → 立即 Destroy
 *   无蒙太奇 →                  立即 Destroy
 *
 * 消解特效配置：AbilityData → PassiveMap["Action.Dead"].DissolveGameplayCueTag
 *   填写 GameplayCue Tag，在对应 GC BP 里配 Niagara/材质消解等效果。
 *   消解触发时角色仍存活（2s 内），适合做材质 Dissolve。
 *   ⚠️ 若需要在动画特定帧触发消解，将 AN_TriggerGameplayCue AnimNotify 放在蒙太奇对应帧，
 *      并直接填写 GC Tag（不依赖本 GA），同时此处 DissolveGameplayCueTag 留空即可避免重复触发。
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

    // 蒙太奇结束后的延迟计时器（2 秒），在消解 GC 触发后等待角色完成消解再销毁
    FTimerHandle DeathDelayTimer;

    // 从 AbilityData 缓存的消解 GC Tag
    FGameplayTag CachedDissolveTag;

    UFUNCTION()
    void OnDeathMontageCompleted();

    UFUNCTION()
    void OnDeathMontageBlendOut();

    UFUNCTION()
    void OnDeathMontageCancelled();

    /**
     * 蒙太奇结束后开始销毁流程：
     * - 有 DissolveTag：立即触发消解 GC（此时角色仍存活），等 2s 后销毁
     * - 无 DissolveTag：立即销毁
     * 防止 OnBlendOut + OnCompleted 双重触发（计时器 IsValid 守卫）。
     */
    void StartDeathDelay();

    // 2s 等待结束 → FinishDying → EndAbility
    UFUNCTION()
    void OnDeathDelayExpired();
};
