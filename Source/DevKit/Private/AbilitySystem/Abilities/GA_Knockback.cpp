#include "AbilitySystem/Abilities/GA_Knockback.h"

#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"

UGA_Knockback::UGA_Knockback(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // GA 激活期间自动挂到 ASC 上，结束时自动移除
    // 注：AbilityTags 身份标签不设置——击退可作用于玩家/敌人任意角色，无需命名空间归属
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Buff.Status.Knockback"));

    // 实例化模式：每次激活一个独立实例
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
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
        DbgInstigator ? *DbgInstigator->GetName() : TEXT("NULL (方向将使用默认向后)"));
    // ──────────────────────────────────────────────────────────────────────

    ACharacter* TargetChar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
    if (!TargetChar)
    {
        UE_LOG(LogTemp, Error, TEXT("[GA_Knockback] AvatarActor 不是 ACharacter，提前结束"));
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // ---- 计算击退方向 ----
    // 优先用事件的 Instigator（攻击者）位置推算方向
    FVector KnockbackDir = TargetChar->GetActorForwardVector() * -1.f; // 默认向后

    if (TriggerEventData && TriggerEventData->Instigator != nullptr)
    {
        FVector FromInstigator = TargetChar->GetActorLocation()
            - TriggerEventData->Instigator->GetActorLocation();
        FromInstigator.Z = 0.f; // 水平方向击退，不影响垂直轴

        if (!FromInstigator.IsNearlyZero())
        {
            KnockbackDir = FromInstigator.GetSafeNormal();
        }
    }

    // ---- 计算目标位置 ----
    const FVector StartLocation  = TargetChar->GetActorLocation();
    const FVector TargetLocation = StartLocation + KnockbackDir * KnockbackDistance;

    // ---- 创建 Root Motion Task ----
    // MoveToForce 直接指定目标位置，距离精确可控（单位 cm）
    // 函数签名：(OwningAbility, TaskName, TargetLocation, Duration,
    //            bSetNewMovementMode, NewMovementMode, bRestrictSpeedToExpected,
    //            PathOffsetCurve, VelocityOnFinishMode, SetVelocityOnFinish, ClampVelocityOnFinish)
    KnockbackTask = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
        this,
        NAME_None,
        TargetLocation,
        KnockbackDuration,
        false,                      // bSetNewMovementMode
        EMovementMode::MOVE_Walking,// NewMovementMode（bSetNewMovementMode=false 时忽略）
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

    // ---- 播放受击动画（与击退物理同步，不等待动画结束）----
    // 击退方向是 TargetChar 被推离的方向，取反即指向攻击者
    // Dot > 0：攻击者在正面 → Front；< 0：攻击者在背面 → Back
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
            true); // bStopWhenAbilityEnds=true，击退结束时自动中断动画
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