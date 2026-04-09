#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_EnemyComboSection.generated.h"

/**
 * AN_EnemyComboSection
 *
 * 敌人连击蒙太奇节跳转 Notify。
 * 在攻击蒙太奇每个连击段的末尾放置此 Notify，自动跳转到下一个节（Section），
 * 实现"多段连击通过单一蒙太奇完成"的效果。
 *
 * 用法：
 *   1. 在攻击蒙太奇中创建多个节，如 Atk1、Atk2、Atk3
 *   2. 在 Atk1 快结束时放本 Notify，设 NextSection = Atk2
 *   3. 在 Atk2 快结束时放本 Notify，设 NextSection = Atk3
 *   4. Atk3 末尾不放 Notify → 蒙太奇自然结束 → GA 结束 → BT Task 返回 Succeeded
 *
 * NextSection 留空时 Notify 不执行任何操作（安全降级）。
 */
UCLASS()
class DEVKIT_API UAN_EnemyComboSection : public UAnimNotify
{
    GENERATED_BODY()

public:
    // 跳转到的下一个蒙太奇节名称，留空则不跳节
    UPROPERTY(EditAnywhere, Category = "Combo")
    FName NextSection;

    virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
        const FAnimNotifyEventReference& EventReference) override;

    virtual FString GetNotifyName_Implementation() const override;
};
