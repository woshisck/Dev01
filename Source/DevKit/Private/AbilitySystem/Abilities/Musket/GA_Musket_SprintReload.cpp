// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_SprintReload.h"
#include "Character/YogCharacterBase.h"
#include "TimerManager.h"

UGA_Musket_SprintReload::UGA_Musket_SprintReload()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.SprintReload"));
    // Reload 按键时与 Reload_Single/All 共用同一激活 Tag，由 ActivationRequiredTags 区分
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.Reload"));

    // 必须在冲刺状态（DashInvincible）中才能激活
    ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.DashInvincible"));

    // 不取消冲刺（冲刺继续，换弹与冲刺并发）
}

void UGA_Musket_SprintReload::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!InitCharacterCache(ActorInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 步骤 1：清空弹药并播放弹夹掉落音效
    ClearAllAmmo();
    ExecuteReloadCue();

    // 步骤 2：等待 RefillDelay 后补满（Timer 在 EndAbility 时会自动清理）
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RefillTimerHandle,
            this,
            &UGA_Musket_SprintReload::OnRefillTimer,
            FMath::Max(RefillDelay, 0.01f),
            false);
    }
    else
    {
        // 无 World（不应发生）：立即补满
        OnRefillTimer();
    }
}

void UGA_Musket_SprintReload::OnRefillTimer()
{
    SetAmmoToMax();
    ExecuteReloadCue();

    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UGA_Musket_SprintReload::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RefillTimerHandle);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
