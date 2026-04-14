#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "BackpackScreenWidget.generated.h"

class UBackpackGridComponent;
class APlayerCharacterBase;
struct FPlacedRune;

// ============================================================
//  格子视觉状态枚举（蓝图用颜色区分）
// ============================================================

UENUM(BlueprintType)
enum class EBackpackCellState : uint8
{
    Empty            UMETA(DisplayName = "空格(灰)"),
    EmptyActive      UMETA(DisplayName = "激活区空格(深蓝)"),
    OccupiedActive   UMETA(DisplayName = "激活中符文(亮蓝)"),
    OccupiedInactive UMETA(DisplayName = "未激活符文(橙)"),
};

// ============================================================
//  UBackpackScreenWidget
//
//  使用方法：
//  1. 新建 Widget Blueprint，父类选 BackpackScreenWidget
//  2. 在 Details 面板填写 AvailableRunes（展示用符文列表）
//  3. 设计 5x5 格子：每格一个 Button，OnClicked 调用 ClickCell(Col, Row)
//  4. 实现 OnGridNeedsRefresh：遍历格子，调用 GetCellVisualState 设置颜色
//  5. 左侧列表每项调用 SelectRuneFromList(Index)
//  6. 实现 OnSelectionChanged：根据 SelectedRuneIndex/SelectedCell 高亮
// ============================================================

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBackpackScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // 配置（在蓝图 Details 面板填写）
    // =========================================================

    /**
     * 展示/调试用固定符文库（在 Details 面板填写 DA_Rune_* 资产）
     * 放置后不消耗，可反复使用（供展示和调试）
     * 游戏中实际符文来自 Player.PendingRunes（三选一获得）
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backpack")
    TArray<TObjectPtr<URuneDataAsset>> AvailableRunes;

    // =========================================================
    // 运行时状态（蓝图可读）
    // =========================================================

    /** 当前在左侧列表中选中的符文索引，-1 = 未选中 */
    UPROPERTY(BlueprintReadOnly, Category = "Backpack")
    int32 SelectedRuneIndex = -1;

    /** 当前在网格中选中的格子，(-1,-1) = 未选中 */
    UPROPERTY(BlueprintReadOnly, Category = "Backpack")
    FIntPoint SelectedCell = FIntPoint(-1, -1);

    // =========================================================
    // 状态查询（BlueprintPure，蓝图直接调用）
    // =========================================================

    /** 格子是否在激活区 */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsCellInActivationZone(int32 Col, int32 Row) const;

    /** 格子是否有符文 */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsCellOccupied(int32 Col, int32 Row) const;

    /** 获取格子上的符文（需先判断 IsCellOccupied） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    FPlacedRune GetRuneAtCell(int32 Col, int32 Row) const;

    /** 获取格子的综合视觉状态（蓝图用于设置颜色） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    EBackpackCellState GetCellVisualState(int32 Col, int32 Row) const;

    /** 是否已选中待放置符文 */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool HasSelectedRune() const { return SelectedRuneIndex >= 0; }

    /**
     * 获取所有可放置的符文列表（PendingRunes 在前，AvailableRunes 在后）
     * 蓝图用此函数填充左侧符文列表，不要缓存返回值（每次调用都是最新数据）
     */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    TArray<FRuneInstance> GetRuneList() const;

    /** PendingRunes 的数量（蓝图可用此值区分列表中哪些来自三选一） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    int32 GetPendingRuneCount() const;

    /** 获取当前选中符文的信息（用于显示描述面板） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    FRuneInstance GetSelectedRuneInfo() const;

    /** 获取所有已放置符文（调试或统计用） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    const TArray<FPlacedRune>& GetAllPlacedRunes() const;

    /** 格子是否是当前选中格（需要绘制高亮边框） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsCellSelected(int32 Col, int32 Row) const
    {
        return SelectedCell == FIntPoint(Col, Row);
    }

    // =========================================================
    // 操作（Button OnClicked 绑定）
    // =========================================================

    /** 从左侧列表选中符文；再次点击同一个则取消选中 */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SelectRuneFromList(int32 Index);

    /**
     * 点击网格格子：
     *  - 格子有符文 → 选中这个格子（可后续移除）
     *  - 格子空 + 有选中符文 → 尝试放置
     *  - 格子空 + 无选中符文 → 无操作
     */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void ClickCell(int32 Col, int32 Row);

    /** 移除当前选中格子上的符文 */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void RemoveRuneAtSelectedCell();

    /** 清除所有选中状态 */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void ClearSelection();

    // =========================================================
    // Blueprint 实现的刷新事件
    // =========================================================

    /** 网格数据有变化，蓝图应遍历 5×5 格子刷新颜色和图标 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnGridNeedsRefresh();

    /** 选中状态变化（符文列表高亮、格子高亮、描述面板刷新） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnSelectionChanged();

    /** 操作结果提示（放置成功/失败/移除等） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnStatusMessage(const FText& Message);

    /**
     * PendingRunes 发生变化（三选一加入或放置消耗后）
     * 蓝图应重新调用 GetRuneList() 刷新左侧符文列表
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnRuneListChanged();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;

    UBackpackGridComponent* GetBackpack() const;

    UFUNCTION()
    void HandleRunePlaced(const FRuneInstance& Rune);

    UFUNCTION()
    void HandleRuneRemoved(FGuid RuneGuid);

    UFUNCTION()
    void HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated);
};
