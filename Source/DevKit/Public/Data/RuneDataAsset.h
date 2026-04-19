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

/** 链路角色：决定符文在链路系统中的身份 */
UENUM(BlueprintType)
enum class ERuneChainRole : uint8
{
    None     UMETA(DisplayName = "无"),
    Producer UMETA(DisplayName = "链路传出（需在激活区内才可传导）"),
    Consumer UMETA(DisplayName = "外圈限定（不能放入任何激活区格子）"),
};

/** 链路传导方向（8方向，Producer 符文按勾选方向传导激活） */
UENUM(BlueprintType)
enum class EChainDirection : uint8
{
    N  UMETA(DisplayName = "上"),
    S  UMETA(DisplayName = "下"),
    E  UMETA(DisplayName = "右"),
    W  UMETA(DisplayName = "左"),
    NE UMETA(DisplayName = "右上"),
    NW UMETA(DisplayName = "左上"),
    SE UMETA(DisplayName = "右下"),
    SW UMETA(DisplayName = "左下"),
};

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
//  FRuneConfig — 符文完整配置（展示信息 + 分类元数据）
//
//  GE / GA 逻辑完全由 FA 层节点（BFNode_ApplyEffect / BFNode_GrantGA 等）负责。
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneConfig
{
    GENERATED_BODY()

    // ── 展示信息 ──────────────────────────────────────────────

    /** 符文名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RuneName;

    /** 符文图标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> RuneIcon;

    /** 信息卡背景图（RuneInfoCard 的 CardBG 使用此贴图；留空则隐藏） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UTexture2D> CardBackground;

    /** 符文描述文本 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText RuneDescription;

    // ── 分类元数据 ────────────────────────────────────────────

    /** 增益 / 减益 / 无（UI 分类显示用） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneType RuneType = ERuneType::Buff;

    /** 数值 ID，供策划表引用 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RuneID = 0;

    // ── 经济 ─────────────────────────────────────────────────────

    /** 购买价格（金币）。卖出价 = GoldCost / 2，由系统自动计算。0 = 免费 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 GoldCost = 0;

    // ── 链路系统 ──────────────────────────────────────────────────

    /** 链路角色：None=普通符文，Producer=可传导激活，Consumer=只能放外圈 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain")
    ERuneChainRole ChainRole = ERuneChainRole::None;

    /**
     * Producer 传导方向（ChainRole=Producer 时生效）
     * 勾选的方向：此符文在激活区内时，向相邻格传导激活
     * 空 = 不传导任何方向
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chain",
              meta = (EditCondition = "ChainRole == ERuneChainRole::Producer"))
    TSet<EChainDirection> ChainDirections;
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
//    ▼ Rune Config   Name / Icon / Description / Type / RuneID
//    Shape           ← 背包格子形状
//    ▼ Flow          FlowAsset
// ============================================================

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneInstance
{
    GENERATED_BODY()

    // ---- 展示 + 分类配置（合并到 RuneConfig）----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneConfig RuneConfig;

    // ---- 背包形状 ----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneShape Shape;

    // ---- 逻辑层（FA）----

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FRuneFlowConfig Flow;

    // ---- 运行时数据（不在 DA 里填写）----

    UPROPERTY(BlueprintReadWrite)
    int32 Level = 1;

    /** 升级等级：0=Lv.I（基础）, 1=Lv.II（×1.5）, 2=Lv.III（×2.0，满级）*/
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    int32 UpgradeLevel = 0;

    /** 根据 UpgradeLevel 返回效果倍率：1.0 / 1.5 / 2.0（C++ 内部使用，FlowAsset 直接读 UpgradeLevel）*/
    float GetUpgradeMultiplier() const { return 1.0f + (UpgradeLevel * 0.5f); }

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
    FRuneInstance RuneInfo;

    UFUNCTION(BlueprintCallable, Category = "Rune")
    FRuneInstance CreateInstance() const;
};
