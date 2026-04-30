// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_LightAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Character/YogCharacterBase.h"
#include "Projectile/MusketBullet.h"

UGA_Musket_LightAttack::UGA_Musket_LightAttack()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.Light"));
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.LightAtk"));

    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.Reload"));

    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Aiming"));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Reloading"));
    // 冲刺中由 SprintAttack 处理，不激活普通轻攻击
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.DashInvincible"));
}

void UGA_Musket_LightAttack::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!InitCharacterCache(ActorInfo) || !HasAmmo())
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    bFired = false;
    LockMovement();

    // 若 FireEventTag 无效，立刻开枪（蒙太奇纯作视觉用）
    if (!FireEventTag.IsValid())
    {
        DoFire();
    }

    // 无蒙太奇时直接结束
    if (!FireMontage)
    {
        UnlockMovement();
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    FGameplayTagContainer EventTags;
    if (FireEventTag.IsValid())
    {
        EventTags.AddTag(FireEventTag);
    }

    auto* Task = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
        this, NAME_None, FireMontage, EventTags, 1.f, NAME_None, true, 1.f);

    Task->EventReceived.AddDynamic(this,  &UGA_Musket_LightAttack::OnFireEvent);
    Task->OnCompleted.AddDynamic(this,    &UGA_Musket_LightAttack::OnMontageComplete);
    Task->OnBlendOut.AddDynamic(this,     &UGA_Musket_LightAttack::OnMontageComplete);
    Task->OnInterrupted.AddDynamic(this,  &UGA_Musket_LightAttack::OnMontageInterrupted);
    Task->OnCancelled.AddDynamic(this,    &UGA_Musket_LightAttack::OnMontageCancelled);

    Task->ReadyForActivation();
}

void UGA_Musket_LightAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UnlockMovement();
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Musket_LightAttack::DoFire()
{
    if (bFired) return;
    bFired = true;

    const float Damage = GetBaseAttack() * DamageMultiplier;
    const float Angle  = FMath::RandRange(-HalfAngleDeg, HalfAngleDeg);
    if (AMusketBullet* Bullet = SpawnBullet(Angle, Damage))
    {
        Bullet->SetCombatDeckContext(ECardRequiredAction::Light, false, false);
    }
    ConsumeOneAmmo();
    ExecuteFireCue();
}

void UGA_Musket_LightAttack::OnFireEvent(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
    DoFire();
}

void UGA_Musket_LightAttack::OnMontageComplete(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UGA_Musket_LightAttack::OnMontageInterrupted(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}

void UGA_Musket_LightAttack::OnMontageCancelled(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}
