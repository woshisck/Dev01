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
class URuneTooltipWidget;
class UUniformGridPanel;
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

    // 待放置格尺寸（改这里即可，不需动 Designer）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backpack|Pending")
    int32 PendingGridCols = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backpack|Pending")
    int32 PendingGridRows = 4;

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
    // 手柄光标状态（蓝图可读，用于绘制手柄光标高亮）
    // =========================================================

    /** 当前手柄光标所在格子（Col, Row） */
    UPROPERTY(BlueprintReadOnly, Category = "Backpack|Gamepad")
    FIntPoint GamepadCursorCell = FIntPoint(0, 0);

    /** 是否正在持有符文（A 抓取后为 true，A 放置或 B 取消后为 false） */
    UPROPERTY(BlueprintReadOnly, Category = "Backpack|Gamepad")
    bool bGrabbingRune = false;

    /** 抓取操作的源格子（bGrabbingRune=true 时有效） */
    UPROPERTY(BlueprintReadOnly, Category = "Backpack|Gamepad")
    FIntPoint GrabbedFromCell = FIntPoint(-1, -1);

    // =========================================================
    // 符文悬浮 Tooltip（蓝图放置 WBP_RuneTooltip 子控件）
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URuneTooltipWidget> RuneTooltip;

    // =========================================================
    // 手柄抓取浮空图标（Designer 在根 CanvasPanel 里放一个 Image，命名 GrabbedRuneIcon）
    // C++ 在 Tick 里自动更新位置、图标、显隐
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> GrabbedRuneIcon;

    // =========================================================
    // 主格子（Designer：放一个空的 UniformGridPanel，命名 BackpackGrid）
    // 格子数量由 BackpackGridComponent::GridWidth/Height 决定，C++ 动态创建 Button
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UUniformGridPanel> BackpackGrid;

    // =========================================================
    // 左侧待放置符文格
    // Designer：在 LeftPanel 里放一个空的 UniformGridPanel，命名 PendingRuneGrid
    // 格子数量由 PendingGridCols × PendingGridRows 决定，C++ 动态创建 Button
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UUniformGridPanel> PendingRuneGrid;

    // =========================================================
    // 右侧符文信息卡
    // Designer：在 RightPanel 空余处放一个任意面板，命名 RuneInfoCard
    //   └─ 内部放（名称必须完全一致）：
    //        CardIcon    Image     符文图标
    //        CardName    TextBlock 符文名
    //        CardDesc    TextBlock 符文描述
    //        CardUpgrade TextBlock 升级等级（Lv.2 / Lv.3），无升级时隐藏
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> RuneInfoCard;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CardIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardDesc;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CardUpgrade;

    // =========================================================
    // 对外调用：手动刷新左侧待放置槽（切换关卡后外部也可调用）
    // =========================================================
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void RefreshPendingRuneSlots();

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

    /** 打开背包：显示 Widget + 暂停游戏 + 切换 UI 输入模式 */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void OpenBackpack();

    /** 关闭背包：隐藏 Widget + 恢复游戏 + 切换 Game 输入模式 */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void CloseBackpack();

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

    // ── 手柄 / 键盘输入 ─────────────────────────────────────────────────
    virtual void   NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

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

    /** 左侧待放置槽缓存 */
    UPROPERTY()
    TArray<TObjectPtr<UButton>> CachedPendingButtons;

    UPROPERTY()
    TArray<TObjectPtr<UImage>> CachedPendingIcons;

    /** 正在从左侧列表拖拽的 PendingRune 索引（-1 = 无） */
    int32 PendingDragIndex = -1;

    void BindPendingRuneSlots();
    bool GetPendingSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

    /** 拖拽悬浮的目标格子（-1 = 无） */
    int32 HoverCol = -1;
    int32 HoverRow = -1;

    /** 鼠标按下时暂存的格子坐标，供 NativeOnDragDetected 使用 */
    int32 PendingDragCol = -1;
    int32 PendingDragRow = -1;

    /** 屏幕绝对坐标 → BackpackGrid 格子坐标（失败返回 false） */
    bool GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const;

    // ── 手柄辅助 ────────────────────────────────────────────────────────
    void MoveGamepadCursor(int32 DCol, int32 DRow);
    void GamepadConfirm();
    void GamepadCancel();
    void UpdateTooltipForCell(int32 Col, int32 Row, const FVector2D& LocalPos);

    // ── 手柄方向键重复（平滑导航） ────────────────────────────────────
    // 首次按下立即移动；持续按住 DirRepeatInitial 秒后，
    // 每隔 DirRepeatRate 秒自动重复移动一格
    FKey  HeldDirKey;
    float HeldKeyTime    = 0.f;
    int32 LastRepeatCount = 0;
    bool  bDirKeyHeld    = false;

    /** D-Pad 按过后置 true，鼠标移动后置 false，用于区分 A键虚拟点击 vs 真实鼠标点击 */
    bool  bIsGamepadInputMode = false;

    static constexpr float DirRepeatInitial = 0.30f;  // 初始延迟（秒）
    static constexpr float DirRepeatRate    = 0.10f;  // 重复间隔（秒）

    UFUNCTION()
    void HandleRunePlaced(const FRuneInstance& Rune);

    UFUNCTION()
    void HandleRuneRemoved(FGuid RuneGuid);

    UFUNCTION()
    void HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated);
};
