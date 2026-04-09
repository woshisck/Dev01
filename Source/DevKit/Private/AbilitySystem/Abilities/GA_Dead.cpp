#include "AbilitySystem/Abilities/GA_Dead.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagsManager.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"

UGA_Dead::UGA_Dead(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.Dead")));
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead")));
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

    ActorInfo->AbilitySystemComponent->CancelAllAbilities(this);

    // ---- 读取 AbilityData：蒙太奇 + 消解 GC Tag ----
    const FGameplayTag LookupTag = AbilityTags.IsEmpty() ? FGameplayTag() : AbilityTags.GetByIndex(0);
    UAnimMontage* DeathMontage = nullptr;
    CachedDissolveTag = FGameplayTag();

    UCharacterData* CharData = Character->CharacterDataComponent->GetCharacterData();
    if (CharData && LookupTag.IsValid())
    {
        if (const UAbilityData* AbilityData = CharData->GetAbilityData())
        {
            FPassiveActionData DeadData = AbilityData->GetPassiveAbility(LookupTag);
            DeathMontage       = DeadData.Montage;
            CachedDissolveTag  = DeadData.DissolveGameplayCueTag;
        }
    }

    if (DeathMontage)
    {
        // ---- 有蒙太奇：播放，结束后进入 2s 延迟 ----
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, DeathMontage, 1.0f,
            NAME_None,
            false);  // bStopWhenAbilityEnds=false，EndAbility 不中断蒙太奇

        MontageTask->OnCompleted.AddDynamic(this, &UGA_Dead::OnDeathMontageCompleted);
        MontageTask->OnBlendOut.AddDynamic(this,  &UGA_Dead::OnDeathMontageBlendOut);
        MontageTask->OnCancelled.AddDynamic(this, &UGA_Dead::OnDeathMontageCancelled);
        MontageTask->ReadyForActivation();
    }
    else
    {
        // ---- 无蒙太奇：直接进入 2s 延迟 ----
        UE_LOG(LogTemp, Warning, TEXT("GA_Dead [%s]: 未找到死亡蒙太奇（LookupTag=%s），直接等待 2s"),
            *GetNameSafe(Character), *LookupTag.ToString());
        StartDeathDelay();
    }
}

// 蒙太奇正常完成 → 开始 2s 延迟
void UGA_Dead::OnDeathMontageCompleted()
{
    StartDeathDelay();
}

// 蒙太奇 BlendOut（视为完成）→ 开始 2s 延迟
void UGA_Dead::OnDeathMontageBlendOut()
{
    StartDeathDelay();
}

// 蒙太奇被取消（异常路径）→ 跳过延迟，立即销毁
void UGA_Dead::OnDeathMontageCancelled()
{
    OnDeathDelayExpired();
}

void UGA_Dead::StartDeathDelay()
{
    // OnBlendOut 和 OnCompleted 都会回调此函数，只启动一次
    UWorld* World = GetWorld();
    if (!World || DeathDelayTimer.IsValid()) return;

    World->GetTimerManager().SetTimer(
        DeathDelayTimer,
        this,
        &UGA_Dead::OnDeathDelayExpired,
        2.0f,
        false);
}

void UGA_Dead::OnDeathDelayExpired()
{
    if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    AActor* Avatar = CurrentActorInfo->AvatarActor.Get();

    // ---- 触发消解 GameplayCue（在世界坐标生成，不附加到角色，Actor 销毁后继续播放）----
    if (CachedDissolveTag.IsValid())
    {
        FGameplayCueParameters CueParams;
        CueParams.Location       = Avatar->GetActorLocation();
        CueParams.Normal         = Avatar->GetActorForwardVector();
        CueParams.SourceObject   = Avatar;

        CurrentActorInfo->AbilitySystemComponent->ExecuteGameplayCue(CachedDissolveTag, CueParams);
    }

    // ---- 销毁角色 ----
    if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(Avatar))
    {
        Char->FinishDying();
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
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

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DeathDelayTimer);
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
