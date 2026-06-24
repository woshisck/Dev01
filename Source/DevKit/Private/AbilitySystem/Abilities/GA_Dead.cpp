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
    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead")));
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    // 监听 Action.Dead 事件自动激活（YogCharacterBase::Die() 在血量归零时发送此 Tag
    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Action.Dead"));
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);
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

    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] Resolved death data Character=%s CharData=%s DeathMontage=%s"),
        *GetNameSafe(Character),
        *GetNameSafe(CharData),
        *GetNameSafe(DeathMontage));

    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] DeathMontage=%s DissolveTag=%s"),
        *GetNameSafe(DeathMontage), *CachedDissolveTag.ToString());

    if (DeathMontage)
    {
        // ---- 有蒙太奇：先播放死亡蒙太奇，再取消其他技----
        // 顺序很重要：先播放死亡蒙太奇（让它抢AnimInstance），Cancel 其他 GA
        // 避免 Cancel BT推进 攻击 GA 立刻覆盖死亡蒙太的竞态
        MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, DeathMontage, 1.0f,
            NAME_None,
            false);  // bStopWhenAbilityEnds=false，EndAbility 不中断蒙太奇

        MontageTask->OnCompleted.AddDynamic(this,    &UGA_Dead::OnDeathMontageCompleted);
        MontageTask->OnBlendOut.AddDynamic(this,     &UGA_Dead::OnDeathMontageBlendOut);
        MontageTask->OnCancelled.AddDynamic(this,    &UGA_Dead::OnDeathMontageCancelled);
        // 被其他蒙太奇强行打断时触OnInterrupted（不OnCancelled），必须绑定否则死亡流程卡住
        MontageTask->OnInterrupted.AddDynamic(this,  &UGA_Dead::OnDeathMontageCancelled);
        MontageTask->ReadyForActivation();
        UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] MontageTask started"));

        // 死亡蒙太奇已抢占 AnimInstance，再取消其他技
        ActorInfo->AbilitySystemComponent->CancelAllAbilities(this);
    }
    else
    {
        // ---- 无蒙太奇：取消其他技能后直接进入销毁流----
        ActorInfo->AbilitySystemComponent->CancelAllAbilities(this);
        UE_LOG(LogTemp, Warning, TEXT("GA_Dead [%s]: 未找到死亡蒙太奇（LookupTag=%s），直接进入销毁流"),
            *GetNameSafe(Character), *LookupTag.ToString());
        StartDeathDelay();
    }
}

void UGA_Dead::OnDeathMontageCompleted()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathMontageCompleted"));
    StartDeathDelay();
}

// 蒙太BlendOut（视为完成）冻结 Mesh 保持死亡姿势，再开始销毁流
void UGA_Dead::OnDeathMontageBlendOut()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathMontageBlendOut"));

    // BlendOut 开始时角色仍处于死亡动画最后一帧，立即冻结 Mesh 防止状态机idle 混进
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

// 蒙太奇被取消（异常路径）跳过延迟，立即销
void UGA_Dead::OnDeathMontageCancelled()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] OnDeathMontageCancelled"));
    OnDeathDelayExpired();
}

void UGA_Dead::StartDeathDelay()
{
    UE_LOG(LogTemp, Warning, TEXT("[GA_Dead] StartDeathDelay: DissolveTag=%s TimerAlreadySet=%d"),
        *CachedDissolveTag.ToString(), (int32)DeathDelayTimer.IsValid());

    // OnBlendOut OnCompleted 都会回调此函数，只启动一
    UWorld* World = GetWorld();
    if (!World || DeathDelayTimer.IsValid()) return;

    AActor* Avatar = CurrentActorInfo && CurrentActorInfo->AvatarActor.IsValid()
        ? CurrentActorInfo->AvatarActor.Get()
        : nullptr;
    const AYogCharacterBase* Character = Cast<AYogCharacterBase>(Avatar);
    const bool bHasDissolveCue = CachedDissolveTag.IsValid();
    const float DefaultDisappearDelay = bHasDissolveCue ? 2.0f : 3.0f;
    const float DisappearDelay = FMath::Max(
        0.001f,
        Character ? Character->GetDeathDisappearDelayAfterAnimation(bHasDissolveCue) : DefaultDisappearDelay);

    // 广播死亡动画完成事件（供 FA_EnemyBuff_DeathPoison 等监听毒液溅射时机）
    if (Avatar)
    {
        static const FGameplayTag TAG_DeathAnimComplete =
            FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.DeathAnimComplete"), false);
        if (TAG_DeathAnimComplete.IsValid())
        {
            FGameplayEventData DeathAnimPayload;
            DeathAnimPayload.Instigator = Avatar;
            DeathAnimPayload.Target     = Avatar;
            UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, TAG_DeathAnimComplete, DeathAnimPayload);
        }
    }

    if (!bHasDissolveCue)
    {
        // No dissolve cue: wait the character-specific disappear delay before destroying.
        World->GetTimerManager().SetTimer(
            DeathDelayTimer,
            this,
            &UGA_Dead::OnDeathDelayExpired,
            DisappearDelay,
            false);
        return;
    }

    // Trigger dissolve immediately, then wait the character-specific disappear delay before destroying.
    if (Avatar)
    {
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
        DisappearDelay,
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

    // 先结GA（移Buff.Dead tag），再销毁角
    // 顺序很重要：若在 GAS 回调链中途（OnInterrupted）同步调Destroy()
    // 会导致调用栈中的 ActivateAbility 访问已销毁对象崩
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);

    // 推迟一帧销毁，确保当前 GAS 调用栈完全退出后再销Actor
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
