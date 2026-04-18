
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/RuneDataAsset.h"
#include "AbilitySystemComponent.h"
#include "BackpackGridComponent.generated.h"

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
    UPROPERTY(BlueprintReadOnly) FIntPoint Pivot = FIntPoint::ZeroValue;
    UPROPERTY(BlueprintReadOnly) bool bIsActivated = false;
    // 永久符文：跳过激活区检查，始终保持激活
    UPROPERTY(BlueprintReadOnly) bool bIsPermanent = false;
};

DECLARE_MULTICAST_DELEGATE(FBGCPhaseEvent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRunePlaced, const FRuneInstance&, Rune);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRuneRemoved, FGuid, RuneGuid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnRuneActivationChanged, FGuid, RuneGuid, bool, bActivated);

/** UI 热度条专用：归一化热度（0-1）+ 当前阶段（0-3） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHeatBarUpdate, float, NormalizedHeat, int32, NewPhase);

/** 金币数量变化 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

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

    // 战斗阶段标志：true 时禁止拖动符文（由 GameMode 写入）
    UPROPERTY(BlueprintReadWrite, Category = "Combat")
    bool bIsInCombat = false;

    // =========================================================
    // 委托
    // =========================================================

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnRunePlaced OnRunePlaced;

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnRuneRemoved OnRuneRemoved;

    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnRuneActivationChanged OnRuneActivationChanged;

    /** 热度条 UI 专用：热度值变化或阶段变化时广播 */
    UPROPERTY(BlueprintAssignable, Category = "Backpack|Events")
    FOnHeatBarUpdate OnHeatBarUpdate;

    /** 金币数量变化时广播（UI 监听此委托刷新显示） */
    UPROPERTY(BlueprintAssignable, Category = "Economy|Events")
    FOnGoldChanged OnGoldChanged;

    // C++专用委托（非动态，FA 的 BFNode 绑定用）
    FBGCPhaseEvent OnPhaseUpReady;    // 热度达到上限 + LastHit → 满足升阶条件
    FBGCPhaseEvent OnHeatReachedZero; // 热度从 >0 跌落到 0（边沿，Phase>0 时）
    FBGCPhaseEvent OnHeatAboveZero;   // 热度从 0 回升到 >0（边沿）

    // =========================================================
    // 金币（Economy）
    // =========================================================

    /** 当前持有金币量（SaveGame 持久化） */
    UPROPERTY(SaveGame, BlueprintReadOnly, Category = "Economy")
    int32 Gold = 0;

    /** 增加金币（Amount > 0 时生效），广播 OnGoldChanged */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    void AddGold(int32 Amount);

    /** 扣除金币。金币不足返回 false，不执行扣除 */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool SpendGold(int32 Amount);

    /** 查询金币是否 >= DA.GoldCost（仅判断，不扣除） */
    UFUNCTION(BlueprintPure, Category = "Economy")
    bool CanAffordRune(const URuneDataAsset* DA) const;

    /**
     * 购买符文：验证 Gold >= DA.GoldCost 后扣除金币，返回 true。
     * 调用方负责将符文加入 PendingRunes（PlayerChar->AddRuneToInventory）。
     */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool BuyRune(URuneDataAsset* DA);

    /**
     * 出售已放置的符文：从 PlacedRunes 移除，返还 GoldCost/2 金币。
     * 内部调用 RemoveRune（触发 BuffFlow 停止 + OnRuneRemoved 广播）。
     * 找不到符文返回 false。
     */
    UFUNCTION(BlueprintCallable, Category = "Economy")
    bool SellRune(FGuid RuneGuid);

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

    // 切关后恢复热度阶段（跳过升降阶逻辑，直接写入）
    void RestorePhase(int32 Phase);

    // 返回所有已放置符文（UI用）
    UFUNCTION(BlueprintPure, Category = "Backpack")
    const TArray<FPlacedRune>& GetAllPlacedRunes() const { return PlacedRunes; }

    // 按 RuneName 查找已放置符文，返回指针（nullptr 表示未找到）；升级检查时使用
    FPlacedRune* FindRuneByName(FName RuneName);

    // 返回已达满级（UpgradeLevel == 2）的符文名称列表；供 GenerateLootBatch 过滤奖励池
    UFUNCTION(BlueprintPure, Category = "Backpack")
    TArray<FName> GetMaxLevelRuneNames() const;

    // 符文升级后调用：重启对应符文的 BuffFlow 使新 UpgradeLevel 立即生效，并广播激活变更事件
    void NotifyRuneUpgraded(FGuid RuneGuid);

    // 返回当前激活区的所有格子坐标（UI高亮用）
    UFUNCTION(BlueprintPure, Category = "Backpack")
    TArray<FIntPoint> GetActivationZoneCells() const;

    // 返回指定热度阶段的激活区格子（Phase 越界时返回空数组）
    UFUNCTION(BlueprintPure, Category = "Backpack|ActivationZone")
    TArray<FIntPoint> GetActivationZoneCellsForPhase(int32 Phase) const;

    // 查询某格是否被占用，返回 PlacedRunes 下标，-1表示空
    UFUNCTION(BlueprintPure, Category = "Backpack")
    int32 GetRuneIndexAtCell(FIntPoint Cell) const;

    // 初始化 ASC 引用（在 PlayerCharacterBase::BeginPlay 中调用）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void InitWithASC(UAbilitySystemComponent* ASC);

    // 装备武器时调用，注入激活区配置（未调用时使用默认矩形配置）
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SetActivationZoneConfig(const FActivationZoneConfig& Config);

    // 装备武器时统一设置背包尺寸 + 激活区配置
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void ApplyBackpackConfig(int32 InGridWidth, int32 InGridHeight, const FActivationZoneConfig& InConfig);

    // =========================================================
    // 永久符文（生产用，BeginPlay 自动放置）
    // =========================================================

    /** 永久符文列表（如 FA_Rune_HeatPhase），BeginPlay 时自动寻位放置，不受 Debug 控制 */
    UPROPERTY(EditAnywhere, Category = "Backpack|Permanent")
    TArray<TObjectPtr<URuneDataAsset>> PermanentRunes;

    // =========================================================
    // 隐藏被动符文（不占格子、不显示在背包 UI、玩家无法操控）
    // =========================================================

    /**
     * 隐藏被动符文列表
     * BeginPlay 时直接启动 BuffFlow，不放入格子，背包 UI 中完全不可见
     * 用于：热度机制、全局 Buff、系统效果等玩家不应干预的后台逻辑
     */
    UPROPERTY(EditAnywhere, Category = "Backpack|Passive",
              meta = (DisplayName = "隐藏被动符文（不进背包格）"))
    TArray<TObjectPtr<URuneDataAsset>> HiddenPassiveRunes;

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
    // 懒初始化：确保 GridOccupancy 已分配（BeginPlay 前调用 TryPlaceRune 时触发）
    void EnsureGridInitialized();

    // 启动隐藏被动符文的 BuffFlow（不放入格子）
    void ActivateHiddenPassiveRunes();

    // 占用图：GridWidth×GridHeight 的平铺数组
    // 值 = PlacedRunes 数组的下标，-1 = 空
    TArray<int32> GridOccupancy;

    // 所有已放置符文
    TArray<FPlacedRune> PlacedRunes;

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

    // 向 UI 广播当前归一化热度 + 阶段（在 Heat/Phase 变化后调用）
    void BroadcastHeatUI(float KnownHeatValue);
};