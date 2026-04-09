#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_Knockback.generated.h"

class UAbilityTask_ApplyRootMotionMoveToForce;

/**
 * 击退 GA
 * - 授予对象：敌人角色（BeginPlay → GiveAbility）
 * - 触发方式：SendGameplayEventToActor（Instigator = 攻击者）
 * - 击退方向：Instigator → Target 的反方向（即被打飞方向）
 * - 击退距离：KnockbackDistance（cm），可按阶段在 DA 里配置
 */
UCLASS()
class DEVKIT_API UGA_Knockback : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_Knockback(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

    // 击退距离（cm）。500 = 5 米
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Knockback")
    float KnockbackDistance = 500.f;

    // 击退持续时间（秒），控制速度感
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Knockback")
    float KnockbackDuration = 0.3f;

    // 击退结束后是否清零速度
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Knockback")
    bool bZeroVelocityOnFinish = true;

private:
    UFUNCTION()
    void OnKnockbackFinished();

    UPROPERTY()
    TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> KnockbackTask;
};