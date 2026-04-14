#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "BackpackScreenWidget.generated.h"

class UBackpackGridComponent;
class UBackpackScreenWidget;
class APlayerCharacterBase;
class UButton;
class UImage;
class UTextBlock;
class URichTextBlock;
struct FPlacedRune;

// ============================================================
//  格子点击中转（每格一个，存储 Col/Row 供 dynamic delegate 调用）
// ============================================================
UCLASS()
class UGridCellClickHandler : public UObject
{
    GENERATED_BODY()
public:
    TWeakObjectPtr<UBackpackScreenWidget> Owner;
    int32 Col = 0;
    int32 Row = 0;

    UFUNCTION()
    void HandleClick();
};

// ============================================================
//  格子视觉状态枚举
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
//  蓝图里只需要：
//  1. 在 Details 面板填写 AvailableRunes
//  2. 在 Designer 里放以下控件（名称必须完全一致）：
//     - BackpackGrid        ← 格子面板（UniformGridPanel / HorizontalBox 等）
//     - DetailPanel         ← 详情区容器（任意面板）
//     - DetailIcon          ← Image，显示符文图标
//     - DetailName          ← TextBlock，显示符文名称
//     - DetailDesc          ← TextBlock，显示符文描述
//  3. 格子内每个 Button 不需要手动绑定，C++ 自动完成
//  4. OnGridNeedsRefresh / OnSelectionChanged 已有 C++ 默认实现，
//     蓝图可覆盖，也可以直接删掉蓝图里的实现节点
// ============================================================

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBackpackScreenWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // 配置
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backpack")
    TArray<TObjectPtr<URuneDataAsset>> AvailableRunes;

    // =========================================================
    // 自动绑定的详情面板控件
    // 在 Designer 里添加对应名称的控件即可，无需蓝图节点
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> DetailPanel;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> DetailIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DetailName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DetailDesc;

    /**
     * 操作提示文本（可选）
     * 在 Designer 里加一个 TextBlock 命名 HintText，C++ 自动写入当前操作提示
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URichTextBlock> HintText;

    // =========================================================
    // 运行时状态（蓝图可读）
    // =========================================================

    UPROPERTY(BlueprintReadOnly, Category = "Backpack")
    int32 SelectedRuneIndex = -1;

    UPROPERTY(BlueprintReadOnly, Category = "Backpack")
    FIntPoint SelectedCell = FIntPoint(-1, -1);

    // =========================================================
    // 状态查询
    // =========================================================

    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsCellInActivationZone(int32 Col, int32 Row) const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsCellOccupied(int32 Col, int32 Row) const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    FPlacedRune GetRuneAtCell(int32 Col, int32 Row) const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    EBackpackCellState GetCellVisualState(int32 Col, int32 Row) const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool HasSelectedRune() const { return SelectedRuneIndex >= 0; }

    UFUNCTION(BlueprintPure, Category = "Backpack")
    TArray<FRuneInstance> GetRuneList() const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    int32 GetPendingRuneCount() const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    FRuneInstance GetSelectedRuneInfo() const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    const TArray<FPlacedRune>& GetAllPlacedRunes() const;

    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsCellSelected(int32 Col, int32 Row) const { return SelectedCell == FIntPoint(Col, Row); }

    UFUNCTION(BlueprintPure, Category = "Backpack")
    UTexture2D* GetRuneIconAtCell(int32 Col, int32 Row) const;

    /** 获取当前焦点符文（格子选中优先，列表选中其次） */
    UFUNCTION(BlueprintPure, Category = "Backpack")
    FRuneInstance GetFocusedRuneInfo() const;

    // =========================================================
    // 操作
    // =========================================================

    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SelectRuneFromList(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void ClickCell(int32 Col, int32 Row);

    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void RemoveRuneAtSelectedCell();

    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void ClearSelection();

    // =========================================================
    // 刷新事件（BlueprintNativeEvent：C++ 提供默认实现）
    // 蓝图里如果有旧的实现节点，请删除，让 C++ 接管
    // =========================================================

    /** 网格颜色 + 图标刷新（C++ 自动处理，蓝图无需实现） */
    UFUNCTION(BlueprintNativeEvent, Category = "Backpack")
    void OnGridNeedsRefresh();
    virtual void OnGridNeedsRefresh_Implementation();

    /** 选中状态变化：详情面板刷新 + 格子高亮（C++ 自动处理） */
    UFUNCTION(BlueprintNativeEvent, Category = "Backpack")
    void OnSelectionChanged();
    virtual void OnSelectionChanged_Implementation();

    /** 操作结果提示（蓝图可实现 Toast/弹窗，C++ 无默认实现） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnStatusMessage(const FText& Message);

    /** PendingRunes 变化（蓝图刷新左侧符文列表） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnRuneListChanged();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;
    UBackpackGridComponent* GetBackpack() const;

    /** 遍历 BackpackGrid，为每个格子绑定点击并创建 Icon Image */
    void BindGridCellClicks();

    /** 按格子索引缓存 Button 和 Icon Image，供刷新时快速访问 */
    UPROPERTY()
    TArray<TObjectPtr<UButton>> CachedCellButtons;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> CachedCellIcons;

    /** 防止 GC 回收格子点击 handler */
    UPROPERTY()
    TArray<TObjectPtr<UGridCellClickHandler>> CellClickHandlers;

    UFUNCTION()
    void HandleRunePlaced(const FRuneInstance& Rune);

    UFUNCTION()
    void HandleRuneRemoved(FGuid RuneGuid);

    UFUNCTION()
    void HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated);
};
