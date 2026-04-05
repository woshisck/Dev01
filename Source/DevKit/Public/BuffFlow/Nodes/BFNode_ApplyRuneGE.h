#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "BFNode_ApplyRuneGE.generated.h"

class URuneDataAsset;

/**
 * 将符文 DA 的 RuneConfig 构建为 GE 并施加到目标，FA 停止时自动移除。
 *
 * 职责：
 *   GE 的生命周期完全由此节点管理，BackpackGrid 只负责启停 FA，
 *   不再直接 apply / remove GE。
 *
 * 使用方式：
 *   在 FA 里根据触发时机选择前置节点：
 *
 *   [Start] → [ApplyRuneGE]          ← 激活时立即生效（永久被动类符文）
 *
 *   [OnDamageDealt] → [ApplyRuneGE]  ← 每次命中触发一次 GE（叠层/触发类）
 *                                      GAS 按 DA 的 StackType 自动处理叠层
 *
 * 生命周期：
 *   ExecuteInput → apply GE → 存储 handle + ASC 弱引用
 *   Cleanup()    → RemoveActiveEffectsWithTags(RuneTag) 移除所有层
 *
 * 执行输出引脚：
 *   Out    — GE 施加成功
 *   Failed — 目标无效、无 ASC 或 DA 未配置
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Rune GE", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyRuneGE : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

    /** 符文数据资产（提供 RuneConfig + Effects 以构建 TransientGE） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    TObjectPtr<URuneDataAsset> RuneAsset;

    /** 施加目标（通常为 BuffOwner） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

    /** GE 等级（对应 DA.RuneInstance.Level） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    int32 Level = 1;

protected:
    virtual void ExecuteInput(const FName& PinName) override;

    /** FA 停止时自动调用，移除所有由此节点施加的 GE（含叠层） */
    virtual void Cleanup() override;

private:
    /** 首次施加后存储的 handle（用于单 handle 快速移除） */
    FActiveGameplayEffectHandle GrantedHandle;

    /** ASC 弱引用，Cleanup 时使用 */
    TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
};
