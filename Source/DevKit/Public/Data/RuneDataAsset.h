#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "CharacterData.h"
#include "RuneDataAsset.generated.h"

class UFlowAsset;
class URuneDataAsset;
class UGenericRuneEffectDA;


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

/** 符文稀有度（仅影响掉落概率权重，不在 UI 中显示） */
UENUM(BlueprintType)
enum class ERuneRarity : uint8
{
    Common    UMETA(DisplayName = "普通"),
    Rare      UMETA(DisplayName = "稀有"),
    Epic      UMETA(DisplayName = "史诗"),
    Legendary UMETA(DisplayName = "传说"),
};

/**
 * 符文触发时机
 *   Passive        — 进激活区立即生效，持续常驻（默认）
 *   OnAttackHit    — 攻击命中目标时触发一次 FA（每次命中独立实例）
 *   OnDash         — 执行冲刺时触发
 *   OnKill         — 击杀目标时触发
 *   OnCritHit      — 暴击命中时触发
 *   OnDamageReceived — 自身受到伤害时触发
 */
UENUM(BlueprintType)
enum class ERuneTriggerType : uint8
{
    Passive          UMETA(DisplayName = "常驻（进激活区生效）"),
    OnAttackHit      UMETA(DisplayName = "攻击命中时"),
    OnDash           UMETA(DisplayName = "冲刺时"),
    OnKill           UMETA(DisplayName = "击杀时"),
    OnCritHit        UMETA(DisplayName = "暴击时"),
    OnDamageReceived UMETA(DisplayName = "自身受击时"),
};

UENUM(BlueprintType)
enum class ECombatCardType : uint8
{
    Attack    = 0 UMETA(DisplayName = "Attack"),
    Link      = 1 UMETA(DisplayName = "连携卡牌"),
    Finisher  = 2 UMETA(DisplayName = "终结技卡牌"),
    Passive   = 3 UMETA(DisplayName = "Passive"),
    Normal    = 4 UMETA(DisplayName = "普通卡牌"),
};

UENUM(BlueprintType)
enum class ECardRequiredAction : uint8
{
    Light UMETA(DisplayName = "Light"),
    Heavy UMETA(DisplayName = "Heavy"),
    Any   UMETA(DisplayName = "Any"),
};

UENUM(BlueprintType)
enum class ECombatCardTriggerTiming : uint8
{
    OnHit    UMETA(DisplayName = "On Hit"),
    OnCommit UMETA(DisplayName = "On Commit"),
};

UENUM(BlueprintType)
enum class ECardLinkMode : uint8
{
    None         UMETA(DisplayName = "None"),
    ReadPrevious UMETA(DisplayName = "Read Previous"),
    PassToNext   UMETA(DisplayName = "Pass To Next"),
};

UENUM(BlueprintType)
enum class ECombatCardLinkDirection : uint8
{
    None                UMETA(DisplayName = "None"),
    ForwardReadPrevious UMETA(DisplayName = "Forward: Read Previous"),
    BackwardEmpowerNext UMETA(DisplayName = "Backward: Empower Next"),
    Both                UMETA(DisplayName = "Both"),
};

UENUM(BlueprintType)
enum class ECombatCardLinkOrientation : uint8
{
    Forward  UMETA(DisplayName = "正向"),
    Reversed UMETA(DisplayName = "反向"),
};

UENUM(BlueprintType)
enum class ECombatLinkBreakPolicy : uint8
{
    ReleaseBaseFlow  UMETA(DisplayName = "Release Base Flow"),
    ReleaseBreakFlow UMETA(DisplayName = "Release Break Flow"),
    Fizzle           UMETA(DisplayName = "Fizzle"),
};

