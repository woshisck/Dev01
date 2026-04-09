#include "Animation/AN_EnemyComboSection.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_EnemyComboSection::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp || NextSection.IsNone()) return;

    UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
    if (!AnimInst) return;

    AnimInst->Montage_JumpToSection(NextSection);
}

FString UAN_EnemyComboSection::GetNotifyName_Implementation() const
{
    if (!NextSection.IsNone())
    {
        return FString::Printf(TEXT("ComboSection→%s"), *NextSection.ToString());
    }
    return TEXT("ComboSection");
}
