#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/YogGameplayAbility.h"
#include "GA_ApplyFinisherMark.generated.h"

/**
 * 轻量 GA：向目标施加 Finisher Mark GE（模式同 GA_Knockback）
 * - 预授予：所有角色（DA_Base_AbilitySet）
 * - 激活方式：Action.Mark.Apply.Finisher GameplayEvent
 * - 配置项：FinisherMarkGEClass -> GE_Mark_Finisher（Blueprint填写）
 *
 * 本GA仅负责施加印记，清除由 GA_FinisherCharge::EndAbility 统一处理。
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_ApplyFinisherMark : public UYogGameplayAbility
{
    GENERATED_BODY()

public:
    UGA_ApplyFinisherMark(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    /** 印记 GE（Blueprint填 GE_Mark_Finisher，Duration=12s，赋予 Buff.Status.Mark.Finisher）*/
    UPROPERTY(EditDefaultsOnly, Category = "Finisher")
    TSubclassOf<UGameplayEffect> FinisherMarkGEClass;
};
