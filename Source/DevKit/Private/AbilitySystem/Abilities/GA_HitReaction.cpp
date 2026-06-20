#include "AbilitySystem/Abilities/GA_HitReaction.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Animation/AnimInstance.h"
#include "Character/YogCharacterBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Component/CharacterDataComponent.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "GameplayTagsManager.h"

namespace
{
    void CancelAbilitiesWithTagIfValid(UAbilitySystemComponent* ASC, const TCHAR* TagName)
    {
        if (!ASC)
        {
            return;
        }

        const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
        if (Tag.IsValid())
        {
            FGameplayTagContainer Tags;
            Tags.AddTag(Tag);
            ASC->CancelAbilities(&Tags);
        }
    }
}

UGA_HitReaction::UGA_HitReaction(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact")));

    ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.HitReact")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Knockback")));
    ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Dead")));

    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

    FAbilityTriggerData TriggerData;
    TriggerData.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact"));
    TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(TriggerData);

    FAbilityTriggerData BlockedTriggerData;
    BlockedTriggerData.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Blocked"));
    BlockedTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(BlockedTriggerData);

    FAbilityTriggerData ParriedTriggerData;
    ParriedTriggerData.TriggerTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Parried"));
    ParriedTriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
    AbilityTriggers.Add(ParriedTriggerData);
}

void UGA_HitReaction::ActivateAbility(
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

    UAnimMontage* HitMontage = nullptr;
    const FGameplayTag LookupTag = ResolveLookupTag(Character, TriggerEventData);
    const bool bParriedReaction = IsParriedReaction(TriggerEventData);

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

    if (bParriedReaction)
    {
        InterruptForParriedReaction(Character);
    }

    if (AAIController* AIController = Cast<AAIController>(Character->GetController()))
    {
        AIController->StopMovement();
    }

    if (!HitMontage)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
        return;
    }

    MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
        this, NAME_None, HitMontage, 1.0f);

    MontageTask->OnCompleted.AddDynamic(this, &UGA_HitReaction::OnMontageCompleted);
    MontageTask->OnBlendOut.AddDynamic(this, &UGA_HitReaction::OnMontageBlendOut);
    MontageTask->OnCancelled.AddDynamic(this, &UGA_HitReaction::OnMontageCancelled);
    MontageTask->OnInterrupted.AddDynamic(this, &UGA_HitReaction::OnMontageInterrupted);
    MontageTask->ReadyForActivation();
}

FGameplayTag UGA_HitReaction::ResolveLookupTag(AYogCharacterBase* Character, const FGameplayEventData* TriggerEventData) const
{
    if (TriggerEventData)
    {
        static const FGameplayTag BlockedTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Blocked"), false);
        static const FGameplayTag ParriedTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Parried"), false);

        if (BlockedTag.IsValid() && TriggerEventData->EventTag.MatchesTagExact(BlockedTag))
        {
            return BlockedTag;
        }
        if (ParriedTag.IsValid() && TriggerEventData->EventTag.MatchesTagExact(ParriedTag))
        {
            return ParriedTag;
        }
    }

    FGameplayTag LookupTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Front"));
    if (Character && TriggerEventData && TriggerEventData->Instigator != nullptr)
    {
        FVector ToInstigator = TriggerEventData->Instigator->GetActorLocation() - Character->GetActorLocation();
        ToInstigator.Z = 0.f;
        if (!ToInstigator.IsNearlyZero())
        {
            const float Dot = FVector::DotProduct(Character->GetActorForwardVector(), ToInstigator.GetSafeNormal());
            if (Dot < 0.f)
            {
                LookupTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Back"));
            }
        }
    }

    return LookupTag;
}

bool UGA_HitReaction::IsParriedReaction(const FGameplayEventData* TriggerEventData) const
{
    if (!TriggerEventData)
    {
        return false;
    }

    static const FGameplayTag ParriedTag = FGameplayTag::RequestGameplayTag(TEXT("Action.HitReact.Parried"), false);
    return ParriedTag.IsValid() && TriggerEventData->EventTag.MatchesTagExact(ParriedTag);
}

void UGA_HitReaction::InterruptForParriedReaction(AYogCharacterBase* Character) const
{
    if (!Character)
    {
        return;
    }

    if (UAbilitySystemComponent* ASC = Character->GetAbilitySystemComponent())
    {
        static const TCHAR* ActionTagNames[] = {
            TEXT("PlayerState.AbilityCast.Attack"),
            TEXT("PlayerState.AbilityCast.WeaponSkill"),
            TEXT("PlayerState.AbilityCast.Special"),
            TEXT("Enemy.Melee.LAtk1"),
            TEXT("Enemy.Melee.LAtk2"),
            TEXT("Enemy.Melee.LAtk3"),
            TEXT("Enemy.Melee.LAtk4"),
            TEXT("Enemy.Melee.HAtk1"),
            TEXT("Enemy.Melee.HAtk2"),
            TEXT("Enemy.Melee.HAtk3"),
            TEXT("Enemy.Melee.HAtk4"),
            TEXT("Enemy.Skill.Skill1"),
            TEXT("Enemy.Skill.Skill2"),
            TEXT("Enemy.Skill.Skill3"),
            TEXT("Enemy.Skill.Skill4"),
        };

        for (const TCHAR* TagName : ActionTagNames)
        {
            CancelAbilitiesWithTagIfValid(ASC, TagName);
        }
    }

    if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
    {
        if (UAnimMontage* ActiveMontage = AnimInstance->GetCurrentActiveMontage())
        {
            AnimInstance->Montage_Stop(ParriedMontageInterruptBlendOutTime, ActiveMontage);
        }
    }
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
