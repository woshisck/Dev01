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

    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] ActivateAbility on %s"), *GetNameSafe(ActorInfo->AvatarActor.Get()));

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(ActorInfo->AvatarActor.Get());
    if (!Character)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

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
            DeathMontage      = DeadData.Montage;
            CachedDissolveTag = DeadData.DissolveGameplayCueTag;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] DeathMontage=%s DissolveTag=%s"),
        *GetNameSafe(DeathMontage), *CachedDissolveTag.ToString());

    if (DeathMontage)
    {
        // ---- 有蒙太奇：先播放死亡蒙太奇，再取消其他技能 ----
        // 顺序很重要：先播放死亡蒙太奇（让它抢占 AnimInstance），再 Cancel 其他 GA，
        // 避免 Cancel → BT推进 → 攻击 GA 立刻覆盖死亡蒙太奇 的竞态。
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, DeathMontage, 1.0f,
            NAME_None,
            false);  // bStopWhenAbilityEnds=false，EndAbility 不中断蒙太奇

        MontageTask->OnCompleted.AddDynamic(this,    &UGA_Dead::OnDeathMontageCompleted);
        MontageTask->OnBlendOut.AddDynamic(this,     &UGA_Dead::OnDeathMontageBlendOut);
        MontageTask->OnCancelled.AddDynamic(this,    &UGA_Dead::OnDeathMontageCancelled);
        // 被其他蒙太奇强行打断时触发 OnInterrupted（不是 OnCancelled），必须绑定否则死亡流程卡住
        MontageTask->OnInterrupted.AddDynamic(this,  &UGA_Dead::OnDeathMontageCancelled);
        MontageTask->ReadyForActivation();
        UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] MontageTask started"));

        // 死亡蒙太奇已抢占 AnimInstance，再取消其他技能
        ActorInfo->AbilitySystemComponent->CancelAllAbilities(this);
    }
    else
    {
        // ---- 无蒙太奇：取消其他技能后直接进入销毁流程 ----
        ActorInfo->AbilitySystemComponent->CancelAllAbilities(this);
        UE_LOG(LogTemp, Warning, TEXT("GA_Dead [%s]: 未找到死亡蒙太奇（LookupTag=%s），直接进入销毁流程"),
            *GetNameSafe(Character), *LookupTag.ToString());
        StartDeathDelay();
    }
}

// 蒙太奇正常完成 → 开始销毁流程
void UGA_Dead::OnDeathMontageCompleted()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathMontageCompleted"));
    StartDeathDelay();
}

// 蒙太奇 BlendOut（视为完成）→ 冻结 Mesh 保持死亡姿势，再开始销毁流程
void UGA_Dead::OnDeathMontageBlendOut()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathMontageBlendOut"));

    // BlendOut 开始时角色仍处于死亡动画最后一帧，立即冻结 Mesh 防止状态机把 idle 混进来
    if (CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid())
    {
        if (ACharacter* Char = Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get()))
        {
            if (USkeletalMeshComponent* Mesh = Char->GetMesh())
            {
                Mesh->bPauseAnims = true;
            }
        }
    }

    StartDeathDelay();
}

// 蒙太奇被取消（异常路径）→ 跳过延迟，立即销毁
void UGA_Dead::OnDeathMontageCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathMontageCancelled"));
    OnDeathDelayExpired();
}

void UGA_Dead::StartDeathDelay()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] StartDeathDelay: DissolveTag=%s TimerAlreadySet=%d"),
        *CachedDissolveTag.ToString(), (int32)DeathDelayTimer.IsValid());

    // OnBlendOut 和 OnCompleted 都会回调此函数，只启动一次
    UWorld* World = GetWorld();
    if (!World || DeathDelayTimer.IsValid()) return;

    if (!CachedDissolveTag.IsValid())
    {
        // 无消解效果 → 等待 3 秒后销毁
        World->GetTimerManager().SetTimer(
            DeathDelayTimer,
            this,
            &UGA_Dead::OnDeathDelayExpired,
            3.0f,
            false);
        return;
    }

    // ---- 有消解效果：立即触发 GC（此时角色仍存活，消解在活体角色上播放），等 2s 后销毁 ----
    if (CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid())
    {
        AActor* Avatar = CurrentActorInfo->AvatarActor.Get();
        FGameplayCueParameters CueParams;
        CueParams.Location     = Avatar->GetActorLocation();
        CueParams.Normal       = Avatar->GetActorForwardVector();
        CueParams.SourceObject = Avatar;
        CurrentActorInfo->AbilitySystemComponent->ExecuteGameplayCue(CachedDissolveTag, CueParams);
    }

    World->GetTimerManager().SetTimer(
        DeathDelayTimer,
        this,
        &UGA_Dead::OnDeathDelayExpired,
        2.0f,
        false);
}

void UGA_Dead::OnDeathDelayExpired()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathDelayExpired -> FinishDying on %s"),
        *GetNameSafe(CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr));

    if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return;
    }

    AYogCharacterBase* Char = Cast<AYogCharacterBase>(CurrentActorInfo->AvatarActor.Get());

    // 先结束 GA（移除 Buff.Status.Dead 等 tag），再销毁角色
    // 顺序很重要：若在 GAS 回调链中途（如 OnInterrupted）同步调用 Destroy()，
    // 会导致调用栈中的 ActivateAbility 访问已销毁对象崩溃
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

    // 推迟一帧销毁，确保当前 GAS 调用栈完全退出后再销毁 Actor
    if (Char && IsValid(Char))
    {
        TWeakObjectPtr<AYogCharacterBase> WeakChar = Char;
        if (UWorld* World = Char->GetWorld())
        {
            World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(
                Char, [WeakChar]()
                {
                    if (WeakChar.IsValid())
                        WeakChar->FinishDying();
                }));
        }
    }
}

void UGA_Dead::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] EndAbility bWasCancelled=%d on %s"),
        (int32)bWasCancelled, *GetNameSafe(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr));

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
