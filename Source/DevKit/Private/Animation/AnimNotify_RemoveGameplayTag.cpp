#include "Animation/AnimNotify_RemoveGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

void UAnimNotify_RemoveGameplayTag::Notify(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!TagToRemove.IsValid() || !MeshComp)
        return;

    AActor* Owner = MeshComp->GetOwner();
    UAbilitySystemComponent* ASC = Owner
        ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner)
        : nullptr;
    if (!ASC)
        return;

    const int32 Count = ASC->GetTagCount(TagToRemove);
    if (Count > 0)
    {
        ASC->SetLooseGameplayTagCount(TagToRemove, 0);
    }
}

FString UAnimNotify_RemoveGameplayTag::GetNotifyName_Implementation() const
{
    if (!TagToRemove.IsValid())
        return TEXT("Remove Tag: (none)");
    return FString::Printf(TEXT("Remove Tag: %s"), *TagToRemove.ToString());
}
