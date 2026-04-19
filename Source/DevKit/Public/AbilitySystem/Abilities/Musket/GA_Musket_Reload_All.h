// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "GA_Musket_Reload_All.generated.h"

/**
 * 全换弹 GA（一次性换满所有子弹）。
 *
 * 工作流：
 *   检查弹药是否满 → 阻断移动
 *   → 播放 ReloadAllMontage → 蒙太奇结束 → CurrentAmmo = MaxAmmo → 换弹 Cue
 *   → 恢复移动 → 结束
 *
 * Blueprint 子类 Class Defaults 需填写：
 *   - ReloadAllMontage   整体换弹动画蒙太奇
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Musket_Reload_All : public UGA_MusketBase
{
    GENERATED_BODY()

public:
    UGA_Musket_Reload_All();

    /** 整体换弹动画蒙太奇 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|Reload")
    TObjectPtr<UAnimMontage> ReloadAllMontage;

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
    UFUNCTION() void OnReloadBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnReloadInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnReloadCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
