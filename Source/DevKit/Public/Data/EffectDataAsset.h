#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ActiveGameplayEffectHandle.h"
#include "Data/RuneDataAsset.h"       // ERuneDurationType / ERuneUniqueType / ERuneStackType 等枚举
#include "Data/RuneEffectFragment.h"  // URuneEffectFragment
#include "EffectDataAsset.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

/**
 * GE 包装 DA（Effect Data Asset）
 *
 * 独立于符文系统，任何地方都可以用。
 * 在编辑器里像填表格一样配置 GE 的持续时间、堆叠、属性修改等，
 * 运行时通过 CreateGameplayEffect() 构建 TransientGE，
 * 或直接用 ApplyTo() 施加到目标。
 *
 * 编辑器分组顺序（策划视角）：
 *   1. Duration  → 这个效果持续多久
 *   2. Stacking  → 多次施加时如何叠加
 *   3. Identity  → 用什么 Tag 标识自己（供查询/移除用）
 *   4. Effects   → 具体做什么（属性改变、Tag 授予等）
 */
UCLASS(BlueprintType)
class DEVKIT_API UEffectDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ─── 1. Duration ──────────────────────────────────────────

	/** 持续时间类型 */
	UPROPERTY(EditAnywhere, Category = "1. Duration")
	ERuneDurationType DurationType = ERuneDurationType::Infinite;

	/**
	 * 持续时长（秒），仅 Duration 类型时生效
	 */
	UPROPERTY(EditAnywhere, Category = "1. Duration",
		meta = (EditCondition = "DurationType == ERuneDurationType::Duration", EditConditionHides, ClampMin = "0.01"))
	float Duration = 5.f;

	/**
	 * 周期触发间隔（秒）
	 * 0 = 不触发周期效果
	 * > 0 = 每隔 N 秒执行一次 Effects（DoT / HoT 场景）
	 */
	UPROPERTY(EditAnywhere, Category = "1. Duration", meta = (ClampMin = "0.0"))
	float Period = 0.f;

	// ─── 2. Stacking ──────────────────────────────────────────

	/** 唯一性：决定同一效果被多次施加时的聚合方式 */
	UPROPERTY(EditAnywhere, Category = "2. Stacking")
	ERuneUniqueType UniqueType = ERuneUniqueType::Unique;

	/** 堆叠类型：刷新 / 叠加 / 禁止（UniqueType 非"非唯一"时生效） */
	UPROPERTY(EditAnywhere, Category = "2. Stacking",
		meta = (EditCondition = "UniqueType != ERuneUniqueType::NonUnique", EditConditionHides))
	ERuneStackType StackType = ERuneStackType::Refresh;

	/** 最大叠加层数（StackType = 叠加 时生效） */
	UPROPERTY(EditAnywhere, Category = "2. Stacking",
		meta = (EditCondition = "UniqueType != ERuneUniqueType::NonUnique && StackType == ERuneStackType::Stack",
			EditConditionHides, ClampMin = "1"))
	int32 MaxStack = 1;

	/** 到期减层方式（全部移除 / 逐一移除） */
	UPROPERTY(EditAnywhere, Category = "2. Stacking",
		meta = (EditCondition = "UniqueType != ERuneUniqueType::NonUnique && StackType != ERuneStackType::None",
			EditConditionHides))
	ERuneStackReduceType StackReduceType = ERuneStackReduceType::All;

	// ─── 3. Identity ──────────────────────────────────────────

	/**
	 * 效果身份 Tag（可选）
	 * 设置后可通过 GetActiveEffectsWithAllTags / HasTag 等节点按此 Tag 查询或移除效果。
	 * 不设置则效果匿名，无法通过 Tag 精确查找。
	 */
	UPROPERTY(EditAnywhere, Category = "3. Identity")
	FGameplayTag EffectTag;

	// ─── 4. Effects ───────────────────────────────────────────

	/**
	 * 效果列表（点 + 选择类型）：
	 *   Add Attribute Modifier  — 属性修改（ATK +20 / AttSpeed ×1.1）
	 *   Add Gameplay Tags       — 授予目标状态 Tag（如 Status.Poisoned）
	 *   Gameplay Cue            — 音效/特效
	 *   Advanced Modifier (GAS) — 直接配置 FGameplayModifierInfo（高级）
	 *   Execution Calculation   — GAS ExecutionCalculation（高级）
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "4. Effects")
	TArray<TObjectPtr<URuneEffectFragment>> Effects;

	// ─── 运行时接口 ───────────────────────────────────────────

	/**
	 * 构建 TransientGE（不缓存，每次调用都新建）
	 * 通常不需要手动调用，使用 ApplyToSelf / ApplyToTarget 更方便
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	UGameplayEffect* CreateGameplayEffect(UObject* Outer = nullptr) const;

	/**
	 * 施加到自身（Self buff / debuff）
	 * @param ASC   目标 AbilitySystemComponent
	 * @param Level GE 等级，影响 ScalableFloat 缩放
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	FActiveGameplayEffectHandle ApplyToSelf(UAbilitySystemComponent* ASC, float Level = 1.f) const;

	/**
	 * 以 Source 为来源，施加到 Target（跨角色 buff / debuff）
	 * @param SourceASC 施加者（GE 来源，影响 SetByCaller 等）
	 * @param TargetASC 接受者
	 * @param Level     GE 等级
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	FActiveGameplayEffectHandle ApplyToTarget(
		UAbilitySystemComponent* SourceASC,
		UAbilitySystemComponent* TargetASC,
		float Level = 1.f) const;
};
