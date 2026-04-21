#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
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
class URuneInfoCardWidget;
class UBackpackGridWidget;
class UBackpackStyleDataAsset;
class UPendingGridWidget;
struct FPlacedRune;

// ============================================================
//  格子视觉状态枚举（蓝图可用，供自定义 BP 逻辑查询）
// ============================================================
UENUM(BlueprintType)
enum class EBackpackCellState : uint8
{
    Empty            UMETA(DisplayName = "空格(灰)"),
    EmptyActive      UMETA(DisplayName = "热度1区空格"),
    EmptyZone1       UMETA(DisplayName = "热度2区空格"),
    EmptyZone2       UMETA(DisplayName = "热度3区空格"),
    OccupiedActive   UMETA(DisplayName = "激活中符文(亮蓝)"),
    OccupiedInactive UMETA(DisplayName = "未激活符文(橙)"),
};

// ============================================================
//  UBackpackScreenWidget（协调层）
//
//  职责：CommonUI 生命周期 / 输入 / 拖拽 / 状态管理。
//  渲染工作委托给子 Widget：
//    - BackpackGridWidget  ← 主格子（创建+刷新）
//    - PendingGridWidget   ← 待放置区（创建+刷新）
//    - RuneInfoCardWidget  ← 右侧信息卡（显隐）
//
//  蓝图 Designer 中只需放置对应名称的子 Widget 实例，无需额外节点。
// ============================================================

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBackpackScreenWidget : public UCommonActivatableWidget
{
    GENERATED_BODY()

public:
    UBackpackScreenWidget(const FObjectInitializer& ObjectInitializer);

    // =========================================================
    // 配置（蓝图 Details 填写）
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backpack")
    TArray<TObjectPtr<URuneDataAsset>> AvailableRunes;

    // =========================================================
    // 子 Widget 绑定
    // Designer 中放对应名称的 WBP 实例即可，C++ 自动绑定
    // =========================================================

    /** 主格子子 Widget（WBP_BackpackGrid，命名 "BackpackGridWidget"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UBackpackGridWidget> BackpackGridWidget;

    /** 待放置区子 Widget（WBP_PendingGrid，命名 "PendingGridWidget"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UPendingGridWidget> PendingGridWidget;

    /** 右侧符文信息卡（WBP_RuneInfoCard，命名 "RuneInfoCard"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URuneInfoCardWidget> RuneInfoCard;

    /** 符文 Tooltip（WBP_RuneTooltip，命名 "RuneTooltip"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<URuneTooltipWidget> RuneTooltip;

    /** 手柄/鼠标拖拽浮空图标（根 CanvasPanel 里放一个 Image，命名 "GrabbedRuneIcon"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> GrabbedRuneIcon;

    /** 出售按钮（命名 "SellButton"，C++ 自动绑定点击事件） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> SellButton;

    /** 右上角关闭按钮（命名 "CloseButton"，点击关闭背包） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> CloseButton;

    // ── 旧版详情面板（可选保留，新版统一用 RuneInfoCard） ──────────────
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
    // 手柄光标状态（蓝图可读）
    // =========================================================

    UPROPERTY(BlueprintReadOnly, Category = "Backpack|Gamepad")
    FIntPoint GamepadCursorCell = FIntPoint(0, 0);

    UPROPERTY(BlueprintReadOnly, Category = "Backpack|Gamepad")
    bool bGrabbingRune = false;

    UPROPERTY(BlueprintReadOnly, Category = "Backpack|Gamepad")
    FIntPoint GrabbedFromCell = FIntPoint(-1, -1);

    // =========================================================
    // 对外调用
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
    // 刷新事件（BlueprintNativeEvent）
    // =========================================================

    UFUNCTION(BlueprintNativeEvent, Category = "Backpack")
    void OnGridNeedsRefresh();
    virtual void OnGridNeedsRefresh_Implementation();

    UFUNCTION(BlueprintNativeEvent, Category = "Backpack")
    void OnSelectionChanged();
    virtual void OnSelectionChanged_Implementation();

    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnStatusMessage(const FText& Message);

    UFUNCTION(BlueprintImplementableEvent, Category = "Backpack")
    void OnRuneListChanged();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    // ── CommonUI ──────────────────────────────────────────────────────────
    virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
    virtual void NativeOnActivated() override;
    virtual void NativeOnDeactivated() override;

    // ── 手柄 / 键盘输入 ─────────────────────────────────────────────────
    virtual void   NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // ── 拖拽输入 ────────────────────────────────────────────────────────
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void   NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool   NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool   NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void   NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;
    UBackpackGridComponent* GetBackpack() const;

    // ── 待放置区稀疏格子（本地，开关背包时与 Player->PendingRunes 同步） ──────
    TArray<FRuneInstance> PendingGrid;  // 平铺数组 PendingCols×PendingRows，无效 GUID = 空
    int32 PendingCols = 2;
    int32 PendingRows = 4;
    int32 PendingSelectedIdx = -1;      // 鼠标点选的待放置格（-1=无）

    // ── 待放置区手柄光标 ─────────────────────────────────────────────────
    bool  bCursorInPendingArea = false;
    int32 PendingCursorIdx     = 0;
    bool  bGrabbingFromPending = false; // 手柄从待放置区抓起符文
    int32 PendingGrabbedIdx    = -1;

    // ── 拖拽状态 ─────────────────────────────────────────────────────────
    int32 PendingDragIndex = -1; // 左侧待放置槽拖拽索引（-1 = 无）
    int32 HoverCol = -1;         // 拖拽悬浮目标格
    int32 HoverRow = -1;
    int32 PendingDragCol = -1;   // 鼠标按下时暂存的格子坐标
    int32 PendingDragRow = -1;

    // ── 鼠标拖拽浮空图标 ──────────────────────────────────────────────────
    bool        bMouseDragging  = false;
    UTexture2D* MouseDragTex    = nullptr;
    FVector2D   LastMouseAbsPos = FVector2D::ZeroVector;

    // ── 热度阶段预览 ──────────────────────────────────────────────────────
    int32 PreviewPhase = -1; // -1 = 无预览；0/1/2 = 预览对应热度阶段激活区

    // ── 手柄方向键重复 ────────────────────────────────────────────────────
    FKey  HeldDirKey;
    float HeldKeyTime    = 0.f;
    int32 LastRepeatCount = 0;
    bool  bDirKeyHeld    = false;

    /** D-Pad 按过后置 true，鼠标移动后置 false */
    bool bIsGamepadInputMode = false;

    static constexpr float DirRepeatInitial = 0.30f;
    static constexpr float DirRepeatRate    = 0.10f;

    // ── 坐标辅助（委托给子 Widget） ───────────────────────────────────────
    bool GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const;
    bool GetPendingSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

    bool IsInCombatPhase() const;

    // ── 待放置区辅助 ──────────────────────────────────────────────────────
    void SyncPendingFromPlayer();        // 打开时：Player->PendingRunes → PendingGrid
    void SyncPendingToPlayer();          // 关闭/修改后：PendingGrid → Player->PendingRunes
    void RefreshPendingGrid();           // 驱动 PendingGridWidget->RefreshSlots
    void MovePendingCursor(int32 DCol, int32 DRow);
    void PendingGamepadConfirm();
    void PendingGamepadCancel();

    // ── 手柄辅助 ──────────────────────────────────────────────────────────
    void MoveGamepadCursor(int32 DCol, int32 DRow);
    void GamepadConfirm();
    void GamepadCancel();
    void UpdateTooltipForCell(int32 Col, int32 Row, const FVector2D& LocalPos);

    // ── 事件处理 ──────────────────────────────────────────────────────────
    UFUNCTION()
    void HandleRunePlaced(const FRuneInstance& Rune);

    UFUNCTION()
    void HandleRuneRemoved(FGuid RuneGuid);

    UFUNCTION()
    void HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated);

    UFUNCTION()
    void OnSellButtonClicked();

    UFUNCTION()
    void OnCloseButtonClicked();

    UFUNCTION()
    void HandleHeatPhaseButtonClicked(int32 Phase);
};
