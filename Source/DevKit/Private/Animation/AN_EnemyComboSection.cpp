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

    // ── 混出 / 跳节 ───────────────────────────────────────────
    if (BlendOutTime > 0.0f)
    {
        // 立即开始平滑混出当前蒙太奇，不跳节。
        // GA 收到 OnBlendOut → EndAbility → BT Task 返回 Succeeded → BT 决定下一段攻击。
        // 等同于玩家 ANS_PostAtkWindow 里的 Montage_StopWithBlendOut 逻辑。
        // NextSection 在此分支下无效（留空即可）。
        AnimInst->Montage_Stop(BlendOutTime);
    }
    else
    {
        // BlendOutTime=0：立即跳节（原始行为）
        if (!NextSection.IsNone())
        {
            AnimInst->Montage_JumpToSection(NextSection);
        }
    }
}

FString UAN_EnemyComboSection::GetNotifyName_Implementation() const
{
    if (BlendOutTime > 0.0f)
    {
        return FString::Printf(TEXT("ComboSection→BlendOut(%.2f)"), BlendOutTime);
    }
    if (!NextSection.IsNone())
    {
        return FString::Printf(TEXT("ComboSection→%s"), *NextSection.ToString());
    }
    return TEXT("ComboSection");
}
