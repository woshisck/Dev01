#include "Animation/AnimNotifyState_AddGameplayTag.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

UAbilitySystemComponent* UAnimNotifyState_AddGameplayTag::GetASC(USkeletalMeshComponent* MeshComp)
{
    if (!MeshComp) return nullptr;
    AActor* Owner = MeshComp->GetOwner();
    return Owner ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner) : nullptr;
}

void UAnimNotifyState_AddGameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

    if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
    {
        ASC->AddLooseGameplayTags(Tags);
    }
}

void UAnimNotifyState_AddGameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp,
    UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
    Super::NotifyEnd(MeshComp, Animation, EventReference);

    if (UAbilitySystemComponent* ASC = GetASC(MeshComp))
    {
        ASC->RemoveLooseGameplayTags(Tags);
    }
}

FString UAnimNotifyState_AddGameplayTag::GetNotifyName_Implementation() const
{
    if (Tags.IsEmpty())
        return TEXT("Add Tag: (none)");
    // 只显示第一个 tag 名，保持编辑器简洁
    return FString::Printf(TEXT("Add Tag: %s"), *Tags.First().ToString());
}
