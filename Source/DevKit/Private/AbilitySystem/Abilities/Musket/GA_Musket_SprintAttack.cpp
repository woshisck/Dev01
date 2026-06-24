// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_SprintAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Character/YogCharacterBase.h"
#include "Data/MusketActionTuningDataAsset.h"
#include "Projectile/MusketBullet.h"

UGA_Musket_SprintAttack::UGA_Musket_SprintAttack()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.SprintAtk"));
    // Attack input shares the ranged attack tag; ActivationRequiredTags selects sprint fire.
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Character.State.Skill.Attack"));
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.Attack"));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Character.State.Skill.Attack"));

    // 必须在冲刺（DashInvincible 标签）中才能激
    ActivationRequiredTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.DashInvincible"));

    // 激活后取消冲刺转化"冲刺为攻击）
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("PlayerState.AbilityCast.Dash"));
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Character.State.Movement.Dash"));
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
    const float   TunedDamageMultiplier = TuningData ? TuningData->SprintDamageMultiplier : DamageMultiplier;
    const float   TunedHalfFanAngle = TuningData ? TuningData->SprintHalfFanAngle : HalfFanAngle;
    const float   Damage      = GetBaseAttack() * TunedDamageMultiplier;
    const FGuid   AttackGuid  = ResolveCombatDeckOnFire(
        ECardRequiredAction::Any,
        ECombatDeckActionSlot::Attack,
        ECombatDeckFlowRole::Any,
        false,
        false,
        Damage,
        0.f);

    // 均匀扇形射出全部子弹
    for (int32 i = 0; i < BulletCount; ++i)
    {
        float Angle = 0.f;
        if (BulletCount > 1)
        {
            Angle = FMath::Lerp(-TunedHalfFanAngle, TunedHalfFanAngle,
                                static_cast<float>(i) / static_cast<float>(BulletCount - 1));
        }

        AMusketBullet* Bullet = SpawnBullet(Angle, Damage);
        if (Bullet)
        {
            ApplyCombatDeckContextToBullet(
                Bullet,
                ECardRequiredAction::Any,
                ECombatDeckActionSlot::Attack,
                ECombatDeckFlowRole::Any,
                false,
                false,
                AttackGuid,
                Damage);
        }

        // 为每颗子弹施加击退 GE（通过子弹命中时的目标，在 BP_MusketBullet.BP_OnHitEnemy 中处理）
        // 此处额外将击退 GE 存储供子弹引用——简单方案：子弹GE 类并OnHit 中施
        // 若要 C++ 完全驱动，可为子弹添KnockbackEffectClass 字段后赋
        // 当前：交Blueprint BP_MusketBullet BP_OnHitEnemy 施加击退（简单）
        (void)Bullet;
    }

    ClearAllAmmo();
    ExecuteFireCue();

    // 播放冲刺攻击蒙太
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
