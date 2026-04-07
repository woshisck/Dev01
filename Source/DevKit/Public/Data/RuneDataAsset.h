#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CharacterData.h"
#include "RuneDataAsset.generated.h"

class UFlowAsset;
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
//  FRuneConfig — 符文标识配置（纯元数据）
//
//  GE / GA 逻辑完全由 FA 层节点（BFNode_ApplyEffect / BFNode_GrantGA 等）负责。
//  GE 状态查询通过 BFNode_ApplyEffect 的输出数据引脚获取，无需 Tag 反查。
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneConfig
{
    GENERATED_BODY()

    /** 增益 / 减益 / 无（UI 分类显示用） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneType RuneType = ERuneType::Buff;

    /** 数值 ID，供策划表引用 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RuneID = 0;
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
//                   RuneID
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
};
