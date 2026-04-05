#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "CharacterData.h"
#include "Data/RuneEffectFragment.h"
#include "RuneDataAsset.generated.h"

class UFlowAsset;
class UAbilitySystemComponent;
class URuneDataAsset;


// ============================================================
//  辅助枚举
// ============================================================

/** Buff 显示类型 */
UENUM(BlueprintType)
enum class ERuneBuffType : uint8
{
    None    UMETA(DisplayName = "无"),
    Buff    UMETA(DisplayName = "增益"),
    Debuff  UMETA(DisplayName = "减益"),
};

/**
 * 堆叠类型
 *   刷新 — 不增加叠加层，重置持续时间（等效于 MaxStack=1 + RefreshOnSuccessfulApplication）
 *   叠加 — 增加叠加层，重置持续时间（StackLimitCount = MaxStack）
 *   禁止 — 不可堆叠，不刷新时间
 */
UENUM(BlueprintType)
enum class ERuneStackType : uint8
{
    Refresh UMETA(DisplayName = "刷新"),
    Stack   UMETA(DisplayName = "叠加"),
    None    UMETA(DisplayName = "禁止"),
};

/**
 * 减层方式（时间到期时）
 *   全部 — 一次性移除所有层
 *   逐一 — 每次只移除一层（并刷新剩余层的持续时间）
 */
UENUM(BlueprintType)
enum class ERuneStackReduceType : uint8
{
    All UMETA(DisplayName = "全部"),
    One UMETA(DisplayName = "逐一"),
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
//  FRuneConfig — 符文核心配置
//
//  对应策划表字段：
//    BuffID / BuffTag / BuffDuration / MaxStack / StackType / StackReduceType
//  以及原子效果数组（Effects），统一在此配置
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneConfig
{
    GENERATED_BODY()

    // ---- 标识 ----

    /** 数值 ID，供策划表引用 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 BuffID = 0;

    /**
     * Buff GP Tag
     * 其他 Buff/Flow 通过此 Tag 查找或移除本 Buff
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag BuffTag;

    // ---- 持续时间 ----

    /**
     * 持续时间（秒）
     *   0  = 瞬发（立即触发一次，不留存）
     *  -1  = 永久（Infinite，需手动移除）
     *  >0  = 有时限
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float BuffDuration = -1.f;

    // ---- 堆叠 ----

    /**
     * 最大堆叠层数（0 和 1 都视为 1 层）
     * 仅 StackType = 叠加 时生效
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackType == ERuneStackType::Stack", EditConditionHides, ClampMin = "1"))
    int32 MaxStack = 1;

    /** 堆叠类型：刷新 / 叠加 / 禁止 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneStackType StackType = ERuneStackType::None;

    /**
     * 减层方式（StackType != 禁止 且有持续时间时生效）
     *   全部 — 到期一次性全部移除
     *   逐一 — 到期每次移除一层
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "StackType != ERuneStackType::None", EditConditionHides))
    ERuneStackReduceType StackReduceType = ERuneStackReduceType::All;

    // ---- 效果（原子功能）----

    /**
     * 效果列表
     * 点击 + 选择效果类型：
     *   Add Attribute Modifier   — 属性修改（Attack +20 / AttSpeed ×1.1）
     *   Add Gameplay Tags        — 激活期间授予 Tag
     *   Trigger Gameplay Ability — 装备时授予被动 GA（携带 Params 字典）
     *   Gameplay Cue             — 音效/特效（高级）
     *   Advanced Modifier        — 直接配置 FGameplayModifierInfo（高级）
     *   Execution Calculation    — GAS ExecutionCalculation（高级）
     */
    UPROPERTY(EditAnywhere, Instanced)
    TArray<TObjectPtr<URuneEffectFragment>> Effects;
};


// ============================================================
//  FRuneFlowConfig — 逻辑层（BuffFlow 可视化逻辑资产）
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneFlowConfig
{
    GENERATED_BODY()

    /**
     * BuffFlow 可视化逻辑资产（FA）
     * 符文激活时自动启动，卸下时自动停止
     * FA 负责：判定时机、触发 GA/粒子/音效等
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UFlowAsset> BuffFlowAsset;
};


// ============================================================
//  FRuneInstance — 运行时符文实例
//
//  DA 编辑器视觉结构：
//    Rune Name / Icon / Description / Buff Type  ← 展示信息
//    Shape                                        ← 背包格子形状
//    ▼ Rune Config   ID / Tag / Duration / Stack / Effects[]
//    ▼ Flow          BuffFlowAsset
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneInstance
{
    GENERATED_BODY()

    // ---- 展示信息（顶层平铺）----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RuneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> RuneIcon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText RuneDescription;

    /** 增益 / 减益 / 无 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneBuffType BuffType = ERuneBuffType::Buff;

    // ---- 背包形状 ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneShape Shape;

    // ---- 核心配置（Duration / Stack / Effects）----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneConfig RuneConfig;

    // ---- 逻辑层（FA）----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneFlowConfig Flow;

    // ---- 运行时数据（不在 DA 里填写）----

    UPROPERTY(BlueprintReadWrite)
    int32 Level = 1;

    UPROPERTY(BlueprintReadWrite)
    FGuid RuneGuid;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<URuneDataAsset> SourceDA;
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

    /** 根据 RuneConfig（Duration/Stack/Effects）构建 TransientGE */
    UGameplayEffect* CreateTransientGE(UObject* Outer) const;
};
