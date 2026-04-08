#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "BuffFlow/BuffFlowTypes.h"
#include "Data/RuneDataAsset.h"
#include "GEConfigDataAsset.generated.h"

/**
 * 属性修改 GE 的配置 DataAsset。
 * 将原本内联在 BFNode_ApplyAttributeModifier 节点上的参数提取到可复用的资产文件。
 *
 * 用法：
 *   1. 在内容浏览器右键 → Miscellaneous → Data Asset → GEConfigDataAsset
 *   2. 填写 Attribute、ModOp、DefaultValue、DurationType、Stacking 等字段
 *   3. 在 FA 中使用 "Apply Modifier (DA)" 节点并引用此资产
 *
 * DefaultValue 可被 FA 节点的 Value 数据引脚覆盖（有连线时以引脚值为准）。
 * 多个节点 / 多个符文可共享同一 DA，避免重复填写相同配置。
 */
UCLASS(BlueprintType)
class DEVKIT_API UGEConfigDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ─── 属性修改 ────────────────────────────────────────────────

	/** 要修改的属性（下拉选择，来自各 AttributeSet） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	FGameplayAttribute Attribute;

	/** 运算类型：Additive（加）/ Multiplicative（乘）/ Override（覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier")
	TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

	/**
	 * 默认修改数值。
	 * FA 节点 Value 数据引脚有连线时以引脚值为准；无连线时使用此值。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Modifier", meta = (DisplayName = "Default Value"))
	float DefaultValue = 0.f;

	// ─── 持续时间 ────────────────────────────────────────────────

	/** 持续类型：瞬发 / 永久 / 有时限 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration")
	ERuneDurationType DurationType = ERuneDurationType::Instant;

	/** 持续秒数（仅 DurationType = 有时限 时生效） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Duration",
		meta = (EditCondition = "DurationType == ERuneDurationType::Duration",
				EditConditionHides, ClampMin = "0.01"))
	float Duration = 1.f;

	// ─── 堆叠控制 ────────────────────────────────────────────────

	/** 堆叠模式：None / Unique / Stackable */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stacking")
	EBFGEStackMode StackMode = EBFGEStackMode::None;

	/** 最大堆叠层数（Stackable 时生效，0 = 不限） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stacking",
		meta = (EditCondition = "StackMode == EBFGEStackMode::Stackable",
				EditConditionHides, ClampMin = "0"))
	int32 StackLimitCount = 3;

	/** 重复命中时是否刷新持续时间（Unique / Stackable + Duration 时有效） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stacking",
		meta = (EditCondition = "StackMode != EBFGEStackMode::None", EditConditionHides))
	EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy =
		EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

	/** 层数到期时的处理策略（Stackable 时有效） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Stacking",
		meta = (EditCondition = "StackMode == EBFGEStackMode::Stackable", EditConditionHides))
	EGameplayEffectStackingExpirationPolicy StackExpirationPolicy =
		EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

	// ─── 标签 ────────────────────────────────────────────────────

	/** 附加到 GE Spec 的动态 Asset Tags（用于触发条件逻辑，如 Heat Phase 升阶） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags")
	FGameplayTagContainer DynamicAssetTags;

	/**
	 * 透传 Owner 标签：运行时若 BuffOwner 身上有这些 Tag，自动附加到 GE Spec。
	 * 无需额外分支节点即可实现条件触发。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Tags",
		meta = (DisplayName = "Pass Through Owner Tags"))
	FGameplayTagContainer PassThroughOwnerTags;
};
