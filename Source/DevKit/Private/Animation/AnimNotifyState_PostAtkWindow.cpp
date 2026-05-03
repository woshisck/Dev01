#include "Animation/AnimNotifyState_PostAtkWindow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Component/BufferComponent.h"
#include "Component/ComboRuntimeComponent.h"

UAnimNotifyState_PostAtkWindow::UAnimNotifyState_PostAtkWindow()
{
    TagToClearOnActionInput = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
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
}

void UAnimNotifyState_PostAtkWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
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
        Buffer->HasBufferedInputSince(EInputCommandType::LightAttack, BeginTime) ||
        Buffer->HasBufferedInputSince(EInputCommandType::HeavyAttack, BeginTime);

    if (bHasAtkInput)
    {
        if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(Character))
        {
            if (Player->ComboRuntimeComponent && Player->ComboRuntimeComponent->HasComboSource())
            {
                return;
            }
        }

        if (TagToClearOnActionInput.IsValid())
        {
            UAbilitySystemComponent* ASC =
                UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner);
            if (ASC && ASC->GetTagCount(TagToClearOnActionInput) > 0)
            {
                ASC->SetLooseGameplayTagCount(TagToClearOnActionInput, 0);
            }
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
