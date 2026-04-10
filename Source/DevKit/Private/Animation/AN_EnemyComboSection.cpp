#include "Animation/AN_EnemyComboSection.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/YogCharacterBase.h"

void UAN_EnemyComboSection::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
    const FAnimNotifyEventReference& EventReference)
{
    Super::Notify(MeshComp, Animation, EventReference);

    if (!MeshComp) return;

    UAnimInstance* AnimInst = MeshComp->GetAnimInstance();
    if (!AnimInst) return;

    // ── 命中判断 ──────────────────────────────────────────────
    if (bRequireHit)
    {
        AYogCharacterBase* Char = Cast<AYogCharacterBase>(MeshComp->GetOwner());
        bool bHit = Char && Char->bComboHitConnected;
        if (Char) Char->bComboHitConnected = false;  // 每次都重置，为下一节做准备
        if (!bHit) return;
    }

    // ── 跳节 / 混出 ───────────────────────────────────────────
    // 优先判断 NextSection：填了就跳节（连招链），跳节本身是即时的
    if (!NextSection.IsNone())
    {
        AnimInst->Montage_JumpToSection(NextSection);
    }
    else if (BlendOutTime > 0.0f)
    {
        // NextSection 为空 + BlendOutTime>0：平滑结束连招
        // GA 收到 OnBlendOut → EndAbility → BT Task 返回 Succeeded → BT 决定下一段攻击
        AnimInst->Montage_Stop(BlendOutTime);
    }
    // 两者均为空/零：蒙太奇自然播完结束
}

FString UAN_EnemyComboSection::GetNotifyName_Implementation() const
{
    if (!NextSection.IsNone() && BlendOutTime > 0.0f)
    {
        return FString::Printf(TEXT("ComboSection→%s(BlendOut%.2f)"), *NextSection.ToString(), BlendOutTime);
    }
    if (!NextSection.IsNone())
    {
        return FString::Printf(TEXT("ComboSection→%s"), *NextSection.ToString());
    }
    if (BlendOutTime > 0.0f)
    {
        return FString::Printf(TEXT("ComboSection→End(%.2f)"), BlendOutTime);
    }
    return TEXT("ComboSection");
}
