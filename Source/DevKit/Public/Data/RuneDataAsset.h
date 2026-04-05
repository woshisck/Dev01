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

/** 符文类型（增益/减益/无，用于 UI 分类显示） */
UENUM(BlueprintType)
enum class ERuneType : uint8
{
    None    UMETA(DisplayName = "无"),
    Buff    UMETA(DisplayName = "增益"),
    Debuff  UMETA(DisplayName = "减益"),
};

/**
 * 持续时间类型
 *   瞬发  — 立即触发一次，不留存（Instant GE）
 *   永久  — 无限持续，需手动移除（Infinite GE）
 *   有时限 — 按 Duration 字段设置的秒数持续（HasDuration GE）
 */
UENUM(BlueprintType)
enum class ERuneDurationType : uint8
{
    Instant  UMETA(DisplayName = "瞬发"),
    Infinite UMETA(DisplayName = "永久"),
    Duration UMETA(DisplayName = "有时限"),
};

/**
 * 唯一性类型（决定多个来源施加同名 GE 时的聚合方式）
 *   唯一   — 同目标只存在一个实例（AggregateByTarget），配合 StackType 使用
 *   源唯一 — 每个施加者在目标上各有一个实例（AggregateBySource）
 *   非唯一 — 每次施加都是独立 GE 实例（None），StackType 不生效
 */
UENUM(BlueprintType)
enum class ERuneUniqueType : uint8
{
    Unique    UMETA(DisplayName = "唯一"),
    BySource  UMETA(DisplayName = "源唯一"),
    NonUnique UMETA(DisplayName = "非唯一"),
};

/**
 * 堆叠类型（UniqueType 为唯一/源唯一时生效）
 *   刷新 — 不增加叠加层，重置持续时间
 *   叠加 — 增加叠加层（至 MaxStack），重置持续时间
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
 * 减层方式（到期时）
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
//  对应策划表字段（严格按图）：
//    Type / Duration Type / Duration / Period
//    Unique Type / Max Stack / Stack Type / Stack Reduce Type
//    RuneID / RuneTag / Effects[]
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneConfig
{
    GENERATED_BODY()

    // ---- 类型 ----

    /** 增益 / 减益 / 无（仅作展示分类，不影响 GE 构建） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneType RuneType = ERuneType::Buff;

    // ---- 持续时间 ----

    /** 持续时间类型：瞬发 / 永久 / 有时限 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneDurationType DurationType = ERuneDurationType::Infinite;

    /**
     * 持续时间（秒），仅 DurationType = 有时限 时生效
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "DurationType == ERuneDurationType::Duration", EditConditionHides, ClampMin = "0.01"))
    float Duration = 5.f;

    /**
     * 周期触发间隔（秒）
     * > 0 时 GE 每隔此时间触发一次 Effect（适用于 DoT、持续恢复等）
     * 0 表示不触发周期效果
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
    float Period = 0.f;

    // ---- 唯一性与堆叠 ----

    /**
     * 唯一性类型
     * 决定同名 GE 被多次施加时如何聚合：
     *   唯一   — 目标上只保留一个实例（配合 StackType 使用）
     *   源唯一 — 每个施加者独立一个实例
     *   非唯一 — 每次施加都是独立 GE，StackType 不生效
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneUniqueType UniqueType = ERuneUniqueType::Unique;

    /**
     * 最大堆叠层数（0 和 1 都视为 1 层）
     * 仅 UniqueType != 非唯一 且 StackType = 叠加 时生效
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "UniqueType != ERuneUniqueType::NonUnique && StackType == ERuneStackType::Stack",
                EditConditionHides, ClampMin = "1"))
    int32 MaxStack = 1;

    /** 堆叠类型：刷新 / 叠加 / 禁止（UniqueType != 非唯一 时生效） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "UniqueType != ERuneUniqueType::NonUnique", EditConditionHides))
    ERuneStackType StackType = ERuneStackType::Refresh;

    /**
     * 减层方式（StackType != 禁止 且有持续时间时生效）
     *   全部 — 到期一次性全部移除
     *   逐一 — 到期每次移除一层
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (EditCondition = "UniqueType != ERuneUniqueType::NonUnique && StackType != ERuneStackType::None",
                EditConditionHides))
    ERuneStackReduceType StackReduceType = ERuneStackReduceType::All;

    // ---- 标识 ----

    /** 数值 ID，供策划表引用 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RuneID = 0;

    /**
     * 符文 GP Tag（GE 的资产标签）
     * 用于 GetRuneInfo 按 Tag 查询此 GE，以及 RemoveRune 按 Tag 移除
     *
     * 注意：此 Tag 是 GE 自身的身份标识，与 AddTags Fragment 中的
     * 状态 Tag（授予目标角色的 Tag）完全不同，两者可同时存在
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag RuneTag;

    // ---- 效果（原子功能）----

    /**
     * 效果列表（点击 + 选择类型）：
     *   Add Attribute Modifier   — 属性修改（Attack +20 / AttSpeed ×1.1）
     *   Add Gameplay Tags        — 激活期间向目标授予状态 Tag（如 Status.Poisoned）
     *   Gameplay Cue             — 音效/特效（高级）
     *   Advanced Modifier        — 直接配置 FGameplayModifierInfo（高级）
     *   Execution Calculation    — GAS ExecutionCalculation（高级）
     *
     * GA 的授予/撤销由 FA 层的 GrantGA 节点负责，不在此处配置
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
     * FA 负责：判定时机、授予 GA、触发粒子/音效等
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UFlowAsset> FlowAsset;
};


// ============================================================
//  FRuneInstance — 运行时符文实例
//
//  DA 编辑器视觉结构：
//    Rune Name / Icon / Description  ← 展示信息
//    Shape                           ← 背包格子形状
//    ▼ Rune Config  Type / DurationType / Duration / Period /
//                   UniqueType / MaxStack / StackType / StackReduceType /
//                   RuneID / RuneTag / Effects[]
//    ▼ Flow         FlowAsset
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

    // ---- 背包形状 ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneShape Shape;

    // ---- 核心配置（Type / Duration / Stack / Effects）----

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

    /** 根据 RuneConfig 构建 TransientGE */
    UGameplayEffect* CreateTransientGE(UObject* Outer) const;
};
