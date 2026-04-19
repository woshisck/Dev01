// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Musket/GA_MusketBase.h"
#include "GA_Musket_Reload_Single.generated.h"

/**
 * 单发换弹 GA（循环上弹，类似泵式霰弹枪）。
 *
 * 工作流：
 *   检查弹药是否满 → 阻断移动
 *   → 循环（ReloadCycleCount < TotalCyclesToReload）：
 *       播放 ReloadOneMontage → 蒙太奇结束 → +1 弹药 → 换弹 Cue
 *   → 恢复移动 → 结束
 *
 * 中途停止：
 *   轻/重攻击 GA 配置 CancelAbilitiesWithTag = Ability.Musket.Reload，
 *   激活攻击时会通过 GAS 自动取消此 GA（EndAbility bWasCancelled=true），
 *   已上好的子弹保留。
 *
 * Blueprint 子类 Class Defaults 需填写：
 *   - ReloadOneMontage   单发上弹动画（约 0.6~0.8s）
 *   （基类 BulletClass / MuzzleSocketName 等不需要，此 GA 不发射子弹）
 */
UCLASS(BlueprintType, Blueprintable)
class DEVKIT_API UGA_Musket_Reload_Single : public UGA_MusketBase
{
    GENERATED_BODY()

public:
    UGA_Musket_Reload_Single();

    /** 单发上弹动画蒙太奇 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Musket|Reload")
    TObjectPtr<UAnimMontage> ReloadOneMontage;

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
    int32 ReloadCycleCount    = 0;
    int32 TotalCyclesToReload = 0;

    FGameplayAbilitySpecHandle  SavedHandle;
    FGameplayAbilityActorInfo   SavedActorInfo;
    FGameplayAbilityActivationInfo SavedActivationInfo;

    void BeginNextReloadCycle();

    UFUNCTION() void OnOneCycleBlendOut(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnOneCycleInterrupted(FGameplayTag EventTag, FGameplayEventData EventData);
    UFUNCTION() void OnOneCycleCancelled(FGameplayTag EventTag, FGameplayEventData EventData);
};
