// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_SprintAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Projectile/MusketBullet.h"

UGA_Musket_SprintAttack::UGA_Musket_SprintAttack()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.SprintAtk"));
    // LightAtk 按键时与 LightAttack 共用同一激活 Tag，由 ActivationRequiredTags 区分
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.LightAtk"));

    // 必须在冲刺（DashInvincible 标签）中才能激活
    ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.DashInvincible"));

    // 激活后取消冲刺（"转化"冲刺为攻击）
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.Dash"));
}

void UGA_Musket_SprintAttack::ActivateAbility(
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

    const int32   BulletCount = FMath::RoundToInt(GetCurrentAmmo());
    const float   Damage      = GetBaseAttack() * DamageMultiplier;

    // 均匀扇形射出全部子弹
    for (int32 i = 0; i < BulletCount; ++i)
    {
        float Angle = 0.f;
        if (BulletCount > 1)
        {
            Angle = FMath::Lerp(-HalfFanAngle, HalfFanAngle,
                                static_cast<float>(i) / static_cast<float>(BulletCount - 1));
        }

        AMusketBullet* Bullet = SpawnBullet(Angle, Damage);

        // 为每颗子弹施加击退 GE（通过子弹命中时的目标，在 BP_MusketBullet.BP_OnHitEnemy 中处理）
        // 此处额外将击退 GE 存储供子弹引用——简单方案：子弹存 GE 类并在 OnHit 中施加
        // 若要 C++ 完全驱动，可为子弹添加 KnockbackEffectClass 字段后赋值
        // 当前：交由 Blueprint BP_MusketBullet 的 BP_OnHitEnemy 施加击退（简单）
        (void)Bullet;
    }

    ClearAllAmmo();
    ExecuteFireCue();

    // 播放冲刺攻击蒙太奇
    if (!SprintAtkMontage)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    auto* Task = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
        this, NAME_None, SprintAtkMontage, FGameplayTagContainer(), 1.f, NAME_None, true, 1.f);

    Task->OnBlendOut.AddDynamic(this,    &UGA_Musket_SprintAttack::OnMontageBlendOut);
    Task->OnInterrupted.AddDynamic(this, &UGA_Musket_SprintAttack::OnMontageInterrupted);
    Task->OnCancelled.AddDynamic(this,   &UGA_Musket_SprintAttack::OnMontageCancelled);

    Task->ReadyForActivation();
}

void UGA_Musket_SprintAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Musket_SprintAttack::OnMontageBlendOut(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UGA_Musket_SprintAttack::OnMontageInterrupted(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}

void UGA_Musket_SprintAttack::OnMontageCancelled(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}
