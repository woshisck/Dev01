#include "Animation/ANS_ComboWindow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagContainer.h"

namespace
{
    const FGameplayTag& CanComboTag()
    {
        static const FGameplayTag Tag =
            FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.CanCombo"));
        return Tag;
    }
}

UAbilitySystemComponent* UANS_ComboWindow::GetASC(USkeletalMeshComponent* MeshComp)
{
    if (!MeshComp) return nullptr;
    AActor* Owner = MeshComp->GetOwner();
    return Owner ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner) : nullptr;
}

void UANS_ComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
    {
        ASC->AddLooseGameplayTag(CanComboTag());
    }
}

void UANS_ComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    // Use SetLooseGameplayTagCount(0) rather than a single Remove so the window
    // is fully closed even if the tag was added more than once (e.g. fast retrigger).
    if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
    {
        ASC->SetLooseGameplayTagCount(CanComboTag(), 0);
    }
}

FString UANS_ComboWindow::GetNotifyName_Implementation() const
{
    return TEXT("Combo Window");
}
