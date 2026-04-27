#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GA_HitReaction.generated.h"

class UAbilityTask_PlayMontageAndWait;
class AEnemyCharacterBase;

/**
 * GA_HitReaction
 *
 * 受击硬直 GA，所有角色通用。
 *
 * 触发方式：FA 通过 BFNode_SendGameplayEvent 发送 Action.HitReact 事件至目标，
 *           或代码直接调用 ASC->HandleGameplayEvent(Action.HitReact)。
 *           FA 层负责判断是否应该触发受击（符文条件、无敌帧等），GA 只负责播放动画。
 *
 * 动画来源：CharacterData → AbilityData → PassiveMap["Action.HitReact"].Montage
 *
 * AbilityTriggers 配置（编辑器里填）：
 *   Trigger Tag    = Action.HitReact
 *   Trigger Source = GameplayEvent
 */
UCLASS()
class DEVKIT_API UGA_HitReaction : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_HitReaction(const FObjectInitializer& ObjectInitializer);

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

    TWeakObjectPtr<AEnemyCharacterBase> MovementLockedEnemy;
    TEnumAsByte<EMovementMode> PreviousMovementMode = MOVE_Walking;
    bool bLockedEnemyMovement = false;

    UFUNCTION()
    void OnMontageCompleted();

    UFUNCTION()
    void OnMontageBlendOut();

    UFUNCTION()
    void OnMontageCancelled();

    UFUNCTION()
    void OnMontageInterrupted();
};
