// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/AbilityTask/AbilityTask_MusketCharge.h"

UAbilityTask_MusketCharge::UAbilityTask_MusketCharge(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bTickingTask = true;
}

UAbilityTask_MusketCharge* UAbilityTask_MusketCharge::CreateMusketCharge(
    UGameplayAbility* OwningAbility,
    float InChargeTime,
    float InStartHalfAngle,
    float InEndHalfAngle,
    float InStartRadius,
    float InEndRadius)
{
    UAbilityTask_MusketCharge* Task = NewAbilityTask<UAbilityTask_MusketCharge>(OwningAbility);
    Task->ChargeTime      = FMath::Max(InChargeTime, 0.01f);
    Task->StartHalfAngle  = InStartHalfAngle;
    Task->EndHalfAngle    = InEndHalfAngle;
    Task->StartRadius     = InStartRadius;
    Task->EndRadius       = InEndRadius;
    Task->CurrentHalfAngle = InStartHalfAngle;
    return Task;
}

void UAbilityTask_MusketCharge::Activate()
{
    ChargeElapsed   = 0.f;
    bFullNotified   = false;
    CurrentHalfAngle = StartHalfAngle;
}

void UAbilityTask_MusketCharge::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    ChargeElapsed += DeltaTime;
    const float Alpha = FMath::Clamp(ChargeElapsed / ChargeTime, 0.f, 1.f);

    CurrentHalfAngle       = FMath::Lerp(StartHalfAngle, EndHalfAngle, Alpha);
    const float CurRadius  = FMath::Lerp(StartRadius,    EndRadius,    Alpha);
    const bool  bFull      = Alpha >= 1.f;

    if (ShouldBroadcastAbilityTaskDelegates())
    {
        OnChargeTick.Broadcast(CurrentHalfAngle, CurRadius, bFull);

        if (bFull && !bFullNotified)
        {
            bFullNotified = true;
            OnChargeFull.Broadcast();
        }
    }
}

void UAbilityTask_MusketCharge::OnDestroy(bool bAbilityEnded)
{
    Super::OnDestroy(bAbilityEnded);
}

float UAbilityTask_MusketCharge::GetChargeAlpha() const
{
    return FMath::Clamp(ChargeElapsed / ChargeTime, 0.f, 1.f);
}
