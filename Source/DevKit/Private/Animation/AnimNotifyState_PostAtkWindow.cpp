#include "Animation/AnimNotifyState_PostAtkWindow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/BufferComponent.h"

UAnimNotifyState_PostAtkWindow::UAnimNotifyState_PostAtkWindow()
{
    TagToClearOnActionInput = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.CanCombo"), false);
    RecoveryWindowTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Window.PostAttackRecovery"), false);
}

namespace
{
    FGameplayTag GetLegacyCanComboTag()
    {
        return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"), false);
    }

    FGameplayTag GetLegacyPostAttackRecoveryTag()
    {
        return FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.PostAttackRecovery"), false);
    }

    void SetLooseTagCountIfValid(UAbilitySystemComponent* ASC, const FGameplayTag& Tag, int32 Count)
    {
        if (ASC && Tag.IsValid())
        {
            ASC->SetLooseGameplayTagCount(Tag, Count);
        }
    }
}

void UAnimNotifyState_PostAtkWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (UWorld* World = MeshComp ? MeshComp->GetWorld() : nullptr)
    {
        BeginTime = World->GetTimeSeconds();
    }
    bStopRequested = false;

    UAbilitySystemComponent* ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp ? MeshComp->GetOwner() : nullptr);
    if (ASC)
    {
        SetLooseTagCountIfValid(ASC, RecoveryWindowTag, 1);
        SetLooseTagCountIfValid(ASC, GetLegacyPostAttackRecoveryTag(), 1);
    }
}

void UAnimNotifyState_PostAtkWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    UAbilitySystemComponent* ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MeshComp ? MeshComp->GetOwner() : nullptr);
    if (ASC)
    {
        SetLooseTagCountIfValid(ASC, RecoveryWindowTag, 0);
        SetLooseTagCountIfValid(ASC, GetLegacyPostAttackRecoveryTag(), 0);
    }

    Super::NotifyEnd(MeshComp, Animation, EventReference);
}

void UAnimNotifyState_PostAtkWindow::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

    if (bStopRequested || !MeshComp)
        return;

    AActor* Owner = MeshComp->GetOwner();
    AYogCharacterBase* Character = Cast<AYogCharacterBase>(Owner);
    if (!Character)
        return;

    UBufferComponent* Buffer = Character->GetInputBufferComponent();
    if (!Buffer)
        return;

    // ── 攻击输入：清 CanCombo tag，新 GA 激活后通过 GAS 流程停止蒙太奇，此处不干预 ──
    const bool bHasAtkInput =
        Buffer->HasBufferedInputSince(EInputCommandType::Attack, BeginTime) ||
        Buffer->HasBufferedInputSince(EInputCommandType::WeaponSkill, BeginTime) ||
        Buffer->HasBufferedInputSince(EInputCommandType::Skill, BeginTime);

    if (bHasAtkInput)
    {
        UAbilitySystemComponent* ASC =
            UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
        if (ASC)
        {
            SetLooseTagCountIfValid(ASC, TagToClearOnActionInput, 0);
            SetLooseTagCountIfValid(ASC, GetLegacyCanComboTag(), 0);
        }
        return;
    }

    // ── 移动输入：混出当前蒙太奇，GA_PlayMontage::OnMontageBlendOut → EndAbility ──
    if (Buffer->HasBufferedInputSince(EInputCommandType::Move, BeginTime))
    {
        if (UAnimInstance* AnimInst = MeshComp->GetAnimInstance())
        {
            AnimInst->Montage_Stop(MoveBlendOutTime);
            bStopRequested = true;
        }
    }
}

FString UAnimNotifyState_PostAtkWindow::GetNotifyName_Implementation() const
{
    return FString::Printf(TEXT("Post Atk Window | Blend: %.2fs"), MoveBlendOutTime);
}
