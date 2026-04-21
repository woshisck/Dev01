// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_Reload_All.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Character/YogCharacterBase.h"

UGA_Musket_Reload_All::UGA_Musket_Reload_All()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.Reload"));
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.Reload"));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Reloading"));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Aiming"));
}

void UGA_Musket_Reload_All::ActivateAbility(
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

    if (GetCurrentAmmo() >= GetMaxAmmo())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    LockMovement();

    // 无蒙太奇：立即补满
    if (!ReloadAllMontage)
    {
        SetAmmoToMax();
        ExecuteReloadCue();
        UnlockMovement();
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    auto* Task = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
        this, NAME_None, ReloadAllMontage, FGameplayTagContainer(), 1.f, NAME_None, true, 1.f);

    Task->OnBlendOut.AddDynamic(this,    &UGA_Musket_Reload_All::OnReloadBlendOut);
    Task->OnInterrupted.AddDynamic(this, &UGA_Musket_Reload_All::OnReloadInterrupted);
    Task->OnCancelled.AddDynamic(this,   &UGA_Musket_Reload_All::OnReloadCancelled);

    Task->ReadyForActivation();
}

void UGA_Musket_Reload_All::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UnlockMovement();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Musket_Reload_All::OnReloadBlendOut(FGameplayTag, FGameplayEventData)
{
    SetAmmoToMax();
    ExecuteReloadCue();
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UGA_Musket_Reload_All::OnReloadInterrupted(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}

void UGA_Musket_Reload_All::OnReloadCancelled(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}
