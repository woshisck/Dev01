#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "BackpackScreenWidget.generated.h"

class UBackpackGridComponent;
class APlayerCharacterBase;
class UButton;
class UImage;
class UTextBlock;
class URichTextBlock;
class UDragDropOperation;
struct FPlacedRune;

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
//  4. 符文移动通过拖拽实现，蓝图无需节点
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

    // ── 拖拽输入重写 ────────────────────────────────────────────────────
    // BackpackGrid 和格子 Button 均设为 HitTestInvisible，
    // 所有鼠标/拖拽事件由 BackpackScreenWidget 自身接管

    /** 鼠标按下预览（隧道阶段）：检测是否在格子上，启动拖拽或执行点击 */
    virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    /** 拖拽检测触发：构建 URuneDragDropOperation 并设置拖拽视觉 */
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

    /** 拖拽经过：更新悬浮格子高亮 */
    virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    /** 松手放置：执行 MoveRune */
    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    /** 拖拽取消（Esc 或松手到无效区域）：清理悬浮状态 */
    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;
    UBackpackGridComponent* GetBackpack() const;

    /** 遍历 BackpackGrid，设置按钮 HitTestInvisible 并创建 Icon Image */
    void BindGridCellClicks();

    /** 按格子索引缓存 Button 和 Icon Image，供刷新时快速访问 */
    UPROPERTY()
    TArray<TObjectPtr<UButton>> CachedCellButtons;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> CachedCellIcons;

    /** 拖拽悬浮的目标格子（-1 = 无） */
    int32 HoverCol = -1;
    int32 HoverRow = -1;

    /** 鼠标按下时暂存的格子坐标，供 NativeOnDragDetected 使用 */
    int32 PendingDragCol = -1;
    int32 PendingDragRow = -1;

    /** 屏幕绝对坐标 → BackpackGrid 格子坐标（失败返回 false） */
    bool GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const;

    UFUNCTION()
    void HandleRunePlaced(const FRuneInstance& Rune);

    UFUNCTION()
    void HandleRuneRemoved(FGuid RuneGuid);

    UFUNCTION()
    void HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated);
};
