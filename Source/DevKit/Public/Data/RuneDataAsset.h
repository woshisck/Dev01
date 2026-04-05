#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "CharacterData.h"
#include "RuneDataAsset.generated.h"

class UFlowAsset;
class UYogGameplayAbility;
class UAbilitySystemComponent;
class URuneDataAsset;


// ============================================================
//  辅助枚举
// ============================================================

UENUM(BlueprintType)
enum class ERuneBuffType : uint8
{
    None    UMETA(DisplayName = "无"),
    Buff    UMETA(DisplayName = "增益"),
    Debuff  UMETA(DisplayName = "减益"),
};

UENUM(BlueprintType)
enum class ERuneCalcOp : uint8
{
    UseA        UMETA(DisplayName = "仅 A"),
    A_Minus_B   UMETA(DisplayName = "A - B"),
    A_Plus_B    UMETA(DisplayName = "A + B"),
    A_Times_B   UMETA(DisplayName = "A × B"),
};


// ============================================================
//  FRuneAttributeModifier — 简化属性修改器（固定数值）
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneAttributeModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 0.f;
};


// ============================================================
//  FRuneCalcSpec — 属性联动公式
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneCalcSpec
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayAttribute AttributeA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "Operation != ERuneCalcOp::UseA", EditConditionHides))
    FGameplayAttribute AttributeB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneCalcOp Operation = ERuneCalcOp::UseA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Coefficient = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Addend = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGameplayEffectAttributeCaptureSource CaptureSource = EGameplayEffectAttributeCaptureSource::Target;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayAttribute OutputAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;
};


// ============================================================
//  FRuneShape — 背包格子形状
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneShape
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FIntPoint> Cells;

    int32 GetCellCount() const { return Cells.Num(); }
    FRuneShape Rotate90() const;
};


// ============================================================
//  FRuneBuffConfig — Buff 行为配置（Duration / Stacking / Tags）
//  在 DA 里显示为可折叠的 "Buff Config" Section
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneBuffConfig
{
    GENERATED_BODY()

    /** 标识此 Buff 的 Tag（用于 EffectRegistry 查找 / 精确移除） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag BuffTag;

    /** GE 激活期间授予目标的 Tag，移除时自动撤销 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer GrantedTagsToTarget;

    // --- Duration ---

    /** 瞬发(Instant) / 有时限(HasDuration) / 永久(Infinite) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGameplayEffectDurationType DurationPolicy = EGameplayEffectDurationType::Infinite;

    /** Duration（秒），仅 HasDuration 时生效 */
    UPROPERTY(EditAnywhere,
        meta = (EditCondition = "DurationPolicy == EGameplayEffectDurationType::HasDuration", EditConditionHides))
    FGameplayEffectModifierMagnitude DurationMagnitude;

    /** Period（秒）：大于 0 时为周期效果（持续掉血等），0 = 不周期 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "DurationPolicy != EGameplayEffectDurationType::Instant", EditConditionHides))
    FScalableFloat Period;

    /** 施加瞬间是否立即触发一次周期效果 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "DurationPolicy != EGameplayEffectDurationType::Instant", EditConditionHides))
    bool bExecutePeriodicEffectOnApplication = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "DurationPolicy != EGameplayEffectDurationType::Instant", EditConditionHides))
    EGameplayEffectPeriodInhibitionRemovedPolicy PeriodicInhibitionPolicy =
        EGameplayEffectPeriodInhibitionRemovedPolicy::NeverReset;

    // --- Stacking ---

    /** None / AggregateByTarget（按目标叠加）/ AggregateBySource（按来源唯一） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EGameplayEffectStackingType StackingType = EGameplayEffectStackingType::None;

    /** Max Stack（最大堆叠层数） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None", EditConditionHides))
    int32 StackLimitCount = 1;

    /** 叠加时如何处理计时（刷新 / 不刷新） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None", EditConditionHides))
    EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy =
        EGameplayEffectStackingDurationPolicy::RefreshOnSuccessfulApplication;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None", EditConditionHides))
    EGameplayEffectStackingPeriodPolicy StackPeriodResetPolicy =
        EGameplayEffectStackingPeriodPolicy::NeverReset;

    /** 到期移除方式：全部(ClearEntireStack) / 逐一(RemoveSingleStackAndRefreshDuration) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None", EditConditionHides))
    EGameplayEffectStackingExpirationPolicy StackExpirationPolicy =
        EGameplayEffectStackingExpirationPolicy::ClearEntireStack;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None", EditConditionHides))
    TArray<TSubclassOf<UGameplayEffect>> OverflowEffects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None", EditConditionHides))
    bool bDenyOverflowApplication = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackingType != EGameplayEffectStackingType::None && bDenyOverflowApplication", EditConditionHides))
    bool bClearStackOnOverflow = false;
};


// ============================================================
//  FRuneValues — 数值配置（属性修改 / 公式 / GA 参数）
//  在 DA 里显示为可折叠的 "Values" Section
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneValues
{
    GENERATED_BODY()

    /**
     * 简化属性修改器（Attack +20 这类固定数值）
     * 选属性、填数值，无需写代码
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRuneAttributeModifier> AttributeModifiers;

    /**
     * 属性联动公式（多属性计算 → 另一属性）
     * 示例：(MaxHealth - Health) × 0.01 → AttackSpeed Additive
     * GE 施加时快照；如需实时更新，配合 Buff Config 里的 Period 使用
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FRuneCalcSpec> CalcSpecs;

    /**
     * GA 参数字典（向 PassiveAbilityClass 传递数值）
     * Key = 参数名（如 "KnockbackStrength"），Value = 数值
     * GA 读取：GetCurrentAbilitySpec().SourceObject → Cast<URuneDataAsset> → RuneTemplate.Values.Params
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<FName, float> Params;

    /**
     * 高级修改器（AttributeBasedFloat / SetByCaller）
     * 不熟悉 GAS 保持空即可
     */
    UPROPERTY(EditAnywhere)
    TArray<FGameplayModifierInfo> Modifiers;

    /** ExecutionCalculation（高级，通常保持空） */
    UPROPERTY(EditAnywhere)
    TArray<FGameplayEffectExecutionDefinition> Executions;

    /** GameplayCue（音效/特效，通常在蓝图侧处理） */
    UPROPERTY(EditAnywhere)
    TArray<FGameplayEffectCue> GameplayCues;
};


