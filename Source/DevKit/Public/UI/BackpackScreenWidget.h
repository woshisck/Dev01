#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Data/RuneDataAsset.h"
#include "CommonInputTypeEnum.h"
#include "BackpackScreenWidget.generated.h"

class UBackpackGridComponent;
class APlayerCharacterBase;
class UButton;
class UImage;
class UCanvasPanel;
class UTextBlock;
class URichTextBlock;
class UDragDropOperation;
class URuneTooltipWidget;
class URuneInfoCardWidget;
class UBackpackGridWidget;
class UBackpackStyleDataAsset;
class UPendingGridWidget;
class UCombatDeckEditWidget;
class UTexture2D;
class UWeaponDefinition;
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

    /**
     * 切换只读预览模式：true = 禁止所有拖拽/旋转/抓取（用于 LootSelection 预览背包），
     * false = 正常整理模式。
     * NativeOnDeactivated 会自动复位为 false。
     */
    UFUNCTION(BlueprintCallable, Category = "Backpack")
    void SetPreviewMode(bool bReadOnly);

    UFUNCTION(BlueprintPure, Category = "Backpack")
    bool IsPreviewMode() const { return bIsPreviewMode; }

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

    /** 战斗卡组编辑列表（可选，命名 "CombatDeckEditWidget"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCombatDeckEditWidget> CombatDeckEditWidget;

    /** 手柄/鼠标拖拽浮空图标（根 CanvasPanel 里放一个 Image，命名 "GrabbedRuneIcon"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> GrabbedRuneIcon;

    /**
     * 拖拽时显示完整 Shape 的画布（命名 "ShapePreviewCanvas"，UCanvasPanel 类型）。
     * 若绑定，鼠标拖拽时会动态填充 N 个 cell UImage 显示完整 Shape，Pivot 跟随鼠标。
     * 若未绑定，回退到 GrabbedRuneIcon 单图标显示（向后兼容）。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UCanvasPanel> ShapePreviewCanvas;

    /**
     * 操作提示浮窗（命名 "OperationHintWidget"，UWidget/Border/Canvas 任意类型）。
     * 玩家抓起符文（bGrabbingRune=true）时显示；放下/取消/进入预览/战斗时隐藏。
     * 内放 CommonRichTextBlock 提示按键，例：「<input action="..."/>移动 <input action="..."/>旋转」。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> OperationHintWidget;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> OperationHintText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> WeaponIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> WeaponNameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> WeaponDescText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UWidget> ComboHintText;

    /** 出售按钮（命名 "SellButton"，C++ 自动绑定点击事件） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> SellButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> ConfirmButton;

    /** 右上角关闭按钮（命名 "CloseButton"，点击关闭背包） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> CloseButton;

    /**
     * 结束预览按钮（命名 "EndPreviewButton"，UButton 类型）
     * 仅在 bIsPreviewMode=true 时显示，与 CloseButton 互斥（同位置）。
     * 点击 → DeactivateWidget → HUD 监听器回调 → LootSelection 恢复
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> EndPreviewButton;


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
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // ── 拖拽输入 ────────────────────────────────────────────────────────
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void   NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
    virtual bool   NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual bool   NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
    virtual void   NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;
    UBackpackGridComponent* GetBackpack() const;
    void HandleCommonInputMethodChanged(ECommonInputType NewInputType);

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
    bool  bDeckSelectButtonWasDown = false;
    bool  bDeckSelectFromVirtualMouse = false;

    /** D-Pad 按过后置 true，鼠标移动后置 false */
    bool bIsGamepadInputMode = false;

    static constexpr float DirRepeatInitial = 0.30f;
    static constexpr float DirRepeatRate    = 0.10f;

    // ── 坐标辅助（委托给子 Widget） ───────────────────────────────────────
    bool GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const;
    bool GetPendingSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

    bool IsInCombatPhase() const;
    void RefreshWeaponAndComboInfo();
    void SetTextIfSupported(UWidget* Widget, const FText& Text) const;
    FText BuildOperationHintText() const;
    FText BuildConfirmButtonText() const;
    FText BuildCancelButtonText() const;
    FText BuildEndPreviewButtonText() const;
    void SetButtonContentText(UButton* Button, const FText& Text) const;
    void RefreshActionButtonHints();
    FText BuildComboHintText(const UWeaponDefinition* WeaponDefinition) const;

    // ── 待放置区辅助 ──────────────────────────────────────────────────────
    void SyncPendingFromPlayer();        // 打开时：Player->PendingRunes → PendingGrid
    void SyncPendingToPlayer();          // 关闭/修改后：PendingGrid → Player->PendingRunes
    void RefreshPendingGrid();           // 驱动 PendingGridWidget->RefreshSlots

    /**
     * 焦点切到主背包时清掉 pending 高亮 + 选择状态。
     * @param bClearSelection true=同时清 PendingSelectedIdx（彻底转让焦点）。
     *                        false=仅清光标视觉，保留待放置选择（用于 ClickCell 放置流程内部）。
     */
    void ClearPendingFocus(bool bClearSelection);

    /** 根据当前抓取/选中状态显隐 OperationHintWidget */
    void UpdateOperationHintVisibility();

    /** Logs packaged-build layout sizes after the widget has opened. */
    void LogLayoutDiagnostics();

    /** UpdateOperationHintVisibility 缓存：避免每帧重复 SetVisibility */
    bool bOperationHintVisible = false;
    bool bActionButtonHintsInitialized = false;
    bool bLastActionButtonHintsGamepad = false;
    bool bLastActionButtonHintsPreviewMode = false;

    void MovePendingCursor(int32 DCol, int32 DRow);
    void PendingGamepadConfirm();
    void PendingGamepadCancel();

    // ── 手柄辅助 ──────────────────────────────────────────────────────────
    void MoveGamepadCursor(int32 DCol, int32 DRow);
    void GamepadConfirm();
    void GamepadCancel();
    FReply HandleCombatDeckSelectButtonState(bool bPressed, const TCHAR* Source);
    void PollCombatDeckSelectButtonState();
    void UpdateTooltipForCell(int32 Col, int32 Row, const FVector2D& LocalPos);

    // ── 旋转 ──────────────────────────────────────────────────────────────
    void RotateSelectedRune();

    // ── 只读预览模式（LootSelection 期间预览背包用） ─────────────────────
    bool bIsPreviewMode = false;
    void RotatePendingRune();

    UFUNCTION()
    void OnEndPreviewClicked();

    // ── Shape 拖拽预览（Phase 2） ──────────────────────────────────────────
    /** 拖拽开始：根据 Rune 构造 N 个 cell UImage 填充 ShapePreviewCanvas，AnchorCell 在旋转后 Shape 本地坐标 */
    void ShowShapePreview(const FRuneInstance& Rune, FIntPoint AnchorCellInRotatedLocal,
                          UTexture2D* IconTex, float CellPx);
    /** 每帧调用：让 AnchorCell 中心对齐 ScreenAbsPos */
    void UpdateShapePreviewPosition(const FGeometry& MyGeometry, FVector2D ScreenAbsPos);
    /** 拖拽结束/取消：清掉所有 cell + 隐藏 */
    void HideShapePreview();

    UPROPERTY()
    TArray<TObjectPtr<UImage>> ShapePreviewCells;
    FIntPoint ShapePreviewAnchorCell = FIntPoint(0, 0);
    float     ShapePreviewCellPx     = 64.f;
    bool      bShapePreviewActive    = false;

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
    void OnConfirmButtonClicked();

};