UENUM(BlueprintType)
enum class EDeckState : uint8
{
    Ready          UMETA(DisplayName = "Ready"),
    EmptyShuffling UMETA(DisplayName = "Empty Shuffling"),
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardLinkCondition
{
    GENERATED_BODY()

    /** Legacy compatibility only. New recipes should use RequiredNeighborIdTags / RequiredNeighborEffectTags. */
    UPROPERTY()
    TArray<ECombatCardType> RequiredNeighborTypes;

    /** Legacy compatibility only. New recipes should use RequiredNeighborIdTags / RequiredNeighborEffectTags. */
    UPROPERTY()
    FGameplayTagContainer RequiredNeighborTags;

    /** Empty = no card id requirement. Any configured tag may identify the neighbor card, such as Card.ID.Moonlight. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Condition")
    FGameplayTagContainer RequiredNeighborIdTags;

    /** Empty = no effect requirement. All configured tags must be present on the neighbor card's CardEffectTags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Condition")
    FGameplayTagContainer RequiredNeighborEffectTags;

    /** Empty = no combo tag requirement. All configured tags must be present in the action context. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Condition", meta = (AdvancedDisplay))
    FGameplayTagContainer RequiredComboTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Condition", meta = (AdvancedDisplay))
    ECardRequiredAction RequiredAction = ECardRequiredAction::Any;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardLinkRecipe
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    ECombatCardLinkOrientation Direction = ECombatCardLinkOrientation::Forward;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    FCombatCardLinkCondition Condition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    TObjectPtr<UFlowAsset> LinkFlow = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    float Multiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    FText ReasonText;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardLinkEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    FCombatCardLinkCondition Condition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    TObjectPtr<UFlowAsset> Flow = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    float Multiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    FText ReasonText;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardLinkConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    ECombatCardLinkDirection Direction = ECombatCardLinkDirection::None;

    /** Current Link card reads the previous resolved card and executes this Flow. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    FCombatCardLinkEffect ForwardEffect;

    /** Current Link card stores this effect and executes it when the next card satisfies the condition. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    FCombatCardLinkEffect BackwardEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    TObjectPtr<UFlowAsset> BreakFlow = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link")
    ECombatLinkBreakPolicy BreakPolicy = ECombatLinkBreakPolicy::ReleaseBaseFlow;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FCombatCardConfig
{
    GENERATED_BODY()

    FCombatCardConfig() = default;

    FCombatCardConfig(ECombatCardType InCardType, ECardRequiredAction InRequiredAction)
        : bIsCombatCard(true)
        , CardType(InCardType)
        , RequiredAction(InRequiredAction)
    {
    }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic")
    bool bIsCombatCard = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic")
    ECombatCardType CardType = ECombatCardType::Normal;

    /** GameplayTag ID for recipe lookup, for example Card.ID.Moonlight. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic")
    FGameplayTag CardIdTag;

    /** Effect tags used by link recipes, for example Card.Effect.Attack or Card.Effect.Moonlight. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic")
    FGameplayTagContainer CardEffectTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic", meta = (AdvancedDisplay))
    ECardRequiredAction RequiredAction = ECardRequiredAction::Any;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic", meta = (AdvancedDisplay))
    ECombatCardTriggerTiming TriggerTiming = ECombatCardTriggerTiming::OnCommit;

    /** Legacy compatibility only. New recipes should use CardIdTag / CardEffectTags. */
    UPROPERTY()
    FGameplayTagContainer CardTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Flow")
    TObjectPtr<UFlowAsset> BaseFlow = nullptr;

    /** Legacy compatibility only. New cards should use BaseFlow or LinkRecipes. */
    UPROPERTY()
    TObjectPtr<UFlowAsset> MatchedFlow = nullptr;

    /** Legacy compatibility only. New link cards should use LinkRecipes. */
    UPROPERTY()
    ECardLinkMode LinkMode = ECardLinkMode::None;

    /** Legacy compatibility only. New link cards should use LinkRecipes. */
    UPROPERTY()
    FCombatCardLinkConfig LinkConfig;

    /** New recipe list. When non-empty, link cards use this instead of the legacy LinkConfig Forward/Backward fields. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    TArray<FCombatCardLinkRecipe> LinkRecipes;

    /** Default runtime orientation for this card instance. Backpack UI can override this per instance later. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Link Recipe")
    ECombatCardLinkOrientation DefaultLinkOrientation = ECombatCardLinkOrientation::Forward;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Basic", meta = (AdvancedDisplay))
    bool bRequiresComboFinisher = false;

    /** Opt-in only. When true, CombatDeck scales this card's effect multiplier from the current combo index. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Combo Scaling")
    bool bUseComboEffectScaling = false;

    /** Additive scalar per combo stack. Combo stacks are max(0, ComboIndex - 1). Example: 0.25 gives Combo2 x1.25. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Combo Scaling", meta = (ClampMin = "0.0", EditCondition = "bUseComboEffectScaling", EditConditionHides))
    float ComboScalarPerIndex = 0.0f;

    /** Maximum additive combo scalar applied by this card. Example: 0.5 caps the combo contribution at +50%. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Combo Scaling", meta = (ClampMin = "0.0", EditCondition = "bUseComboEffectScaling", EditConditionHides))
    float MaxComboScalar = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Display")
    FText DisplayName;

    /** Short player-facing summary for compact HUD surfaces such as the weapon pickup popup. Keep it to 1-2 lines. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Display", meta = (MultiLine = true))
    FText HUDSummaryText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card|Display")
    FText HUDReasonText;
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
    FIntPoint  GetPivotOffset(int32 NumRotations) const;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FRuneTuningScalar
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
    FName Key;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
    float Value = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
    float MinValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
    float MaxValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning", meta = (Categories = "Data"))
    FGameplayTag ValueTag;
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

    /** Short player-facing summary for compact HUD surfaces such as room/portal buff panels. Keep it to 1-2 lines. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (MultiLine = true))
    FText HUDSummaryText;

    // ── 分类元数据 ────────────────────────────────────────────

    /** 增益 / 减益 / 无（UI 分类显示用） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ERuneType RuneType = ERuneType::Buff;

    /**
     * 符文身份 Tag（推荐使用，命名空间 Rune.ID.*，如 Rune.ID.BurningEdge）
     * 替代 int32 RuneID，提供稳定且可读的身份标识。
     * 业务代码请通过 URuneDataAsset::GetRuneIdTag() 读取，避免直接访问字段。
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Rune.ID"))
    FGameplayTag RuneIdTag;

    /** 数值 ID（已弃用：迁移期 fallback，新代码请用 RuneIdTag）*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 RuneID = 0;

    // ── 经济 ─────────────────────────────────────────────────────

    /** 购买价格（金币）。卖出价 = GoldCost / 2，由系统自动计算。0 = 免费 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Economy")
    int32 GoldCost = 0;

    /** 稀有度（影响掉落概率权重；UI 不显示此字段） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
    ERuneRarity Rarity = ERuneRarity::Common;

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

    // ── 触发时机 ──────────────────────────────────────────────────

    /**
     * 符文 FA 的触发时机（BGC 读取此字段决定何时调用 StartBuffFlow）
     * Passive = 进激活区立即生效；其余类型 = 对应事件发生时才启动 FA
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Trigger")
    ERuneTriggerType TriggerType = ERuneTriggerType::Passive;

    // ── 通用效果引用 ─────────────────────────────────────────────

    /** 该符文携带的通用效果引用（如击退/燃烧等），由 RuneInfoCard 右侧子窗解释 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generic Effects")
    TArray<TObjectPtr<UGenericRuneEffectDA>> GenericEffects;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tuning")
    TArray<FRuneTuningScalar> TuningScalars;
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Card")
    FCombatCardConfig CombatCard;

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

    /** 旋转次数（0-3，每次 90° 顺时针）。运行时修改，不从 DA 读取。*/
    UPROPERTY(BlueprintReadWrite)
    int32 Rotation = 0;

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

    // ── 统一访问器（业务代码请使用，禁止直接访问 RuneInfo.RuneConfig.*）──

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    FGameplayTag GetRuneIdTag() const { return RuneInfo.RuneConfig.RuneIdTag; }

    /** 仅迁移期使用，业务代码请改用 GetRuneIdTag() */
    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    int32 GetLegacyRuneID() const { return RuneInfo.RuneConfig.RuneID; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    FName GetRuneName() const { return RuneInfo.RuneConfig.RuneName; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    UTexture2D* GetRuneIcon() const { return RuneInfo.RuneConfig.RuneIcon; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    UTexture2D* GetCardBackground() const { return RuneInfo.RuneConfig.CardBackground; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    FText GetRuneDescription() const { return RuneInfo.RuneConfig.RuneDescription; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    FText GetHUDSummaryText() const { return RuneInfo.RuneConfig.HUDSummaryText; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    ERuneType GetRuneType() const { return RuneInfo.RuneConfig.RuneType; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    int32 GetGoldCost() const { return RuneInfo.RuneConfig.GoldCost; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    int32 GetSellPrice() const { return RuneInfo.RuneConfig.GoldCost / 2; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    ERuneRarity GetRarity() const { return RuneInfo.RuneConfig.Rarity; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    ERuneChainRole GetChainRole() const { return RuneInfo.RuneConfig.ChainRole; }

    /** C++ 引用访问（无封送拷贝；蓝图请用 GetChainDirectionsArray） */
    const TSet<EChainDirection>& GetChainDirections() const { return RuneInfo.RuneConfig.ChainDirections; }

    /** Blueprint 友好版本：返回数组拷贝 */
    UFUNCTION(BlueprintPure, Category = "Rune|Accessor", DisplayName = "Get Chain Directions")
    TArray<EChainDirection> GetChainDirectionsArray() const { return RuneInfo.RuneConfig.ChainDirections.Array(); }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    ERuneTriggerType GetTriggerType() const { return RuneInfo.RuneConfig.TriggerType; }

    /** C++ 引用访问 */
    const TArray<TObjectPtr<UGenericRuneEffectDA>>& GetGenericEffects() const { return RuneInfo.RuneConfig.GenericEffects; }

    /** Blueprint 友好版本：返回裸指针数组拷贝 */
    UFUNCTION(BlueprintPure, Category = "Rune|Accessor", DisplayName = "Get Generic Effects")
    TArray<UGenericRuneEffectDA*> GetGenericEffectsArray() const
    {
        TArray<UGenericRuneEffectDA*> Out;
        Out.Reserve(RuneInfo.RuneConfig.GenericEffects.Num());
        for (const TObjectPtr<UGenericRuneEffectDA>& E : RuneInfo.RuneConfig.GenericEffects)
        {
            Out.Add(E.Get());
        }
        return Out;
    }

    /** C++ 引用访问 */
    const FRuneShape& GetShape() const { return RuneInfo.Shape; }

    /** Blueprint 友好版本：返回结构体拷贝 */
    UFUNCTION(BlueprintPure, Category = "Rune|Accessor", DisplayName = "Get Shape")
    FRuneShape GetShapeCopy() const { return RuneInfo.Shape; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    UFlowAsset* GetFlowAsset() const { return RuneInfo.Flow.FlowAsset; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    const TArray<FRuneTuningScalar>& GetTuningScalars() const { return RuneInfo.RuneConfig.TuningScalars; }

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    bool GetRuneTuningScalar(FName Key, FRuneTuningScalar& OutScalar) const;

    UFUNCTION(BlueprintPure, Category = "Rune|Accessor")
    float GetRuneTuningValue(FName Key, float DefaultValue = 0.f) const;
};
