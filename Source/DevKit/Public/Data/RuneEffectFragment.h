#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "RuneEffectFragment.generated.h"

class UAbilitySystemComponent;
class URuneDataAsset;

// ============================================================
//  URuneEffectFragment — 符文效果原子基类
//
//  在 DA_Rune 的 Effects 数组中添加子类实例，
//  即可以无代码方式组合出任意效果。
//
//  子类分两种：
//    GE 类  — 覆盖 ApplyToGE()，向 GE 注入 Modifier / Tag / Cue
//    非GE 类 — 覆盖 OnActivate() / OnDeactivate()，在装备/卸下时执行
// ============================================================

UCLASS(Abstract, EditInlineNew, BlueprintType, DefaultToInstanced, CollapseCategories,
    meta = (DisplayName = "Rune Effect"))
class DEVKIT_API URuneEffectFragment : public UObject
{
    GENERATED_BODY()

public:
    /** GE 类 Fragment：向 TransientGE 注入 Modifier / Tag / Cue 等 */
    virtual void ApplyToGE(UGameplayEffect* GE) const {}

    /**
     * 非GE 类 Fragment：符文装备时调用
     * @return 若授予了 GA，返回其 Handle；否则返回无效 Handle
     */
    virtual FGameplayAbilitySpecHandle OnActivate(UAbilitySystemComponent* ASC, URuneDataAsset* SourceDA) const
    {
        return FGameplayAbilitySpecHandle();
    }

    /** 非GE 类 Fragment：符文卸下时调用（与 OnActivate 返回的 Handle 配对） */
    virtual void OnDeactivate(UAbilitySystemComponent* ASC, const FGameplayAbilitySpecHandle& Handle) const {}
};


// ============================================================
//  URuneEffect_AttributeModifier — 属性修改（固定数值）
//  示例：Attack +20、AttackSpeed ×1.1、MaxHealth = 500
// ============================================================

UCLASS(DisplayName = "Add Attribute Modifier")
class DEVKIT_API URuneEffect_AttributeModifier : public URuneEffectFragment
{
    GENERATED_BODY()

public:
    /** 要修改的属性 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FGameplayAttribute Attribute;

    /** 运算类型：Additive（加）/ Multiplicative（乘）/ Override（覆盖） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

    /** 数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    float Value = 0.f;

    virtual void ApplyToGE(UGameplayEffect* GE) const override;
};


// ============================================================
//  URuneEffect_AddTags — 激活期间授予目标 GameplayTag
//  示例：授予 Character.Status.Poisoned
// ============================================================

UCLASS(DisplayName = "Add Gameplay Tags")
class DEVKIT_API URuneEffect_AddTags : public URuneEffectFragment
{
    GENERATED_BODY()

public:
    /** GE 激活期间授予目标的 Tag（GE 移除时自动撤销） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    FGameplayTagContainer Tags;

    virtual void ApplyToGE(UGameplayEffect* GE) const override;
};


// ============================================================
//  URuneEffect_TriggerGA — 装备时授予被动 GA，卸下时撤销
//  示例：击退 GA、护盾 GA
//  GA 通过 SourceObject（URuneDataAsset*）读取 Params 字典获取参数
// ============================================================

UCLASS(DisplayName = "Trigger Gameplay Ability")
class DEVKIT_API URuneEffect_TriggerGA : public URuneEffectFragment
{
    GENERATED_BODY()

public:
    /** 要授予的 GA 类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TSubclassOf<UGameplayAbility> AbilityClass;

    /**
     * 传给 GA 的参数字典
     * GA 在 OnAbilityAdded 里读取：
     *   Get Current Source Object → Cast To RuneDataAsset → Values → Params
     *   （实际上是这里的 Params，通过 SourceDA 暴露给 GA）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effect")
    TMap<FName, float> Params;

    virtual FGameplayAbilitySpecHandle OnActivate(UAbilitySystemComponent* ASC, URuneDataAsset* SourceDA) const override;
    virtual void OnDeactivate(UAbilitySystemComponent* ASC, const FGameplayAbilitySpecHandle& Handle) const override;
};


// ============================================================
//  URuneEffect_GameplayCue — 触发 GameplayCue（音效/特效）
//  高级用法，通常保持空
// ============================================================

UCLASS(DisplayName = "Gameplay Cue")
class DEVKIT_API URuneEffect_GameplayCue : public URuneEffectFragment
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Effect")
    TArray<FGameplayEffectCue> Cues;

    virtual void ApplyToGE(UGameplayEffect* GE) const override;
};


// ============================================================
//  URuneEffect_Modifier — 高级 GAS Modifier（AttributeBasedFloat / SetByCaller）
//  不熟悉 GAS 的策划保持空即可，用 AttributeModifier 替代
// ============================================================

UCLASS(DisplayName = "Advanced Modifier (GAS)")
class DEVKIT_API URuneEffect_Modifier : public URuneEffectFragment
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Effect")
    FGameplayModifierInfo ModifierInfo;

    virtual void ApplyToGE(UGameplayEffect* GE) const override;
};


// ============================================================
//  URuneEffect_Execution — ExecutionCalculation（高级 GAS）
// ============================================================

UCLASS(DisplayName = "Execution Calculation (GAS)")
class DEVKIT_API URuneEffect_Execution : public URuneEffectFragment
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, Category = "Effect")
    FGameplayEffectExecutionDefinition ExecutionDefinition;

    virtual void ApplyToGE(UGameplayEffect* GE) const override;
};
