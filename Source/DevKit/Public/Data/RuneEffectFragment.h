#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "RuneEffectFragment.generated.h"

// ============================================================
//  URuneEffectFragment — 符文效果原子基类
//
//  在 DA_Rune 的 RuneConfig.Effects 数组中添加子类实例，
//  即可无代码地组合出任意 GE 效果。
//
//  所有子类均为 GE 类 Fragment，覆盖 ApplyToGE()，
//  向运行时构建的 TransientGE 注入 Modifier / Tag / Cue 等。
//
//  GA 的授予/撤销由 FA 层的 BFNode_GrantGA 节点负责，
//  不在 DA 层配置。
// ============================================================

UCLASS(Abstract, EditInlineNew, BlueprintType, DefaultToInstanced, CollapseCategories,
    meta = (DisplayName = "Rune Effect"))
class DEVKIT_API URuneEffectFragment : public UObject
{
    GENERATED_BODY()

public:
    /** 向 TransientGE 注入 Modifier / Tag / Cue 等 */
    virtual void ApplyToGE(UGameplayEffect* GE) const {}
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
