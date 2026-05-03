#include "AbilitySystem/Abilities/GA_HitReaction.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "AIController.h"

UGA_HitReaction::UGA_HitReaction(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // GA 身份标签，同时作为 AbilityData.PassiveMap 的 lookup key
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact")));

    // 受击硬直中，StateConflict 系统可据此 Tag 阻断低优先级技能
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact")));

    // 每次受击独立实例，并发受击各自播放各自的动画
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

    // 监听 Action.HitReact 事件自动激活（FA/C++ 通过 HandleGameplayEvent 发出此 Tag）
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"));
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
}

void UGA_HitReaction::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // ---- 获取动画来源 ----
    AYogCharacterBase* Character = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAnimMontage* HitMontage = nullptr;

    // 根据攻击者相对位置判断受击方向，选择对应的 PassiveMap lookup key
    // 攻击者在目标正面（Dot >= 0）→ Front；攻击者在背面 → Back
    FGameplayTag LookupTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Front")); // 默认正面
    if (TriggerEventData && TriggerEventData->Instigator != nullptr)
    {
        FVector ToInstigator = TriggerEventData->Instigator->GetActorLocation()
            - Character->GetActorLocation();
        ToInstigator.Z = 0.f;
        if (!ToInstigator.IsNearlyZero())
        {
            const float Dot = FVector::DotProduct(
                Character->GetActorForwardVector(), ToInstigator.GetSafeNormal());
            if (Dot < 0.f)
            {
                LookupTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Back"));
            }
        }
    }

    UCharacterData* CharData = Character->CharacterDataComponent->GetCharacterData();
    if (CharData && LookupTag.IsValid())
    {
        const UAbilityData* AbilityData = CharData->GetAbilityData();
        if (AbilityData)
        {
            FPassiveActionData HitData = AbilityData->GetPassiveAbility(LookupTag);
            HitMontage = HitData.Montage;
        }
    }

    if (!HitMontage)
    {
        // 未配置蒙太奇时直接结束（不是错误，视觉上无硬直）
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // ---- 播放受击蒙太奇 ----
    if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
    {
        AIController->StopMovement();
    }

    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, HitMontage, 1.0f);

    MontageTask->OnCompleted.AddDynamic(this,  &UGA_HitReaction::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this,   &UGA_HitReaction::OnMontageBlendOut);
    MontageTask->OnCancelled.AddDynamic(this,  &UGA_HitReaction::OnMontageCancelled);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_HitReaction::OnMontageInterrupted);
    MontageTask->ReadyForActivation();
}

void UGA_HitReaction::OnMontageCompleted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_HitReaction::OnMontageBlendOut()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_HitReaction::OnMontageCancelled()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_HitReaction::OnMontageInterrupted()
{
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_HitReaction::EndAbility(
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

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
