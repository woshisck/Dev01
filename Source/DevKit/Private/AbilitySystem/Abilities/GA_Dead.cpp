#include "AbilitySystem/Abilities/GA_Dead.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagsManager.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"

UGA_Dead::UGA_Dead(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    // GA 身份标签，同时作为 AbilityData.PassiveMap 的 lookup key
    // 编辑器里 AbilityTags 填 Action.Dead，PassiveMap 也用同一个 Tag 作 key
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.Dead")));

    // 死亡期间挂载 Buff.Status.Dead，StateConflict 系统据此取消全部其他 GA
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead")));

    // 死亡只有一个实例，不允许重复激活
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Dead::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // ---- 主动取消当前所有非死亡 GA ----
    // StateConflict 会处理大部分情况，这里作为兜底保证
    // 传入 this 排除自身被取消
    ActorInfo->AbilitySystemComponent->CancelAllAbilities(this);

    // ---- 获取死亡蒙太奇 ----
    // 用自身 AbilityTags 的第一个 Tag 作为 PassiveMap 的 lookup key
    FGameplayTag LookupTag = AbilityTags.IsEmpty() ? FGameplayTag() : AbilityTags.GetByIndex(0);

    UAnimMontage* DeathMontage = nullptr;

    UCharacterData* CharData = Character->CharacterDataComponent->GetCharacterData();
    if (CharData && LookupTag.IsValid())
    {
        const UAbilityData* AbilityData = CharData->GetAbilityData();
        if (AbilityData)
        {
            FPassiveActionData DeadData = AbilityData->GetPassiveAbility(LookupTag);
            DeathMontage = DeadData.Montage;
        }
    }

    if (!DeathMontage)
    {
        // 未配置死亡蒙太奇——打印日志帮助排查，然后依赖 EnemyCharacterBase::SetLifeSpan 延迟销毁
        UE_LOG(LogTemp, Warning, TEXT("GA_Dead [%s]: 未找到死亡蒙太奇（LookupTag=%s），跳过播放"),
            *GetNameSafe(Character), *LookupTag.ToString());
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    // ---- 播放死亡蒙太奇 ----
    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, DeathMontage, 1.0f,
        NAME_None,   // StartSection
        false);      // bStopWhenAbilityEnds = false，避免 EndAbility 时中断蒙太奇

    MontageTask->OnCompleted.AddDynamic(this, &UGA_Dead::OnDeathMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this,  &UGA_Dead::OnDeathMontageBlendOut);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_Dead::OnDeathMontageCancelled);
    MontageTask->ReadyForActivation();
}

void UGA_Dead::OnDeathMontageCompleted()
{
    AYogCharacterBase* Character = Cast<AYogCharacterBase>(CurrentActorInfo->AvatarActor.Get());
    if (Character)
    {
        Character->FinishDying();
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dead::OnDeathMontageBlendOut()
{
    // BlendOut 时视为播放完成，调用 FinishDying
    AYogCharacterBase* Character = Cast<AYogCharacterBase>(CurrentActorInfo->AvatarActor.Get());
    if (Character)
    {
        Character->FinishDying();
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Dead::OnDeathMontageCancelled()
{
    // 被取消时也需要确保 FinishDying 被调用
    AYogCharacterBase* Character = Cast<AYogCharacterBase>(CurrentActorInfo->AvatarActor.Get());
    if (Character)
    {
        Character->FinishDying();
    }
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}

void UGA_Dead::EndAbility(
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
