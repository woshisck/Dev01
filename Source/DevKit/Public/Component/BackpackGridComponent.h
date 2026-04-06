
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RuneDataAsset.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "BackpackGridComponent.generated.h"

UENUM(BlueprintType)
enum class EHeatTier : uint8
{
    Tier1        = 0,  // 0–99
    Tier2        = 1,  // 100–199
    Tier3        = 2,  // 200–299
    Transcendence = 3, // 300+，激活区同 Tier3
};

// Source/DevKit/Public/Component/BackpackGridComponent.h

USTRUCT(BlueprintType)
struct DEVKIT_API FActivationZoneConfig
{
    GENERATED_BODY()

    // 热度升级阈值（百分比 0~1），升级到 Tier2/Tier3 的门槛
    // 默认：[0.33, 0.66]
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ActivationZone")
    TArray<float> HeatTierThresholds = { 0.33f, 0.66f };

    // 每个热度等级对应的激活区格子（绝对坐标，X=列 Y=行，范围 0~4）
    // 下标 0 = Tier1，下标 1 = Tier2，下标 2 = Tier3
    // 使用 FRuneShape.Cells 复用现有结构（语义上是绝对坐标而非偏移）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ActivationZone")
    TArray<FRuneShape> ZoneShapes;

    // 创建默认矩形配置（1×1 → 2×2 → 4×4）
    static FActivationZoneConfig MakeDefault();
};

USTRUCT(BlueprintType)
struct DEVKIT_API FPlacedRune
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FRuneInstance Rune;
    UPROPERTY(BlueprintReadOnly) FIntPoint Pivot;
    UPROPERTY(BlueprintReadOnly) bool bIsActivated = false;
    // 永久符文：跳过激活区检查，始终保持激活
    UPROPERTY(BlueprintReadOnly) bool bIsPermanent = false;
    // GE 生命周期由 FA 内的 BFNode_ApplyRuneGE 节点管理，BackpackGrid 不持有 handle
};

