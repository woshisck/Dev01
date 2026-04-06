#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Types/FlowDataPinProperties.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "Data/RuneDataAsset.h"
#include "BFNode_ApplyAttributeModifier.generated.h"

/**
 * 向目标施加单条属性修改，无需 DA 或 GE 资产——数值直接在节点上配置。
 *
 * Value 字段支持两种用法：
 *   1. 直接在节点上填写固定数值（无连线）
 *   2. 连接来自 GetAttribute.CachedValue、GetRuneInfo.StackCount 等数据引脚（运行时动态）
 *
 * DurationType 控制生命周期：
 *   Instant    — 立即触发一次属性变化，无 Cleanup
 *   Infinite   — 持续生效直到 FA 停止，Cleanup() 自动移除
 *   HasDuration — 持续 Duration 秒后自动到期，FA 提前停止时 Cleanup() 移除
 *
 * 执行输出引脚：
 *   Out    — 施加成功
 *   Failed — 目标无效、无 ASC 或属性未配置
 *
 * 典型用法（击杀后临时加速）：
 *   [OnKill] → [ApplyAttributeModifier]
 *                  Attribute    = MoveSpeed
 *                  ModOp        = Additive
 *                  Value        = 200.0
 *                  DurationType = HasDuration
 *                  Duration     = 3.0s
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Gameplay Effect", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyAttributeModifier : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

    /** 要修改的属性（下拉选择，来自各 AttributeSet） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    FGameplayAttribute Attribute;

    /** 运算类型：Additive（加）/ Multiplicative（乘）/ Override（覆盖） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

    /**
     * 修改数值
     * · 无连线：使用节点上直接填写的值
     * · 连线：使用连入的数据引脚值（如 GetAttribute.CachedValue、GetRuneInfo.StackCount）
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    FFlowDataPinInputProperty_Float Value;

    /** 持续时间类型 */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    ERuneDurationType DurationType = ERuneDurationType::Instant;

    /**
     * 持续秒数（仅 HasDuration 时生效）
     * Instant / Infinite 时忽略此字段
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow",
        meta = (EditCondition = "DurationType == ERuneDurationType::Duration",
                EditConditionHides, ClampMin = "0.01"))
    float Duration = 1.0f;

    /** 施加目标 */
    UPROPERTY(EditAnywhere, Category = "BuffFlow")
    EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

    /** 动态 Asset Tags（添加到 GE Spec，用于触发特殊逻辑，如 Action.Combo.LastHit 升阶） */
    UPROPERTY(EditAnywhere, Category = "BuffFlow",
        meta = (DisplayName = "Dynamic Asset Tags"))
    FGameplayTagContainer DynamicAssetTags;

    /**
     * 透传 Owner 标签：运行时检测 BuffOwner 身上是否有这些 Tag，
     * 若有则自动添加到 GE Spec 的 DynamicAssetTags。
     * 典型用途：配置 Action.Combo.LastHit，无需额外分支节点。
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow",
        meta = (DisplayName = "Pass Through Owner Tags"))
    FGameplayTagContainer PassThroughOwnerTags;

    // ── 堆叠控制 ──

    /**
     * 堆叠模式
     *   None      — 每次命中独立一个 GE，不去重
     *   Unique    — 同目标唯一一个实例，重复命中刷新持续时间
     *   Stackable — 同目标共享实例，可叠加层数（配合 Max Stacks）
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "Stack Mode"))
    EBFGEStackMode StackMode = EBFGEStackMode::None;

    /**
     * 最大堆叠层数（仅 Stackable 时生效）
     * 0 = 不限层数
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "Max Stacks",
                EditCondition = "StackMode == EBFGEStackMode::Stackable",
                EditConditionHides, ClampMin = "0"))
    int32 StackLimitCount = 3;

    /**
     * 重复命中时是否重置持续时间（Unique / Stackable + Duration 时有效）
     * RefreshOnSuccessfulApplication = 每次命中刷新整体倒计时
     * NeverRefresh                   = 保持首次应用的剩余时间
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "Duration Refresh",
                EditCondition = "StackMode != EBFGEStackMode::None",
                EditConditionHides))
    EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    /**
     * 层数到期时的处理策略（Stackable 时有效）
     * ClearEntireStack                    = 一次清空所有层
     * RemoveSingleStackAndRefreshDuration = 每次只减一层，剩余层重置时间
     */
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "Expiration Policy",
                EditCondition = "StackMode == EBFGEStackMode::Stackable",
                EditConditionHides))
    EGameplayEffectStackingExpirationPolicy StackExpirationPolicy =
        EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

protected:
    virtual void ExecuteInput(const FName& PinName) override;

    /** FA 停止时自动调用，移除非瞬发 GE */
    virtual void Cleanup() override;

private:
    FActiveGameplayEffectHandle GrantedHandle;
    TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;

    /** 缓存同一个 GE 对象供复用——GAS 堆叠规则依赖相同的 Def 指针 */
    UPROPERTY()
    TObjectPtr<UGameplayEffect> CachedGE;
};
