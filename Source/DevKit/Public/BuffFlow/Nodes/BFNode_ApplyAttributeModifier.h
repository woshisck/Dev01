#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Abilities/GameplayAbility.h"
#include "UObject/ObjectKey.h"
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
UCLASS(NotBlueprintable, meta = (DisplayName = "Apply Attribute Modifier", Category = "BuffFlow|Effect"))
class DEVKIT_API UBFNode_ApplyAttributeModifier : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

    // 修改属性 — 下拉选择目标属性（来自各 AttributeSet）
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "修改属性"))
    FGameplayAttribute Attribute;

    // 运算类型 — Additive（加）/ Multiplicative（乘）/ Override（覆盖）
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "运算类型"))
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

    // 修改数值 — 无连线时使用直接填写的值；连线时使用数据引脚值
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "修改数值"))
    FFlowDataPinInputProperty_Float Value;

    // 使用卡牌效果倍率 — 勾选后将数值乘以战斗卡 DA 的 EffectMultiplier
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Combat Card", meta = (DisplayName = "使用卡牌效果倍率"))
    bool bUseCombatCardEffectMultiplier = false;

    // 持续时间类型 — Instant/Infinite/HasDuration 三种生命周期
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "持续时间类型"))
    ERuneDurationType DurationType = ERuneDurationType::Instant;

    // 持续秒数 — 仅 HasDuration 时生效；Instant/Infinite 时忽略
    UPROPERTY(EditAnywhere, Category = "BuffFlow",
        meta = (DisplayName = "持续秒数",
                EditCondition = "DurationType == ERuneDurationType::Duration",
                EditConditionHides, ClampMin = "0.01"))
    float Duration = 1.0f;

    // 周期间隔（秒）— 0=关闭；>0 则每隔N秒触发一次（仅 Infinite/HasDuration 时有意义）
    UPROPERTY(EditAnywhere, Category = "BuffFlow",
        meta = (DisplayName = "周期间隔（0=关闭）",
                EditCondition = "DurationType != ERuneDurationType::Instant",
                EditConditionHides, ClampMin = "0.0"))
    float Period = 0.f;

    // 立即触发首次 — Period>0 时，施加时立刻执行一次，而非等待第一个间隔
    UPROPERTY(EditAnywhere, Category = "BuffFlow",
        meta = (DisplayName = "立即触发首次",
                EditCondition = "Period > 0.0", EditConditionHides))
    bool bFireImmediately = false;

    // 施加目标
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "施加目标"))
    EBFTargetSelector Target = EBFTargetSelector::BuffOwner;

    // 动态资产Tags — 添加到 GE Spec 中，可触发特殊逻辑（如连击升阶）
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "动态资产Tags"))
    FGameplayTagContainer DynamicAssetTags;

    // 透传来源Tags — 若 BuffOwner 身上有这些Tag则自动加入GE Spec（无需额外分支节点）
    UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "透传来源Tags"))
    FGameplayTagContainer PassThroughOwnerTags;

    // 授予目标ASC的Tags — GE生效期间授予目标ASC的Tag；GE到期时GAS自动撤销
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Granted", meta = (DisplayName = "授予目标ASC的Tags"))
    FGameplayTagContainer GrantedTagsToASC;

    // 授予GA列表 — GE生效期间动态授予目标ASC的GA；GE到期时自动撤销，无需预授予到角色蓝图
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Granted", meta = (DisplayName = "授予GA列表"))
    TArray<TSubclassOf<UGameplayAbility>> GrantedAbilities;

    // ── 堆叠控制 ──

    // 堆叠模式 — None/Unique/Stackable 三种堆叠策略
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking", meta = (DisplayName = "堆叠模式"))
    EBFGEStackMode StackMode = EBFGEStackMode::None;

    // 最大堆叠层数 — 仅 Stackable 时生效；0=不限层数
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "最大堆叠层数",
                EditCondition = "StackMode == EBFGEStackMode::Stackable",
                EditConditionHides, ClampMin = "0"))
    int32 StackLimitCount = 3;

    // 时间刷新策略 — 重复命中时是否重置持续时间（Unique/Stackable+Duration 时有效）
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "时间刷新策略",
                EditCondition = "StackMode != EBFGEStackMode::None",
                EditConditionHides))
    EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    // 到期清除策略 — 层数到期时一次清空还是逐层减少（Stackable 时有效）
    UPROPERTY(EditAnywhere, Category = "BuffFlow|Stacking",
        meta = (DisplayName = "到期清除策略",
                EditCondition = "StackMode == EBFGEStackMode::Stackable",
                EditConditionHides))
    EGameplayEffectStackingExpirationPolicy StackExpirationPolicy =
        EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

protected:
    virtual void ExecuteInput(const FName& PinName) override;

    /** FA 停止时自动调用，移除非瞬发 GE，取消过期计时器 */
    virtual void Cleanup() override;

private:
    struct FRuntimeGrantState
    {
        FActiveGameplayEffectHandle EffectHandle;
        TArray<FGameplayAbilitySpecHandle> AbilityHandles;
        FTimerHandle ExpiryTimer;
        bool bManualGrantActive = false;
    };

    FActiveGameplayEffectHandle GrantedHandle;
    TWeakObjectPtr<UAbilitySystemComponent> GrantedASC;
    TMap<TObjectKey<UAbilitySystemComponent>, FRuntimeGrantState> RuntimeGrantStates;

    /** 缓存同一个 GE 对象供复用——GAS 堆叠规则依赖相同的 Def 指针 */
    UPROPERTY()
    TObjectPtr<UGameplayEffect> CachedGE;

    /**
     * HasDuration 时用于触发 Expired 引脚的计时器。
     * 每次 In 引脚触发（包括堆叠刷新）都重置倒计时，与 GAS GE 的真实剩余时间保持同步。
     * FA 提前停止时 Cleanup() 清除此计时器，Expired 不会错误触发。
     */
    FTimerHandle ExpiryTimer;

    /**
     * 手动授予给目标 ASC 的 GA handles（随 GE 生命周期管理）。
     * UE5.4 不支持对动态 NewObject GE 使用 GE.GrantedAbilities，改为手动 GiveAbility。
     * ExpiryTimer 到期或 Cleanup() 时调用 ClearAbility 撤销。
     */
    TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

};