// ============================================================
//  FRuneFlowConfig — 逻辑实现（BuffFlow / 被动 GA）
//  在 DA 里显示为可折叠的 "Flow" Section
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneFlowConfig
{
    GENERATED_BODY()

    /**
     * BuffFlow 可视化逻辑资产
     * 符文激活时自动启动，符文卸下时自动停止
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UFlowAsset> BuffFlowAsset;

    /**
     * 被动能力类（符文激活时授予）
     * GA 通过 GetCurrentAbilitySpec().SourceObject → Cast<URuneDataAsset> 读取 Values.Params
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UYogGameplayAbility> PassiveAbilityClass;
};


// ============================================================
//  FRuneInstance
//  编辑器里 DA 的视觉结构：
//    Rune Name / Rune Icon / Rune Description / Buff Type  ← 展示信息（顶层平铺）
//    Shape                                                 ← 背包形状（顶层平铺）
//    ▼ Buff Config   Duration / Period / Stacking / Tags
//    ▼ Values        属性修改 / 公式 / GA参数
//    ▼ Flow          BuffFlow / PassiveGA
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneInstance
{
    GENERATED_BODY()

    // ---- 展示信息（顶层平铺，一眼可见） ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RuneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> RuneIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText RuneDescription;

    /** 增益 / 减益 / 无，供 UI 显示颜色 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneBuffType BuffType = ERuneBuffType::Buff;

    // ---- 背包形状 ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneShape Shape;

    // ---- 分组 Section（子结构体 → 编辑器里自动折叠展示） ----

    /** Buff 行为配置：Duration / Period / Stacking / Tags */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneBuffConfig BuffConfig;

    /** 数值配置：属性修改 / 公式联动 / GA 参数 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneValues Values;

    /** 逻辑实现：BuffFlow / 被动 GA */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneFlowConfig Flow;

    // ---- 运行时数据（不在 DA 里填写）----

    UPROPERTY(BlueprintReadWrite)
    int32 Level = 1;

    UPROPERTY(BlueprintReadWrite)
    FGuid RuneGuid;

    /** CreateInstance() 时自动设置，供 GA 通过 SourceObject 读取 DA 配置 */
    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<URuneDataAsset> SourceDA;

    // ---- 运行时方法 ----

    UGameplayEffect* CreateTransientGE(UObject* Outer, UAbilitySystemComponent* ASC = nullptr) const;
};


// ============================================================
//  URuneDataAsset
// ============================================================

UCLASS(BlueprintType)
class DEVKIT_API URuneDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rune")
    FRuneInstance RuneTemplate;

    UFUNCTION(BlueprintCallable, Category = "Rune")
    FRuneInstance CreateInstance() const;

    UGameplayEffect* CreateTransientGE(UObject* Outer, UAbilitySystemComponent* ASC = nullptr) const
    {
        return RuneTemplate.CreateTransientGE(Outer, ASC);
    }
};
