#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GameplayTagContainer.h"
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
 * 动画结束后自动调用 FinishDying()。
 *
 * AbilityTriggers 配置（编辑器里填）：
 *   Trigger Tag    = Action.Dead
 *   Trigger Source = GameplayEvent
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

    FGameplayTag CachedDissolveTag;
    FTimerHandle DeathDelayTimer;

    void StartDeathDelay();
    void OnDeathDelayExpired();

    UFUNCTION()
    void OnDeathMontageCompleted();

    UFUNCTION()
    void OnDeathMontageBlendOut();

    UFUNCTION()
    void OnDeathMontageCancelled();
};
