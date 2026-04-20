// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Abilities/Musket/GA_Musket_HeavyAttack.h"
#include "AbilitySystem/AbilityTask/AbilityTask_MusketCharge.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Item/Weapon/AimArcActor.h"
#include "Character/YogCharacterBase.h"

UGA_Musket_HeavyAttack::UGA_Musket_HeavyAttack()
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.Heavy"));

    // C++ 默认值：无材质时弧不可见但不崩溃
    AimArcClass = AYogAimArcActor::StaticClass();

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Aiming"));

    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Ability.Musket.Reload"));

    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Musket.Reloading"));
}

void UGA_Musket_HeavyAttack::ActivateAbility(
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

    CurrentHalfAngle = StartHalfAngle;
    bFullCharge      = false;
    bFired           = false;

    LockMovement();

    // 生成并显示瞄准弧
    if (AimArcClass && GetWorld())
    {
        FActorSpawnParameters P;
        P.Instigator = Cast<APawn>(CachedCharacter);
        P.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

        AimArcActor = GetWorld()->SpawnActor<AYogAimArcActor>(
            AimArcClass,
            CachedCharacter->GetActorLocation(),
            CachedCharacter->GetActorRotation(),
            P);

        if (AimArcActor)
        {
            AimArcActor->UpdateArc(StartHalfAngle, StartRadius);
            AimArcActor->SetArcColor(NormalArcColor);
            AimArcActor->ShowArc();
        }
    }

    // 启动蓄力 Tick 任务（bTickingTask=true，每帧调用 TickTask）
    ChargeTask = UAbilityTask_MusketCharge::CreateMusketCharge(
        this, ChargeTime, StartHalfAngle, EndHalfAngle, StartRadius, EndRadius);
    ChargeTask->OnChargeTick.AddDynamic(this, &UGA_Musket_HeavyAttack::OnChargeTick);
    ChargeTask->OnChargeFull.AddDynamic(this, &UGA_Musket_HeavyAttack::OnChargeFullNotify);
    ChargeTask->ReadyForActivation();

    // 等待玩家松开输入
    UAbilityTask_WaitInputRelease* ReleaseTask =
        UAbilityTask_WaitInputRelease::WaitInputRelease(this, false);
    ReleaseTask->OnRelease.AddDynamic(this, &UGA_Musket_HeavyAttack::OnInputReleased);
    ReleaseTask->ReadyForActivation();
}

void UGA_Musket_HeavyAttack::OnChargeTick(float HalfAngleDeg, float RadiusCm, bool bFull)
{
    CurrentHalfAngle = HalfAngleDeg;

    if (AimArcActor && CachedCharacter)
    {
        AimArcActor->SetActorLocation(CachedCharacter->GetActorLocation());
        AimArcActor->SetActorRotation(CachedCharacter->GetActorRotation());
        AimArcActor->UpdateArc(HalfAngleDeg, RadiusCm);
    }
}

void UGA_Musket_HeavyAttack::OnChargeFullNotify()
{
    bFullCharge = true;
    ExecuteChargeFullCue();
    if (AimArcActor) AimArcActor->SetArcColor(FullChargeArcColor);
}

void UGA_Musket_HeavyAttack::OnInputReleased(float /*TimeHeld*/)
{
    DoFire();
}

void UGA_Musket_HeavyAttack::DoFire()
{
    if (bFired) return;
    bFired = true;

    // 停止蓄力 Tick
    if (ChargeTask)
    {
        ChargeTask->EndTask();
        ChargeTask = nullptr;
    }
    CleanupArc();

    const float Multiplier = bFullCharge ? FullChargeMultiplier : BaseDamageMultiplier;
    const float Damage     = GetBaseAttack() * Multiplier;
    const float Angle      = FMath::RandRange(-CurrentHalfAngle, CurrentHalfAngle);
    SpawnBullet(Angle, Damage);
    ConsumeOneAmmo();
    ExecuteFireCue();

    if (FireMontage)
    {
        auto* Task = UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
            this, NAME_None, FireMontage, FGameplayTagContainer(), 1.f, NAME_None, true, 1.f);

        Task->OnCompleted.AddDynamic(this,   &UGA_Musket_HeavyAttack::OnFireMontageComplete);
        Task->OnBlendOut.AddDynamic(this,    &UGA_Musket_HeavyAttack::OnFireMontageBlendOut);
        Task->OnInterrupted.AddDynamic(this, &UGA_Musket_HeavyAttack::OnFireMontageInterrupted);
        Task->OnCancelled.AddDynamic(this,   &UGA_Musket_HeavyAttack::OnFireMontageCancelled);
        Task->ReadyForActivation();
    }
    else
    {
        EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
                   GetCurrentActivationInfo(), true, false);
    }
}

void UGA_Musket_HeavyAttack::CleanupArc()
{
    if (AimArcActor)
    {
        AimArcActor->HideArc();
        AimArcActor->Destroy();
        AimArcActor = nullptr;
    }
}

void UGA_Musket_HeavyAttack::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (ChargeTask)
    {
        ChargeTask->EndTask();
        ChargeTask = nullptr;
    }
    CleanupArc();
    UnlockMovement();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Musket_HeavyAttack::OnFireMontageComplete(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UGA_Musket_HeavyAttack::OnFireMontageBlendOut(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, false);
}

void UGA_Musket_HeavyAttack::OnFireMontageInterrupted(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}

void UGA_Musket_HeavyAttack::OnFireMontageCancelled(FGameplayTag, FGameplayEventData)
{
    EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(),
               GetCurrentActivationInfo(), true, true);
}
