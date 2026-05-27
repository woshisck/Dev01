#include "Animation/ANS_FinisherTimeDilation.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/YogCharacterBase.h"
#include "Containers/Ticker.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "UI/YogHUD.h"
#include "Visual/TimeDilationVisualSubsystem.h"

namespace
{
FGameplayTag GetANSFinisherTimeDilationQTEOpenTag()
{
    return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.FinisherQTEOpen"));
}

void CloseANSFinisherTimeDilationWindow(TWeakObjectPtr<AYogCharacterBase> WeakCharacter, bool bHidePrompt)
{
    AYogCharacterBase* Character = WeakCharacter.Get();
    if (!Character)
    {
        return;
    }

    if (UWorld* World = Character->GetWorld())
    {
        if (AWorldSettings* WS = World->GetWorldSettings())
        {
            WS->SetTimeDilation(1.0f);
        }
    }

    Character->CustomTimeDilation = 1.0f;

    if (UYogAbilitySystemComponent* ASC = Character->GetASC())
    {
        ASC->SetLooseGameplayTagCount(GetANSFinisherTimeDilationQTEOpenTag(), 0);
    }

    UTimeDilationVisualSubsystem::EndTimeDilationVisual(Character);

    if (bHidePrompt)
    {
        if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
        {
            if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
            {
                HUD->HideFinisherQTEPrompt();
            }
        }
    }
}
}

void UANS_FinisherTimeDilation::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character)
    {
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        return;
    }

    const float SafeDilation = FMath::Clamp(SlowDilation, 0.0f, 1.0f);
    const float PromptDuration = RealTimePauseDuration > 0.0f ? RealTimePauseDuration : TotalDuration;

    // 全局减速
    if (AWorldSettings* WS = World->GetWorldSettings())
    {
        WS->SetTimeDilation(SafeDilation);
    }
    UTimeDilationVisualSubsystem::BeginTimeDilationVisual(Character);

    // 玩家自身用倒数抵消，维持正常速度（"子弹时间"效果）
    Character->CustomTimeDilation = SafeDilation > SMALL_NUMBER ? 1.0f / SafeDilation : 1.0f;

    if (UYogAbilitySystemComponent* ASC = Character->GetASC())
    {
        ASC->SetLooseGameplayTagCount(GetANSFinisherTimeDilationQTEOpenTag(), 1);
    }

    if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
    {
        if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
        {
            HUD->ShowFinisherQTEPrompt(PromptDuration);
        }
    }

    if (PromptDuration > 0.0f)
    {
        TWeakObjectPtr<AYogCharacterBase> WeakCharacter(Character);
        FTSTicker::GetCoreTicker().AddTicker(
            FTickerDelegate::CreateLambda([WeakCharacter](float)
            {
                AYogCharacterBase* TickedCharacter = WeakCharacter.Get();
                UYogAbilitySystemComponent* ASC = TickedCharacter ? TickedCharacter->GetASC() : nullptr;
                if (ASC && ASC->HasMatchingGameplayTag(GetANSFinisherTimeDilationQTEOpenTag()))
                {
                    CloseANSFinisherTimeDilationWindow(WeakCharacter, true);
                }
                return false;
            }),
            PromptDuration);
    }

    // 通知 UI 显示输入提示
    if (PromptShowEventTag.IsValid() && Character->GetASC())
    {
        FGameplayEventData UIPayload;
        UIPayload.Instigator = Character;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, PromptShowEventTag, UIPayload);
    }
}

void UANS_FinisherTimeDilation::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (!MeshComp)
    {
        return;
    }

    AYogCharacterBase* Character = Cast<AYogCharacterBase>(MeshComp->GetOwner());
    if (!Character)
    {
        return;
    }

    UWorld* World = Character->GetWorld();
    if (!World)
    {
        return;
    }

    // 仅在未被 GA 提前恢复时才恢复（玩家确认后 GA 会提前恢复 GlobalTimeDilation = 1.0）
    if (AWorldSettings* WS = World->GetWorldSettings())
    {
        if (!FMath::IsNearlyEqual(WS->GetEffectiveTimeDilation(), 1.0f, 0.01f))
        {
            WS->SetTimeDilation(1.0f);
            Character->CustomTimeDilation = 1.0f;
        }
        else
        {
            // GA已恢复全局时间，仅确保玩家CustomTimeDilation复位
            Character->CustomTimeDilation = 1.0f;
        }
    }

    // 通知 UI 隐藏输入提示
    UTimeDilationVisualSubsystem::EndTimeDilationVisual(Character);

    if (UYogAbilitySystemComponent* ASC = Character->GetASC())
    {
        ASC->SetLooseGameplayTagCount(GetANSFinisherTimeDilationQTEOpenTag(), 0);
    }

    if (APlayerController* PC = Cast<APlayerController>(Character->GetController()))
    {
        if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
        {
            HUD->HideFinisherQTEPrompt();
        }
    }

    if (PromptHideEventTag.IsValid() && Character->GetASC())
    {
        FGameplayEventData UIPayload;
        UIPayload.Instigator = Character;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Character, PromptHideEventTag, UIPayload);
    }
}

FString UANS_FinisherTimeDilation::GetNotifyName_Implementation() const
{
    return FString::Printf(TEXT("Finisher Time Dilation x%.2f"), SlowDilation);
}
