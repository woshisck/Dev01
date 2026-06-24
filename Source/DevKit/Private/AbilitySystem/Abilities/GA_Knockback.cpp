#include "AbilitySystem/Abilities/GA_Knockback.h"

#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/PlayerCharacterBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"

namespace
{
FVector GetHorizontalSafeNormal(FVector Direction)
{
    Direction.Z = 0.f;
    return Direction.IsNearlyZero() ? FVector::ZeroVector : Direction.GetSafeNormal();
}
}

UGA_Knockback::UGA_Knockback(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // GA 激活期间自动挂ASC 上，结束时自动移
    // 注：AbilityTags 身份标签不设置——击退可作用于玩家/敌人任意角色，无需命名空间归属
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Knockback"));

    // 实例化模式：每次激活一个独立实
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

    // 击退激活时立即取消受击硬直 GA（GA_HitReaction），改为播放击退专用动画
    CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead")));

    // 监听 Action.Knockback 事件自动激活（FA Send Gameplay Event 节点发出Tag
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"));
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

FVector UGA_Knockback::ResolveKnockbackDirection(
    const ACharacter* TargetChar,
    const FGameplayEventData* TriggerEventData)
{
    if (!TargetChar)
    {
        return FVector::ZeroVector;
    }

    if (TriggerEventData)
    {
        for (int32 Index = 0; Index < TriggerEventData->TargetData.Num(); ++Index)
        {
            const FGameplayAbilityTargetData* DirectionData = TriggerEventData->TargetData.Get(Index);
            if (!DirectionData || !DirectionData->HasOrigin() || !DirectionData->HasEndPoint())
            {
                continue;
            }

            const FVector ExplicitDirection = GetHorizontalSafeNormal(
                DirectionData->GetEndPoint() - DirectionData->GetOrigin().GetLocation());
            if (!ExplicitDirection.IsNearlyZero())
            {
                return ExplicitDirection;
            }
        }
    }

    FVector KnockbackDir = TargetChar->GetActorForwardVector() * -1.f;

    if (TriggerEventData && TriggerEventData->Instigator != nullptr)
    {
        FVector FromInstigator = TargetChar->GetActorLocation()
            - TriggerEventData->Instigator->GetActorLocation();
        FromInstigator.Z = 0.f;

        if (!FromInstigator.IsNearlyZero())
        {
            KnockbackDir = FromInstigator.GetSafeNormal();
        }
    }

    return GetHorizontalSafeNormal(KnockbackDir);
}

FVector UGA_Knockback::ResolveAttackDirectionFromSource(const AActor* SourceActor)
{
    const AActor* DirectionActor = SourceActor;
    if (const AController* Controller = Cast<AController>(DirectionActor))
    {
        if (const APawn* Pawn = Controller->GetPawn())
        {
            DirectionActor = Pawn;
        }
    }

    if (const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(DirectionActor))
    {
        const FVector InputDirection = GetHorizontalSafeNormal(Player->LastInputDirection);
        if (!InputDirection.IsNearlyZero())
        {
            return InputDirection;
        }
    }

    return DirectionActor
        ? GetHorizontalSafeNormal(DirectionActor->GetActorForwardVector())
        : FVector::ZeroVector;
}

void UGA_Knockback::AppendAttackDirectionTargetData(
    FGameplayEventData& EventData,
    const FVector& InDirection,
    const AActor* AnchorActor)
{
    const FVector Direction = GetHorizontalSafeNormal(InDirection);
    if (Direction.IsNearlyZero())
    {
        return;
    }

    const FVector Origin = AnchorActor ? AnchorActor->GetActorLocation() : FVector::ZeroVector;
    FGameplayAbilityTargetData_LocationInfo* DirectionData = new FGameplayAbilityTargetData_LocationInfo();
    DirectionData->SourceLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
    DirectionData->SourceLocation.LiteralTransform = FTransform(FRotator::ZeroRotator, Origin);
    DirectionData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
    DirectionData->TargetLocation.LiteralTransform = FTransform(Direction.Rotation(), Origin + Direction);
    EventData.TargetData.Add(DirectionData);
}

void UGA_Knockback::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // ── DEBUG ──────────────────────────────────────────────────────────────
    AActor* DbgAvatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
    const AActor* DbgInstigator = TriggerEventData ? TriggerEventData->Instigator.Get() : nullptr;
    UE_LOG(LogTemp, Warning, TEXT("[GA_Knockback] ActivateAbility called | Target=%s | Instigator=%s"),
        DbgAvatar     ? *DbgAvatar->GetName()     : TEXT("NULL"),
        DbgInstigator ? *DbgInstigator->GetName() : TEXT("NULL (方向将使用默认向"));
    // ──────────────────────────────────────────────────────────────────────

    ACharacter* TargetChar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!TargetChar)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Knockback] AvatarActor 不是 ACharacter，提前结"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // ---- 计算击退方向 ----
    // Prefer an explicit attack direction in the event payload, then fall back to instigator position.
    const FVector KnockbackDir = ResolveKnockbackDirection(TargetChar, TriggerEventData);

    // ---- 计算目标位置 ----
    const float EffectiveKnockbackDistance =
        (TriggerEventData && TriggerEventData->EventMagnitude > KINDA_SMALL_NUMBER)
            ? TriggerEventData->EventMagnitude
            : KnockbackDistance;
    const FVector StartLocation  = TargetChar->GetActorLocation();
    const FVector TargetLocation = StartLocation + KnockbackDir * EffectiveKnockbackDistance;

    // ---- 创建 Root Motion Task ----
    // MoveToForce 直接指定目标位置，距离精确可控（单位 cm
    // 函数签名OwningAbility, TaskName, TargetLocation, Duration,
    //            bSetNewMovementMode, NewMovementMode, bRestrictSpeedToExpected,
    //            PathOffsetCurve, VelocityOnFinishMode, SetVelocityOnFinish, ClampVelocityOnFinish)
    {
        UCharacterMovementComponent* CMC = TargetChar->GetCharacterMovement();
        UE_LOG(LogTemp, Warning, TEXT("[GA_Knockback] KnockDir=%s TargetLoc=%s Distance=%.0f Duration=%.2f CMC_Mode=%d"),
            *KnockbackDir.ToString(), *TargetLocation.ToString(),
            EffectiveKnockbackDistance, KnockbackDuration,
            CMC ? (int32)CMC->MovementMode : -1);
    }
    KnockbackTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
        this,
        NAME_None,
        TargetLocation,
        KnockbackDuration,
        true,                       // bSetNewMovementMode 强制切到 Walking，避MOVE_None 跳过 Root Motion
        EMovementMode::MOVE_Walking,// NewMovementMode
        true,                       // bRestrictSpeedToExpected
        nullptr,                    // PathOffsetCurve
        bZeroVelocityOnFinish
            ? ERootMotionFinishVelocityMode::SetVelocity
            : ERootMotionFinishVelocityMode::MaintainLastRootMotionVelocity,
        FVector::ZeroVector,        // SetVelocityOnFinish
        0.f                         // ClampVelocityOnFinish
    );

    KnockbackTask->OnTimedOut.AddDynamic(this, &UGA_Knockback::OnKnockbackFinished);
    KnockbackTask->OnTimedOutAndDestinationReached.AddDynamic(this, &UGA_Knockback::OnKnockbackFinished);
    KnockbackTask->ReadyForActivation();

    // ---- 播放受击动画（与击退物理同步，不等待动画结束---
    // 击退方向TargetChar 被推离的方向，取反即指向攻击
    // Dot > 0：攻击者在正面 Front 0：攻击者在背面 Back
    const float HitDot = FVector::DotProduct(
        TargetChar->GetActorForwardVector(), (-KnockbackDir).GetSafeNormal());
    const FGameplayTag HitTag = (HitDot >= 0.f)
        ? FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Front"))
        : FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Back"));

    UAnimMontage* HitMontage = nullptr;
    if (AYogCharacterBase* YogChar = Cast<AYogCharacterBase>(TargetChar))
    {
        UCharacterData* CharData = YogChar->CharacterDataComponent->GetCharacterData();
        if (CharData)
        {
            const UAbilityData* AbilityData = CharData->GetAbilityData();
            if (AbilityData)
            {
                HitMontage = AbilityData->GetPassiveAbility(HitTag).Montage;
            }
        }
    }

    if (HitMontage)
    {
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, HitMontage, 1.0f,
            NAME_None,
            true); // bStopWhenAbilityEnds=true，击退结束时自动中断动
        MontageTask->ReadyForActivation();
    }
}

void UGA_Knockback::OnKnockbackFinished()
{
    constexpr bool bReplicateEndAbility = true;
    constexpr bool bWasCancelled        = false;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo,
               bReplicateEndAbility, bWasCancelled);
}

void UGA_Knockback::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (MontageTask)
    {
        MontageTask->EndTask();
        MontageTask = nullptr;
    }

    if (KnockbackTask)
    {
        KnockbackTask->EndTask();
        KnockbackTask = nullptr;
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
