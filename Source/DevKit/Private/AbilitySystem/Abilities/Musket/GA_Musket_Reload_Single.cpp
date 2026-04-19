// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_Reload_Single.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Character/YogCharacterBase.h"

UGA_Musket_Reload_Single::UGA_Musket_Reload_Single()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.Reload"));

    // 激活时持有 Reloading 标签（阻断射击 GA 并广播换弹状态）
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Reloading"));

    // 瞄准/蓄力中不允许换弹
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Aiming"));
}

void UGA_Musket_Reload_Single::ActivateAbility(
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

    // 弹药已满，无需换弹
    if (GetCurrentAmmo() >= GetMaxAmmo())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    if (!ReloadOneMontage)
    {
        // 无蒙太奇：直接补满并结束
        SetAmmoToMax();
        ExecuteReloadCue();
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // 缓存 Handle / ActorInfo / ActivationInfo 供循环回调使用
    SavedHandle         = Handle;
    SavedActorInfo      = *ActorInfo;
    SavedActivationInfo = ActivationInfo;

    ReloadCycleCount    = 0;
    TotalCyclesToReload = FMath::RoundToInt(GetMaxAmmo() - GetCurrentAmmo());

    LockMovement();
    BeginNextReloadCycle();
}

void UGA_Musket_Reload_Single::BeginNextReloadCycle()
{
    // 检查是否换弹完成
    if (ReloadCycleCount >= TotalCyclesToReload || GetCurrentAmmo() >= GetMaxAmmo())
    {
        UnlockMovement();
        EndAbility(SavedHandle, &SavedActorInfo, SavedActivationInfo, true, false);
        return;
    }

    // 每次循环用新任务名防止 GAS 内部去重冲突
    const FName TaskName = *FString::Printf(TEXT("ReloadSingle_%d"), ReloadCycleCount);

    auto* Task = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
        this, TaskName, ReloadOneMontage, FGameplayTagContainer(), 1.f, NAME_None, true, 1.f);

    // 监听蒙太奇混出（上弹动画结束信号）
    Task->OnBlendOut.AddDynamic(this,    &UGA_Musket_Reload_Single::OnOneCycleBlendOut);
    Task->OnInterrupted.AddDynamic(this, &UGA_Musket_Reload_Single::OnOneCycleInterrupted);
    Task->OnCancelled.AddDynamic(this,   &UGA_Musket_Reload_Single::OnOneCycleCancelled);

    Task->ReadyForActivation();
}

void UGA_Musket_Reload_Single::OnOneCycleBlendOut(FGameplayTag, FGameplayEventData)
{
    // 上弹成功：+1 弹药，播放音效，进入下一循环
    AddOneAmmo();
    ExecuteReloadCue();
    ReloadCycleCount++;
    BeginNextReloadCycle();
}

void UGA_Musket_Reload_Single::OnOneCycleInterrupted(FGameplayTag, FGameplayEventData)
{
    UnlockMovement();
    EndAbility(SavedHandle, &SavedActorInfo, SavedActivationInfo, true, true);
}

void UGA_Musket_Reload_Single::OnOneCycleCancelled(FGameplayTag, FGameplayEventData)
{
    UnlockMovement();
    EndAbility(SavedHandle, &SavedActorInfo, SavedActivationInfo, true, true);
}

void UGA_Musket_Reload_Single::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UnlockMovement();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