DECLARE_MULTICAST_DELEGATE(FBGCPhaseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunePlaced, const FRuneInstance&, Rune);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRuneRemoved, FGuid, RuneGuid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatTierChanged, EHeatTier, NewTier);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRuneActivationChanged, FGuid, RuneGuid, bool, bActivated);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class DEVKIT_API UBackpackGridComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBackpackGridComponent();

    UFUNCTION(BlueprintCallable)
    void EditorCenterOnGrid();

    // =========================================================
    // 配置（可在 BP 中设置）
    // =========================================================

    // 网格尺寸（默认5×5）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    int32 GridWidth = 5;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    int32 GridHeight = 5;

    // 激活区配置（默认矩形，可由武器DataAsset覆盖）
    // 通过 SetActivationZoneConfig() 在装备武器时注入
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    FActivationZoneConfig ActivationZoneConfig;

    // =========================================================
    // 委托
    // =========================================================

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnRunePlaced OnRunePlaced;

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnRuneRemoved OnRuneRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnHeatTierChanged OnHeatTierChanged;

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnRuneActivationChanged OnRuneActivationChanged;

    // C++专用委托（非动态，FA 的 BFNode 绑定用）
    FBGCPhaseEvent OnPhaseUpReady;    // 热度达到上限 + LastHit → 满足升阶条件
    FBGCPhaseEvent OnHeatReachedZero; // 热度从 >0 跌落到 0（边沿，Phase>0 时）
    FBGCPhaseEvent OnHeatAboveZero;   // 热度从 0 回升到 >0（边沿）

    // =========================================================
    // 公开接口
    // =========================================================

    // 尝试将符文放置到 Pivot 位置，成功返回 true
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    bool TryPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot);

    // 移除指定 Guid 的符文，成功返回 true
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    bool RemoveRune(FGuid RuneGuid);

    // 将指定符文移动到新位置（内部等价于 Remove + Place）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    bool MoveRune(FGuid RuneGuid, FIntPoint NewPivot);

    // 仅查询是否可放置，不实际放置
    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool CanPlaceRune(const FRuneInstance& Rune, FIntPoint Pivot) const;

    // 锁定/解锁背包（战斗阶段锁定）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SetLocked(bool bLocked);

    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsLocked() const { return bIsLocked; }

    // 由 AttributeSet 的 PostAttributeChange 调用，传入热度绝对值
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void OnHeatValueChanged(float HeatValue);

    // 阶段升级（由 BFNode_IncrementPhase 调用）
    void IncrementPhase();

    // 阶段降级（由 BFNode_DecrementPhase 调用）
    void DecrementPhase();

    // 关卡结束时将热度重置到当前阶段起点（升华归 200，其余取阶段起点）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void ResetHeatToPhaseFloor();

    UFUNCTION(BlueprintPure, Category = "Backpack")
    int32 GetCurrentPhase() const { return CurrentPhase; }

    UFUNCTION(BlueprintPure, Category = "Backpack")
    EHeatTier GetCurrentHeatTier() const { return CurrentTier; }

    // 返回所有已放置符文（UI用）
    UFUNCTION(BlueprintPure, Category = "Backpack")
    const TArray<FPlacedRune>& GetAllPlacedRunes() const { return PlacedRunes; }

    // 返回当前激活区的所有格子坐标（UI高亮用）
    UFUNCTION(BlueprintPure, Category = "Backpack")
    TArray<FIntPoint> GetActivationZoneCells() const;

    // 查询某格是否被占用，返回 PlacedRunes 下标，-1表示空
    UFUNCTION(BlueprintPure, Category = "Backpack")
    int32 GetRuneIndexAtCell(FIntPoint Cell) const;

    // 初始化 ASC 引用（在 PlayerCharacterBase::BeginPlay 中调用）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void InitWithASC(UAbilitySystemComponent* ASC);

    // 装备武器时调用，注入激活区配置（未调用时使用默认矩形配置）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SetActivationZoneConfig(const FActivationZoneConfig& Config);

    // =========================================================
    // 永久符文（生产用，BeginPlay 自动放置）
    // =========================================================

    /** 永久符文列表（如 FA_Rune_HeatPhase），BeginPlay 时自动寻位放置，不受 Debug 控制 */
    UPROPERTY(EditAnywhere, Category = "Backpack|Permanent")
    TArray<TObjectPtr<URuneDataAsset>> PermanentRunes;

    // =========================================================
    // Debug / Test
    // =========================================================

    /** 测试用：拖入 DA_Rune 资产 + 指定 Pivot 位置，BeginPlay 自动放置 */
    UPROPERTY(EditAnywhere, Category = "Backpack|Debug")
    TArray<TObjectPtr<URuneDataAsset>> DebugTestRunes;

    /** 每个测试符文的放置 Pivot（与 DebugTestRunes 一一对应，未填则自动寻位） */
    UPROPERTY(EditAnywhere, Category = "Backpack|Debug")
    TArray<FIntPoint> DebugTestPositions;

    /** 手动触发：放置 DebugTestRunes 到网格（可绑按键调用） */
    UFUNCTION(BlueprintCallable, Category = "Backpack|Debug")
    void DebugPlaceTestRunes();

protected:
    virtual void BeginPlay() override;

private:

    // 占用图：GridWidth×GridHeight 的平铺数组
    // 值 = PlacedRunes 数组的下标，-1 = 空
    TArray<int32> GridOccupancy;

    // 所有已放置符文
    TArray<FPlacedRune> PlacedRunes;

    // 当前热度等级（由 Phase 驱动）
    EHeatTier CurrentTier = EHeatTier::Tier1;

    // 当前阶段（0=Phase1, 1=Phase2, 2=Phase3, 3=升华），最大3
    int32 CurrentPhase = 0;

    // 上次 OnHeatValueChanged 的热度值，用于边沿检测（避免重复广播）
    float PreviousHeatValue = 1.f; // 初始设为 >0，防止游戏开始时误触发

    // 是否锁定（战斗阶段 = true）
    bool bIsLocked = false;

    // ASC 弱引用（避免循环引用）
    TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

    // =========================================================
    // 私有算法
    // =========================================================

    // 计算当前激活区的格子集合
    TSet<FIntPoint> ComputeActivationZone() const;

    // 根据符文形状+Pivot，计算实际占用的所有格子坐标
    TArray<FIntPoint> GetRuneCells(const FRuneInstance& Rune, FIntPoint Pivot) const;

    // 判断已放置符文是否（部分或全部）在激活区内
    bool IsRuneInActivationZone(const FPlacedRune& Placed) const;

    // 激活单个符文（施加 GE）
    void ActivateRune(FPlacedRune& Placed);

    // 取消激活单个符文（移除 GE）
    void DeactivateRune(FPlacedRune& Placed);

    // 重新计算所有符文的激活状态（热度变化或网格变动后调用）
    void RefreshAllActivations();

    // 坐标转平铺索引
    int32 CellToIndex(FIntPoint Cell) const;

    // 坐标合法性检查
    bool IsCellValid(FIntPoint Cell) const;
};