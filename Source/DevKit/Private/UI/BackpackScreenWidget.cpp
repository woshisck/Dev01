#include "UI/BackpackScreenWidget.h"
#include "UI/BackpackGridWidget.h"
#include "UI/PendingGridWidget.h"
#include "UI/RuneDragDropOperation.h"
#include "UI/RuneTooltipWidget.h"
#include "UI/RuneInfoCardWidget.h"
#include "UI/CombatDeckEditWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogPlayerControllerBase.h"
#include "CommonInputSubsystem.h"
#include "Input/CommonUIInputTypes.h"
#include "GameFramework/Pawn.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/SizeBox.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "InputCoreTypes.h"
#include "UI/YogHUD.h"
#include "GameModes/YogGameMode.h"
#include "Tutorial/TutorialManager.h"
#include "TimerManager.h"

// ============================================================
//  生命周期
// ============================================================

UBackpackScreenWidget::UBackpackScreenWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    bIsFocusable = true;
}

void UBackpackScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Log, TEXT("[BackpackScreen] NativeConstruct outer=%s"), *GetNameSafe(GetOuter()));

    SetVisibility(ESlateVisibility::Collapsed);

    // 子 Widget 初始化（NativeConstruct 时子 Widget 已构建完毕）
    if (BackpackGridWidget)
        BackpackGridWidget->BuildGrid();

    if (PendingGridWidget)
    {
        PendingGridWidget->BuildSlots();
        PendingCols = PendingGridWidget->PendingGridCols;
        PendingRows = PendingGridWidget->PendingGridRows;
    }

    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->BindToOwningPlayerCombatDeck();
        CombatDeckEditWidget->SetInteractionLocked(IsInCombatPhase() || bIsPreviewMode);
    }

    RefreshPendingRuneSlots();

    if (SellButton)
        SellButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnSellButtonClicked);

    if (CloseButton)
        CloseButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnCloseButtonClicked);

    if (EndPreviewButton)
    {
        EndPreviewButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnEndPreviewClicked);
        EndPreviewButton->SetVisibility(ESlateVisibility::Collapsed);  // 默认隐藏，进入预览模式时由 SetPreviewMode 切到 Visible
    }
    // CloseButton 默认让 WBP Designer 设的 Visibility 生效（通常是 Visible），
    // SetPreviewMode 会在切换时强制覆盖为 Visible/Collapsed

    if (BackpackGridWidget)
        BackpackGridWidget->OnHeatPhaseButtonClicked.AddDynamic(
            this, &UBackpackScreenWidget::HandleHeatPhaseButtonClicked);

    if (HintText)
        HintText->SetVisibility(ESlateVisibility::Collapsed);
}

void UBackpackScreenWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

// ============================================================
//  委托处理
// ============================================================

void UBackpackScreenWidget::HandleRunePlaced(const FRuneInstance& Rune)
{
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::HandleRuneRemoved(FGuid RuneGuid)
{
    OnGridNeedsRefresh();
}

void UBackpackScreenWidget::HandleRuneActivationChanged(FGuid RuneGuid, bool bActivated)
{
    OnGridNeedsRefresh();
}

// ============================================================
//  格子刷新（委托给 BackpackGridWidget）
// ============================================================

void UBackpackScreenWidget::OnGridNeedsRefresh_Implementation()
{
    if (BackpackGridWidget)
    {
        // 手柄模式下直接用 GamepadCursorCell 作为悬浮格，
        // 不依赖 HoverCol/Row，防止合成鼠标事件将其清零后一帧内高亮消失。
        const FIntPoint EffectiveHover = (bIsGamepadInputMode && !bGrabbingRune)
            ? GamepadCursorCell
            : FIntPoint(HoverCol, HoverRow);

        BackpackGridWidget->RefreshCells(
            SelectedCell,
            EffectiveHover,
            GrabbedFromCell,
            bGrabbingRune,
            PreviewPhase);
        BackpackGridWidget->RefreshHeatPhaseButtons(PreviewPhase, bIsGamepadInputMode);
    }
}

// ============================================================
//  详情面板刷新
// ============================================================

void UBackpackScreenWidget::OnSelectionChanged_Implementation()
{
    FRuneInstance Info = GetFocusedRuneInfo();
    const bool bHasSelection = Info.RuneGuid.IsValid();

    if (DetailPanel)
        DetailPanel->SetVisibility(bHasSelection ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

    if (bHasSelection)
    {
        if (DetailName)
            DetailName->SetText(FText::FromName(Info.RuneConfig.RuneName));

        if (DetailDesc)
            DetailDesc->SetText(Info.RuneConfig.RuneDescription);

        if (DetailIcon)
        {
            if (Info.RuneConfig.RuneIcon)
            {
                DetailIcon->SetBrushFromTexture(Info.RuneConfig.RuneIcon, false);
                DetailIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
            }
            else
            {
                DetailIcon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
    }

    if (HintText)
    {
        if (SelectedCell != FIntPoint(-1, -1))
        {
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintMove", "已选中：{0}\n拖拽格子 → 移动\n「移除选中」→ 移除"),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else if (SelectedRuneIndex >= 0)
        {
            HintText->SetText(FText::Format(
                NSLOCTEXT("Backpack", "HintPlace", "已选中：{0}\n点击背包格子 → 放置"),
                FText::FromName(Info.RuneConfig.RuneName)));
        }
        else
        {
            HintText->SetText(NSLOCTEXT("Backpack", "HintIdle", "拖拽格子中的符文可移动位置\n点击左侧列表选择待放置符文"));
        }
    }

    if (RuneInfoCard)
    {
        if (bHasSelection)
            RuneInfoCard->ShowRune(Info);
        else
            RuneInfoCard->HideCard();
    }

    OnGridNeedsRefresh();
}

// ============================================================
//  状态查询
// ============================================================

bool UBackpackScreenWidget::IsCellInActivationZone(int32 Col, int32 Row) const
{
    return false;
}

bool UBackpackScreenWidget::IsCellOccupied(int32 Col, int32 Row) const
{
    return false;
}

FRuneInstance UBackpackScreenWidget::GetFocusedRuneInfo() const
{
    return GetSelectedRuneInfo();
}

UTexture2D* UBackpackScreenWidget::GetRuneIconAtCell(int32 Col, int32 Row) const
{
    return nullptr;
}

EBackpackCellState UBackpackScreenWidget::GetCellVisualState(int32 Col, int32 Row) const
{
    return EBackpackCellState::Empty;
}

TArray<FRuneInstance> UBackpackScreenWidget::GetRuneList() const
{
    TArray<FRuneInstance> Result;
    if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
        Result.Append(Player->PendingRunes);

    for (const TObjectPtr<URuneDataAsset>& DA : AvailableRunes)
        if (DA) Result.Add(DA->RuneInfo);

    return Result;
}

int32 UBackpackScreenWidget::GetPendingRuneCount() const
{
    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    return Player ? Player->PendingRunes.Num() : 0;
}

FRuneInstance UBackpackScreenWidget::GetSelectedRuneInfo() const
{
    TArray<FRuneInstance> RuneList = GetRuneList();
    if (!RuneList.IsValidIndex(SelectedRuneIndex)) return FRuneInstance();
    return RuneList[SelectedRuneIndex];
}

// ============================================================
//  操作
// ============================================================

void UBackpackScreenWidget::SelectRuneFromList(int32 Index)
{
    SelectedRuneIndex = (SelectedRuneIndex == Index) ? -1 : Index;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}

void UBackpackScreenWidget::ClickCell(int32 Col, int32 Row)
{
    if (bIsPreviewMode) return;

    // Grid has no placed runes without BGC — clicking empty cells just clears selection
    const FIntPoint Cell(Col, Row);
    if (SelectedCell != FIntPoint(-1, -1) || SelectedRuneIndex >= 0)
    {
        SelectedCell      = FIntPoint(-1, -1);
        SelectedRuneIndex = -1;
        OnSelectionChanged();
    }
}

void UBackpackScreenWidget::RemoveRuneAtSelectedCell()
{
}

void UBackpackScreenWidget::ClearSelection()
{
    SelectedRuneIndex = -1;
    SelectedCell = FIntPoint(-1, -1);
    OnSelectionChanged();
}

// ============================================================
//  待放置区刷新（委托给 PendingGridWidget）
// ============================================================

void UBackpackScreenWidget::RefreshPendingRuneSlots()
{
    RefreshPendingGrid();
}

// ── 待放置区稀疏格子辅助 ─────────────────────────────────────────────────

void UBackpackScreenWidget::SyncPendingFromPlayer()
{
    PendingCols = PendingGridWidget ? FMath::Max(1, PendingGridWidget->PendingGridCols) : 2;
    PendingRows = PendingGridWidget ? FMath::Max(1, PendingGridWidget->PendingGridRows) : 4;
    PendingGrid.SetNum(PendingCols * PendingRows);
    for (FRuneInstance& R : PendingGrid) R = FRuneInstance();

    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    if (!Player) return;
    for (int32 i = 0; i < Player->PendingRunes.Num() && i < PendingGrid.Num(); i++)
        PendingGrid[i] = Player->PendingRunes[i];
}

void UBackpackScreenWidget::SyncPendingToPlayer()
{
    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    if (!Player) return;
    Player->PendingRunes.Empty();
    for (const FRuneInstance& R : PendingGrid)
        if (R.RuneGuid.IsValid())
            Player->PendingRunes.Add(R);
}

void UBackpackScreenWidget::RefreshPendingGrid()
{
    if (!PendingGridWidget) return;
    const int32 CursorIdx  = bCursorInPendingArea ? PendingCursorIdx : -1;
    const int32 GrabbedIdx = bGrabbingFromPending ? PendingGrabbedIdx : -1;
    PendingGridWidget->RefreshSlots(PendingGrid, CursorIdx, GrabbedIdx);
}

void UBackpackScreenWidget::ClearPendingFocus(bool bClearSelection)
{
    bool bDirty = false;
    if (bCursorInPendingArea)
    {
        bCursorInPendingArea = false;
        bDirty = true;
    }
    if (bClearSelection && PendingSelectedIdx >= 0)
    {
        PendingSelectedIdx = -1;
        bDirty = true;
    }
    if (bDirty) RefreshPendingGrid();
}

void UBackpackScreenWidget::UpdateOperationHintVisibility()
{
    if (!OperationHintWidget) return;

    // 抓取状态下显示按键提示；预览/战斗模式不应进入抓取状态，但保险起见再 guard 一次
    const bool bShow = bGrabbingRune && !bIsPreviewMode && !IsInCombatPhase();
    if (bShow == bOperationHintVisible) return;  // 缓存：避免每帧重复 SetVisibility
    bOperationHintVisible = bShow;
    OperationHintWidget->SetVisibility(bShow
        ? ESlateVisibility::SelfHitTestInvisible
        : ESlateVisibility::Collapsed);
}

// ============================================================
//  CommonUI 生命周期
// ============================================================

TOptional<FUIInputConfig> UBackpackScreenWidget::GetDesiredInputConfig() const
{
    return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
}

void UBackpackScreenWidget::NativeOnActivated()
{
    Super::NativeOnActivated();
    SetVisibility(ESlateVisibility::Visible);

    if (APlayerController* PC = GetOwningPlayer())
        if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
        {
            HUD->BeginPauseEffect();
            HUD->PushMajorUI();
        }

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetPause(true);
        PC->SetShowMouseCursor(true);
        // 记录当前光标位置，防止激活后合成鼠标事件被误判为真实移动而清除手柄高亮
        float MouseX = 0.f, MouseY = 0.f;
        PC->GetMousePosition(MouseX, MouseY);
        LastMouseAbsPos = FVector2D(MouseX, MouseY);
    }

    SetUserFocus(GetOwningPlayer());

    // 每次打开时重建格子
    if (BackpackGridWidget)
        BackpackGridWidget->BuildGrid();

    SyncPendingFromPlayer();
    RefreshPendingGrid();

    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->BindToOwningPlayerCombatDeck();
        CombatDeckEditWidget->SetInteractionLocked(IsInCombatPhase() || bIsPreviewMode);
    }

    OnGridNeedsRefresh();
    OnSelectionChanged();

    // Tutorial ③：第一次打开背包时弹窗（state guard 内部去重）
    if (APlayerController* PC = GetOwningPlayer())
        if (UGameInstance* GI = GetGameInstance())
            if (UTutorialManager* TM = GI->GetSubsystem<UTutorialManager>())
                TM->TryBackpackTutorial(PC);

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimerForNextTick(
            FTimerDelegate::CreateUObject(this, &UBackpackScreenWidget::LogLayoutDiagnostics));
    }
}

void UBackpackScreenWidget::LogLayoutDiagnostics()
{
    const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
    const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);

    UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] Viewport=%.0fx%.0f Scale=%.2f Pending=%dx%d PreviewPhase=%d SelectedCell=(%d,%d) PendingSelected=%d Grabbing=%d PreviewMode=%d"),
        ViewportSize.X, ViewportSize.Y, ViewportScale,
        PendingCols, PendingRows, PreviewPhase,
        SelectedCell.X, SelectedCell.Y, PendingSelectedIdx,
        bGrabbingRune ? 1 : 0,
        bIsPreviewMode ? 1 : 0);

    auto LogWidget = [](const TCHAR* Name, const UWidget* Widget)
    {
        if (!Widget)
        {
            UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] %s=null"), Name);
            return;
        }

        const FGeometry& Geo = Widget->GetCachedGeometry();
        const FVector2D LocalSize = Geo.GetLocalSize();
        const FVector2D AbsPos = Geo.GetAbsolutePosition();
        UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] %s Vis=%d Local=%.1fx%.1f Abs=(%.1f,%.1f)"),
            Name,
            static_cast<int32>(Widget->GetVisibility()),
            LocalSize.X, LocalSize.Y,
            AbsPos.X, AbsPos.Y);
    };

    LogWidget(TEXT("Screen"), this);
    LogWidget(TEXT("BackpackGridWidget"), BackpackGridWidget);
    LogWidget(TEXT("BackpackGridPanel"), BackpackGridWidget ? BackpackGridWidget->BackpackGrid.Get() : nullptr);
    LogWidget(TEXT("BackpackGridSizeBox"), BackpackGridWidget ? BackpackGridWidget->GridSizeBox.Get() : nullptr);
    LogWidget(TEXT("PendingGridWidget"), PendingGridWidget);
    LogWidget(TEXT("PendingGridPanel"), PendingGridWidget ? PendingGridWidget->PendingRuneGrid.Get() : nullptr);
    LogWidget(TEXT("PendingGridSizeBox"), PendingGridWidget ? PendingGridWidget->PendingGridSizeBox.Get() : nullptr);
    LogWidget(TEXT("OperationHintWidget"), OperationHintWidget);
    LogWidget(TEXT("RuneInfoCard"), RuneInfoCard);

    const UBackpackStyleDataAsset* GridStyle = BackpackGridWidget ? BackpackGridWidget->StyleDA.Get() : nullptr;
    const UBackpackStyleDataAsset* PendingStyle = PendingGridWidget ? PendingGridWidget->StyleDA.Get() : nullptr;
    UE_LOG(LogTemp, Warning, TEXT("[BackpackUILayout] GridStyle=%s PendingStyle=%s GridCell=%.1f GridPadding=%.1f PendingCell=%.1f PendingPadding=%.1f"),
        *GetNameSafe(GridStyle),
        *GetNameSafe(PendingStyle),
        GridStyle ? GridStyle->CellSize : -1.f,
        GridStyle ? GridStyle->CellPadding : -1.f,
        PendingStyle ? PendingStyle->CellSize : -1.f,
        PendingStyle ? PendingStyle->CellPadding : -1.f);
}

void UBackpackScreenWidget::NativeOnDeactivated()
{
    SetVisibility(ESlateVisibility::Collapsed);

    // 预览模式：跳过 SyncPendingToPlayer（背包只读，无需写回数据）
    // 但 Pause/Input/PauseEffect 拆解必须执行：
    //   - EndPauseEffect 必须配对 NativeOnActivated 里的 BeginPauseEffect，否则 PausePopupCount 泄漏
    //   - SetPause(false) / SetInputMode(GameOnly) 由调用方（LootSelection.ReactivateAfterPreview）随后再恢复
    const bool bSkipDataSync = bIsPreviewMode;

    if (!bSkipDataSync)
    {
        SyncPendingToPlayer();
    }

    bCursorInPendingArea = false;
    bGrabbingFromPending = false;
    PendingGrabbedIdx    = -1;
    PendingSelectedIdx   = -1;

    bGrabbingRune   = false;
    GrabbedFromCell = FIntPoint(-1, -1);
    PreviewPhase    = -1;
    ClearSelection();

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetPause(false);
        PC->SetShowMouseCursor(false);
        // AddToViewport 绕过 CommonUI Stack，输入模式不会自动还原，必须手动设回 GameOnly
        PC->SetInputMode(FInputModeGameOnly());
    }

    if (APlayerController* PC = GetOwningPlayer())
        if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
        {
            HUD->EndPauseEffect();
            HUD->PopMajorUI();
        }

    Super::NativeOnDeactivated();

    // 统一走 SetPreviewMode(false) 触发 BP 事件 + 切换按钮显隐 + 清理状态
    // 不再裸写 bIsPreviewMode = false（避免漏触发 OnPreviewModeChanged 和按钮复位）
    SetPreviewMode(false);
}

void UBackpackScreenWidget::SetPreviewMode(bool bReadOnly)
{
    if (bIsPreviewMode == bReadOnly && !bReadOnly)
    {
        // 同状态切 false 时也允许执行（NativeOnDeactivated 兜底场景）
    }
    bIsPreviewMode = bReadOnly;
    UE_LOG(LogTemp, Log, TEXT("[Backpack] SetPreviewMode(%s)"), bReadOnly ? TEXT("true") : TEXT("false"));

    // 进入预览模式时清理所有可能导致"卡在抓取/拖拽中"的中间状态
    if (bIsPreviewMode)
    {
        bGrabbingRune        = false;
        bGrabbingFromPending = false;
        bMouseDragging       = false;
        GrabbedFromCell      = FIntPoint(-1, -1);
        PendingGrabbedIdx    = -1;
        PendingDragIndex     = -1;
        PendingDragCol       = PendingDragRow = -1;
        HoverCol = HoverRow  = -1;
        MouseDragTex         = nullptr;
        HideShapePreview();
        ClearSelection();
        OnGridNeedsRefresh();
        RefreshPendingGrid();
    }

    // EndPreviewButton 与 CloseButton 互斥显隐 — 必须用 Visible 而非 SelfHitTestInvisible，
    // 否则按钮本体不参与命中测试，OnClicked 不触发
    if (EndPreviewButton)
        EndPreviewButton->SetVisibility(bIsPreviewMode
            ? ESlateVisibility::Visible
            : ESlateVisibility::Collapsed);
    if (CloseButton)
        CloseButton->SetVisibility(bIsPreviewMode
            ? ESlateVisibility::Collapsed
            : ESlateVisibility::Visible);

    if (CombatDeckEditWidget)
    {
        CombatDeckEditWidget->SetInteractionLocked(IsInCombatPhase() || bIsPreviewMode);
    }

}

void UBackpackScreenWidget::OnEndPreviewClicked()
{
    if (!bIsPreviewMode) return;
    UE_LOG(LogTemp, Log, TEXT("[Backpack] OnEndPreviewClicked → DeactivateWidget"));
    // DeactivateWidget → NativeOnDeactivated → HUD 监听器回调 LootSelection.ReactivateAfterPreview
    DeactivateWidget();
}

// ============================================================
//  Shape 拖拽预览（Phase 2）
// ============================================================

void UBackpackScreenWidget::ShowShapePreview(const FRuneInstance& Rune, FIntPoint AnchorCellInRotatedLocal,
                                              UTexture2D* IconTex, float CellPx)
{
    if (!ShapePreviewCanvas) return;  // BindWidgetOptional：未绑定就回退到 GrabbedRuneIcon

    HideShapePreview();

    // 计算 N 次旋转后的 Shape（同 BackpackGridWidget 的渲染规约）
    FRuneShape RotShape = Rune.Shape;
    const int32 N = ((Rune.Rotation % 4) + 4) % 4;
    for (int32 i = 0; i < N; i++)
        RotShape = RotShape.Rotate90();

    // 为每个 cell 创建 UImage
    for (const FIntPoint& Cell : RotShape.Cells)
    {
        UImage* CellImg = NewObject<UImage>(this);
        if (IconTex)
            CellImg->SetBrushFromTexture(IconTex, false);
        // pivot/anchor cell 不透明，其它 cell 半透明做视觉区分
        const float Alpha = (Cell == AnchorCellInRotatedLocal) ? 1.0f : 0.65f;
        CellImg->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Alpha));

        UPanelSlot* PSlot = ShapePreviewCanvas->AddChild(CellImg);
        if (UCanvasPanelSlot* CSlot = Cast<UCanvasPanelSlot>(PSlot))
        {
            CSlot->SetAutoSize(false);
            CSlot->SetPosition(FVector2D(Cell.X * CellPx, Cell.Y * CellPx));
            CSlot->SetSize(FVector2D(CellPx, CellPx));
        }
        ShapePreviewCells.Add(CellImg);
    }

    ShapePreviewAnchorCell = AnchorCellInRotatedLocal;
    ShapePreviewCellPx     = CellPx;
    bShapePreviewActive    = true;
    ShapePreviewCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UBackpackScreenWidget::UpdateShapePreviewPosition(const FGeometry& MyGeometry, FVector2D ScreenAbsPos)
{
    if (!bShapePreviewActive || !ShapePreviewCanvas) return;
    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenAbsPos);
    // 让 anchor cell 的中心（不是左上角）对齐鼠标位置 — 视觉上 Pivot 跟手感觉更自然
    const FVector2D AnchorOffset(
        (ShapePreviewAnchorCell.X + 0.5f) * ShapePreviewCellPx,
        (ShapePreviewAnchorCell.Y + 0.5f) * ShapePreviewCellPx);
    FWidgetTransform T;
    T.Translation = LocalPos - AnchorOffset;
    ShapePreviewCanvas->SetRenderTransform(T);
}

void UBackpackScreenWidget::HideShapePreview()
{
    if (!ShapePreviewCanvas) return;
    for (UImage* Img : ShapePreviewCells)
    {
        if (Img) ShapePreviewCanvas->RemoveChild(Img);
    }
    ShapePreviewCells.Reset();
    ShapePreviewCanvas->SetVisibility(ESlateVisibility::Collapsed);
    bShapePreviewActive = false;
}

// ============================================================
//  出售按钮
// ============================================================

void UBackpackScreenWidget::OnSellButtonClicked()
{
}

void UBackpackScreenWidget::OnCloseButtonClicked()
{
    DeactivateWidget();
}

bool UBackpackScreenWidget::IsInCombatPhase() const
{
    if (AYogGameMode* GM = GetWorld()->GetAuthGameMode<AYogGameMode>())
        return GM->CurrentPhase == ELevelPhase::Combat;
    return false;
}

void UBackpackScreenWidget::HandleHeatPhaseButtonClicked(int32 Phase)
{
    PreviewPhase = (PreviewPhase == Phase) ? -1 : Phase;
    OnGridNeedsRefresh();
}

// ============================================================
//  坐标辅助（委托给子 Widget）
// ============================================================

bool UBackpackScreenWidget::GetGridCellAtScreenPos(const FVector2D& AbsolutePos, int32& OutCol, int32& OutRow) const
{
    if (!BackpackGridWidget) return false;
    return BackpackGridWidget->GetCellAtScreenPos(AbsolutePos, OutCol, OutRow);
}

bool UBackpackScreenWidget::GetPendingSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const
{
    if (!PendingGridWidget) return false;
    return PendingGridWidget->GetSlotAtScreenPos(AbsPos, OutIndex);
}

// ============================================================
//  拖拽事件实现
// ============================================================

FReply UBackpackScreenWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[BackpackScreen] MouseButtonDown button=%s pos=(%f,%f)"),
        *InMouseEvent.GetEffectingButton().ToString(),
        InMouseEvent.GetScreenSpacePosition().X, InMouseEvent.GetScreenSpacePosition().Y);

    // 右键 → 取消抓取
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        if (bGrabbingRune || bGrabbingFromPending)
        {
            GamepadCancel();
            return FReply::Handled();
        }
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
    }

    if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    bool bCommonInputGamepad = false;
    if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
    {
        if (UCommonInputSubsystem* CommonInput = LocalPlayer->GetSubsystem<UCommonInputSubsystem>())
        {
            bCommonInputGamepad = CommonInput->GetCurrentInputType() == ECommonInputType::Gamepad;
        }
    }

    const bool bGamepadAIsDown = GetOwningPlayer()
        && GetOwningPlayer()->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
    const bool bTreatAsGamepadSelect =
        bIsGamepadInputMode || bCommonInputGamepad || bGamepadAIsDown || bDeckSelectButtonWasDown;

    if (bTreatAsGamepadSelect && CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        bIsGamepadInputMode = true;
        bDeckSelectFromVirtualMouse = true;
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] VirtualMouseAsA Mode=%d Common=%d ADown=%d WasDown=%d"),
            bIsGamepadInputMode ? 1 : 0,
            bCommonInputGamepad ? 1 : 0,
            bGamepadAIsDown ? 1 : 0,
            bDeckSelectButtonWasDown ? 1 : 0);
        return HandleCombatDeckSelectButtonState(true, TEXT("VirtualMouseDown")).CaptureMouse(TakeWidget());
    }

    if (bIsGamepadInputMode)
    {
        bIsGamepadInputMode = false;
        if (BackpackGridWidget)
        {
            BackpackGridWidget->RefreshHeatPhaseButtons(PreviewPhase, false);
        }
    }

    // 左侧待放置槽：优先于主格子
    {
        int32 PendingIdx;
        if (GetPendingSlotAtScreenPos(InMouseEvent.GetScreenSpacePosition(), PendingIdx))
        {
            bCursorInPendingArea = true;
            PendingCursorIdx     = PendingIdx;

            const bool bHasRune = PendingGrid.IsValidIndex(PendingIdx)
                && PendingGrid[PendingIdx].RuneGuid.IsValid();

            if (bHasRune)
            {
                PendingSelectedIdx = PendingIdx;
                SelectedCell       = FIntPoint(-1, -1);
                SelectedRuneIndex  = -1;
                RefreshPendingGrid();
                OnSelectionChanged();

                // 预览模式 / 战斗阶段：弹消息 + 禁止 DetectDrag（pending 无 shake，主格子有）
                if (IsInCombatPhase() || bIsPreviewMode)
                {
                    OnStatusMessage(bIsPreviewMode
                        ? NSLOCTEXT("Backpack", "PreviewLock", "预览模式：无法操作符文")
                        : NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文"));
                    return FReply::Handled();
                }
                PendingDragIndex = PendingIdx;
                return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
            }
            else if (PendingSelectedIdx >= 0
                && PendingGrid.IsValidIndex(PendingSelectedIdx)
                && PendingGrid[PendingSelectedIdx].RuneGuid.IsValid())
            {
                if (bIsPreviewMode) return FReply::Handled();  // 预览模式：禁止 pending 内 swap
                PendingGrid[PendingIdx]          = PendingGrid[PendingSelectedIdx];
                PendingGrid[PendingSelectedIdx]  = FRuneInstance();
                SyncPendingToPlayer();
                PendingSelectedIdx = PendingIdx;
                PendingCursorIdx   = PendingIdx;
                RefreshPendingGrid();
                return FReply::Handled();
            }
            else
            {
                PendingSelectedIdx = -1;
                RefreshPendingGrid();
                return FReply::Handled();
            }
        }
    }

    int32 Col, Row;
    if (!GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    const FIntPoint ClickCell(Col, Row);

    // 焦点切到主背包：清 pending 黄框 + 待放置选择（鼠标路径不支持 click-to-place from pending）
    ClearPendingFocus(true);

    // ── 非抓取：点击占用格 → 进入抓取/悬浮 ─────────────────────────────────
    if (IsCellOccupied(Col, Row))
    {
        SelectedCell      = ClickCell;
        SelectedRuneIndex = -1;
        GamepadCursorCell = ClickCell;
        OnSelectionChanged();

        // 预览模式 / 战斗阶段：与战斗一致 — shake + 闪红光 + 状态提示，禁止抓取
        if (IsInCombatPhase() || bIsPreviewMode)
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
            OnStatusMessage(bIsPreviewMode
                ? NSLOCTEXT("Backpack", "PreviewLock", "预览模式：无法操作符文")
                : NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文"));
            return FReply::Handled();
        }

        bGrabbingRune     = true;
        GrabbedFromCell   = ClickCell;
        HoverCol = HoverRow = -1;

        PendingDragCol = Col;
        PendingDragRow = Row;
        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    else
    {
        PendingDragCol    = PendingDragRow = -1;
        if (SelectedCell != FIntPoint(-1, -1) || SelectedRuneIndex >= 0)
        {
            SelectedCell      = FIntPoint(-1, -1);
            SelectedRuneIndex = -1;
            OnSelectionChanged();
        }
        return FReply::Handled();
    }
}

FReply UBackpackScreenWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[BackpackScreen] MouseButtonUp button=%s pos=(%f,%f) VirtualDeck=%d"),
        *InMouseEvent.GetEffectingButton().ToString(),
        InMouseEvent.GetScreenSpacePosition().X,
        InMouseEvent.GetScreenSpacePosition().Y,
        bDeckSelectFromVirtualMouse ? 1 : 0);

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bDeckSelectFromVirtualMouse)
    {
        bDeckSelectFromVirtualMouse = false;
        return HandleCombatDeckSelectButtonState(false, TEXT("VirtualMouseUp")).ReleaseMouseCapture();
    }

    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton &&
        bIsGamepadInputMode &&
        bDeckSelectButtonWasDown &&
        CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] VirtualMouseUpAsARelease"));
        return HandleCombatDeckSelectButtonState(false, TEXT("VirtualMouseUpFallback")).ReleaseMouseCapture();
    }

    return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UBackpackScreenWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    if (bIsPreviewMode) return;  // 只读预览模式：禁止拖拽

    // 拖拽接管：清除点击抓取状态
    bGrabbingRune     = false;
    GrabbedFromCell   = FIntPoint(-1,-1);

    if (PendingDragIndex >= 0)
    {
        const bool bValid = PendingGrid.IsValidIndex(PendingDragIndex)
            && PendingGrid[PendingDragIndex].RuneGuid.IsValid();
        if (bValid)
        {
            if (IsInCombatPhase())
            {
                PendingDragIndex = -1;
                return;
            }

            const FRuneInstance PendingRune = PendingGrid[PendingDragIndex];
            URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
            DragOp->SrcCol             = -1;
            DragOp->SrcRow             = -1;
            DragOp->SrcPivot           = FIntPoint(-1, -1);
            DragOp->DraggedRune        = PendingRune;
            DragOp->PendingSourceIndex = PendingDragIndex;

            bMouseDragging  = true;
            MouseDragTex    = PendingRune.RuneConfig.RuneIcon;
            LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

            // Phase 2: 显示完整 Shape 跟随鼠标，AnchorCell = Pivot 在旋转后 Shape 的位置
            if (ShapePreviewCanvas && BackpackGridWidget)
            {
                const FIntPoint AnchorCell = PendingRune.Shape.GetPivotOffset(PendingRune.Rotation);
                const FVector2D GridSize   = BackpackGridWidget->GetGridGeometry().GetLocalSize();
                const int32 GW = 5;
                const float CellPx = (GridSize.X > 0.f) ? (GridSize.X / GW) : 64.f;
                ShowShapePreview(PendingRune, AnchorCell, MouseDragTex, CellPx);
                // 首帧立即对位，避免出现在 (0,0) 一帧
                UpdateShapePreviewPosition(InGeometry, InMouseEvent.GetScreenSpacePosition());
            }

            PendingDragIndex = -1;
            OutOperation = DragOp;
            return;
        }
        PendingDragIndex = -1;
        return;
    }

    PendingDragCol = PendingDragRow = -1;
}

bool UBackpackScreenWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (!Cast<URuneDragDropOperation>(InOperation))
        return false;

    LastMouseAbsPos = InDragDropEvent.GetScreenSpacePosition();

    // Phase 2: 让完整 Shape 预览跟随鼠标
    UpdateShapePreviewPosition(InGeometry, InDragDropEvent.GetScreenSpacePosition());

    int32 Col, Row;
    if (GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), Col, Row))
    {
        if (HoverCol != Col || HoverRow != Row)
        {
            HoverCol = Col;
            HoverRow = Row;
            OnGridNeedsRefresh();
        }
    }
    else if (HoverCol != -1 || HoverRow != -1)
    {
        HoverCol = HoverRow = -1;
        OnGridNeedsRefresh();
    }

    return true;
}

bool UBackpackScreenWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (bIsPreviewMode)
    {
        HideShapePreview();  // 预览模式提早返回也要清理 Shape 预览
        return false;
    }

    bMouseDragging = false;
    MouseDragTex   = nullptr;
    HoverCol = HoverRow = -1;
    HideShapePreview();  // Phase 2: 所有 Drop 路径统一在入口清掉 Shape 预览

    URuneDragDropOperation* RuneOp = Cast<URuneDragDropOperation>(InOperation);
    if (!RuneOp)
    {
        OnGridNeedsRefresh();
        return false;
    }

    // ── 主格子符文拖回待放置区（取回） ──────────────────────────────────────
    // 只要落点在主格子面板左侧（含 Pending 区域和两者之间的空白），都触发取回
    if (RuneOp->PendingSourceIndex < 0 && RuneOp->DraggedRune.RuneGuid.IsValid())
    {
        bool bShouldUnplace = false;

        // 精确命中 Pending 面板
        int32 PendingTargetIdx;
        bShouldUnplace = GetPendingSlotAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), PendingTargetIdx);

        // 扩展：落点在主格子面板左侧也算取回
        if (!bShouldUnplace && BackpackGridWidget)
        {
            const FGeometry GridGeo  = BackpackGridWidget->GetGridGeometry();
            const FVector2D LocalInGrid = GridGeo.AbsoluteToLocal(InDragDropEvent.GetScreenSpacePosition());
            bShouldUnplace = (LocalInGrid.X < 0.f);
        }

        // No BGC — cannot unplace
    }

    if (RuneOp->PendingSourceIndex >= 0)
    {
        if (!PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex))
        {
            OnGridNeedsRefresh();
            return false;
        }

        // ── pending → pending 交换 ──
        {
            int32 PendingTargetIdx;
            if (PendingGridWidget && PendingGridWidget->GetSlotAtScreenPos(
                    InDragDropEvent.GetScreenSpacePosition(), PendingTargetIdx)
                && PendingTargetIdx != RuneOp->PendingSourceIndex)
            {
                const FRuneInstance Src = PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex)
                    ? PendingGrid[RuneOp->PendingSourceIndex] : FRuneInstance();
                const FRuneInstance Dst = PendingGrid.IsValidIndex(PendingTargetIdx)
                    ? PendingGrid[PendingTargetIdx] : FRuneInstance();
                if (PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex))
                    PendingGrid[RuneOp->PendingSourceIndex] = Dst;
                if (PendingGrid.IsValidIndex(PendingTargetIdx))
                    PendingGrid[PendingTargetIdx] = Src;
                SyncPendingToPlayer();
                PendingSelectedIdx = PendingTargetIdx;
                PendingCursorIdx   = PendingTargetIdx;
                RefreshPendingGrid();
                return true;
            }
        }

        OnGridNeedsRefresh();
        return false;
    }

    OnGridNeedsRefresh();
    return false;
}

void UBackpackScreenWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    bMouseDragging = false;
    MouseDragTex   = nullptr;
    HoverCol       = HoverRow       = -1;
    PendingDragCol = PendingDragRow = -1;
    HideShapePreview();  // Phase 2: 拖拽取消（如 ESC / 右键）也清掉 Shape 预览
    OnGridNeedsRefresh();
}

// ============================================================
//  手柄输入
// ============================================================

FReply UBackpackScreenWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    const FVector2D NewPos = InMouseEvent.GetScreenSpacePosition();
    // 只有光标真实移动超过 2px 才算"真实移动"；零位移或极小偏移视为合成事件，
    // 保持手柄模式，防止多次合成事件反复将 HoverCol/Row 清零导致高亮闪烁。
    const bool bRealMove = (NewPos - LastMouseAbsPos).SizeSquared() > 4.f;

    if (bIsGamepadInputMode)
    {
        LastMouseAbsPos = NewPos;
        if (!bRealMove)
            return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
        bIsGamepadInputMode = false;
        if (BackpackGridWidget)
            BackpackGridWidget->RefreshHeatPhaseButtons(PreviewPhase, false);
    }
    else
    {
        LastMouseAbsPos = NewPos;
    }

    // 无抓取/拖拽时追踪主格子悬浮格，驱动绿框高亮
    if (!bGrabbingRune && !bMouseDragging)
    {
        int32 NewHCol = -1, NewHRow = -1;
        int32 Col, Row;
        if (GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        {
            NewHCol = Col;
            NewHRow = Row;
        }
        if (HoverCol != NewHCol || HoverRow != NewHRow)
        {
            HoverCol = NewHCol;
            HoverRow = NewHRow;
            OnGridNeedsRefresh();
        }
    }

    return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UBackpackScreenWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackKeyUp] Key=%s HasDeckWidget=%d CanDeck=%d"),
        *InKeyEvent.GetKey().ToString(),
        CombatDeckEditWidget ? 1 : 0,
        (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput()) ? 1 : 0);

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput() &&
        InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Bottom)
    {
        return HandleCombatDeckSelectButtonState(false, TEXT("KeyUp"));
    }

    if (InKeyEvent.GetKey() == HeldDirKey)
    {
        bDirKeyHeld    = false;
        HeldKeyTime    = 0.f;
        LastRepeatCount = 0;
    }
    return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

void UBackpackScreenWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    UpdateOperationHintVisibility();

    PollCombatDeckSelectButtonState();

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        CombatDeckEditWidget->TickDeckGamepadInput(InDeltaTime);
    }

    if (GrabbedRuneIcon)
    {
        const UBackpackStyleDataAsset* Style = BackpackGridWidget ? BackpackGridWidget->StyleDA.Get() : nullptr;
        const float CellPx  = Style ? Style->CellSize : 64.f;
        const float HalfPx  = CellPx * 0.5f;
        const float FloatY  = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) * 5.f;

        // Phase 2: 如果 ShapePreview 已激活（WBP 绑了 ShapePreviewCanvas），不再显示单图标 GrabbedRuneIcon，
        // 避免拖拽时双图标叠加
        if (bShapePreviewActive)
        {
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
        else if (bMouseDragging && MouseDragTex)
        {
            GrabbedRuneIcon->SetBrushFromTexture(MouseDragTex, false);
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

            const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(LastMouseAbsPos);
            FWidgetTransform T;
            T.Translation = LocalPos + FVector2D(-HalfPx, -HalfPx + FloatY);
            GrabbedRuneIcon->SetRenderTransform(T);
        }
        else if ((bGrabbingRune || bGrabbingFromPending) && BackpackGridWidget)
        {
            UTexture2D* Tex = bGrabbingFromPending
                ? (PendingGrid.IsValidIndex(PendingGrabbedIdx) ? PendingGrid[PendingGrabbedIdx].RuneConfig.RuneIcon.Get() : nullptr)
                : GetRuneIconAtCell(GrabbedFromCell.X, GrabbedFromCell.Y);
            if (Tex)
            {
                GrabbedRuneIcon->SetBrushFromTexture(Tex, false);
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

                if (bCursorInPendingArea
                    && PendingGridWidget && PendingGridWidget->PendingRuneGrid)
                {
                    // 手柄在待放置区抓起符文时：浮空图标跟随待放置区光标格
                    const FGeometry& PGeo  = PendingGridWidget->PendingRuneGrid->GetCachedGeometry();
                    const FVector2D  PSize = PGeo.GetLocalSize();
                    const int32 PCols = FMath::Max(1, PendingCols);
                    const int32 PRows = FMath::Max(1, PendingRows);
                    const float PCW   = PSize.X / PCols;
                    const float PCH   = PSize.Y / PRows;
                    const int32 CurCol = PendingCursorIdx % PCols;
                    const int32 CurRow = PendingCursorIdx / PCols;
                    const FVector2D SlotCenter((CurCol + 0.5f) * PCW, (CurRow + 0.5f) * PCH);
                    const FVector2D AbsPos   = PGeo.LocalToAbsolute(SlotCenter);
                    const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(AbsPos);
                    const float HalfSize = PCW * 0.35f;
                    FWidgetTransform T;
                    T.Translation = LocalPos + FVector2D(-HalfSize, -HalfSize + FloatY);
                    T.Scale       = FVector2D(0.7f, 0.7f);
                    GrabbedRuneIcon->SetRenderTransform(T);
                }
                else
                {
                    // 手柄在主格子时：浮空图标跟随主格子光标格
                    const FGeometry GridGeo  = BackpackGridWidget->GetGridGeometry();
                    const FVector2D GridSize = GridGeo.GetLocalSize();
                    if (GridSize.X > 0.f && GridSize.Y > 0.f)
                    {
                        const int32 TGW = 5;
                        const int32 TGH = 5;

                        const float CellW = GridSize.X / TGW;
                        const float CellH = GridSize.Y / TGH;
                        const FVector2D CellCenter(
                            (GamepadCursorCell.X + 0.5f) * CellW,
                            (GamepadCursorCell.Y + 0.5f) * CellH);

                        const FVector2D AbsPos   = GridGeo.LocalToAbsolute(CellCenter);
                        const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(AbsPos);

                        const float HalfSize = CellW * 0.35f;
                        FWidgetTransform T;
                        T.Translation = LocalPos + FVector2D(-HalfSize, -HalfSize + FloatY);
                        T.Scale       = FVector2D(0.7f, 0.7f);
                        GrabbedRuneIcon->SetRenderTransform(T);
                    }
                }
            }
            else
            {
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
            }
        }
        else
        {
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (!bDirKeyHeld) return;

    if (APlayerController* PC = GetOwningPlayer())
    {
        if (!PC->IsInputKeyDown(HeldDirKey))
        {
            bDirKeyHeld    = false;
            HeldKeyTime    = 0.f;
            LastRepeatCount = 0;
            return;
        }
    }

    HeldKeyTime += InDeltaTime;
    if (HeldKeyTime < DirRepeatInitial) return;

    const int32 TargetCount = FMath::FloorToInt((HeldKeyTime - DirRepeatInitial) / DirRepeatRate);
    if (TargetCount <= LastRepeatCount) return;

    LastRepeatCount = TargetCount;

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRepeat] HeldKey=%s TargetCount=%d"),
            *HeldDirKey.ToString(),
            TargetCount);
        if      (HeldDirKey == EKeys::Gamepad_DPad_Up || HeldDirKey == EKeys::Gamepad_LeftStick_Up)     CombatDeckEditWidget->HandleDeckDirectionalInput(-1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Down || HeldDirKey == EKeys::Gamepad_LeftStick_Down) CombatDeckEditWidget->HandleDeckDirectionalInput(1);
    }
    else if (bCursorInPendingArea)
    {
        if      (HeldDirKey == EKeys::Gamepad_DPad_Up || HeldDirKey == EKeys::Gamepad_LeftStick_Up)     MovePendingCursor( 0, -1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Down || HeldDirKey == EKeys::Gamepad_LeftStick_Down) MovePendingCursor( 0,  1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Left)  MovePendingCursor(-1,  0);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MovePendingCursor( 1,  0);
    }
    else
    {
        if      (HeldDirKey == EKeys::Gamepad_DPad_Up || HeldDirKey == EKeys::Gamepad_LeftStick_Up)     MoveGamepadCursor( 0, -1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Down || HeldDirKey == EKeys::Gamepad_LeftStick_Down) MoveGamepadCursor( 0,  1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Left)  MoveGamepadCursor(-1,  0);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MoveGamepadCursor( 1,  0);
    }
}

FReply UBackpackScreenWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput() &&
        !InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Bottom)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackPreviewKeyDown] A Press"));
        bIsGamepadInputMode = true;
        bDeckSelectFromVirtualMouse = false;
        return HandleCombatDeckSelectButtonState(true, TEXT("PreviewKeyDown"));
    }

    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UBackpackScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();
    UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackKeyDown] Key=%s Repeat=%d HasDeckWidget=%d CanDeck=%d"),
        *Key.ToString(),
        InKeyEvent.IsRepeat() ? 1 : 0,
        CombatDeckEditWidget ? 1 : 0,
        (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput()) ? 1 : 0);

    // 摇杆轴事件不触发输入模式切换，直接忽略
    if (Key == EKeys::Gamepad_RightStick_Up   || Key == EKeys::Gamepad_RightStick_Down  ||
        Key == EKeys::Gamepad_RightStick_Left  || Key == EKeys::Gamepad_RightStick_Right ||
        Key == EKeys::Gamepad_LeftStick_Left   || Key == EKeys::Gamepad_LeftStick_Right)
    {
        return FReply::Handled();
    }

    // 首次切换到手柄模式时立刻显示操作提示
    const bool bWasGamepad = bIsGamepadInputMode;
    bIsGamepadInputMode = true;
    if (!bWasGamepad && BackpackGridWidget)
        BackpackGridWidget->RefreshHeatPhaseButtons(PreviewPhase, true);

    if (Key == EKeys::Gamepad_Special_Left ||
        Key == EKeys::Gamepad_Special_Right ||
        Key == EKeys::Escape ||
        Key == EKeys::Tab)
    {
        DeactivateWidget();
        return FReply::Handled();
    }

    if (CombatDeckEditWidget && CombatDeckEditWidget->IsVisible() && !InKeyEvent.IsRepeat() &&
        (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top))
    {
        const bool bVisible = CombatDeckEditWidget->ToggleDetailPreview();
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] DetailPreview Key=%s Visible=%d"),
            *Key.ToString(),
            bVisible ? 1 : 0);
        return FReply::Handled();
    }

    if (CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput())
    {
        auto StartDeckDirRepeat = [&](int32 Direction) -> FReply
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] DPad Direction=%d"), Direction);
            CombatDeckEditWidget->HandleDeckDirectionalInput(Direction);
            HeldDirKey      = Key;
            bDirKeyHeld     = true;
            HeldKeyTime     = 0.f;
            LastRepeatCount = 0;
            return FReply::Handled();
        };

        if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
        {
            if (InKeyEvent.IsRepeat())
            {
                UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] IgnoreNativeRepeat Key=%s"), *Key.ToString());
                return FReply::Handled();
            }
            return StartDeckDirRepeat(-1);
        }
        if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
        {
            if (InKeyEvent.IsRepeat())
            {
                UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] IgnoreNativeRepeat Key=%s"), *Key.ToString());
                return FReply::Handled();
            }
            return StartDeckDirRepeat(1);
        }
        if (!InKeyEvent.IsRepeat() && Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] A Press"));
            return HandleCombatDeckSelectButtonState(true, TEXT("KeyDown"));
        }
        if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Left)
        {
            UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] Reverse Key=%s"), *Key.ToString());
            return CombatDeckEditWidget->ToggleSelectedLinkOrientation() ? FReply::Handled() : FReply::Unhandled();
        }
    }
    else if (CombatDeckEditWidget)
    {
        UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackRoute] Deck widget present but CanHandleDeckInput=false"));
    }

    // ── 热度阶段预览切换 ────────────────────────────────────────────────
    {
        const int32 MaxPhase = 2;

        auto TogglePreview = [&](int32 Phase) -> FReply
        {
            PreviewPhase = (PreviewPhase == Phase) ? -1 : Phase;
            OnGridNeedsRefresh();
            return FReply::Handled();
        };

        if (Key == EKeys::One)   return TogglePreview(0);
        if (Key == EKeys::Two)   return TogglePreview(1);
        if (Key == EKeys::Three) return TogglePreview(2);

        if (Key == EKeys::Gamepad_LeftShoulder)
        {
            PreviewPhase = (PreviewPhase < 0) ? MaxPhase : PreviewPhase - 1;
            OnGridNeedsRefresh();
            return FReply::Handled();
        }
        if (Key == EKeys::Gamepad_RightShoulder)
        {
            PreviewPhase = (PreviewPhase >= MaxPhase) ? -1 : PreviewPhase + 1;
            OnGridNeedsRefresh();
            return FReply::Handled();
        }
    }

    auto StartDirRepeat = [&](int32 DC, int32 DR) -> FReply
    {
        if (bCursorInPendingArea) MovePendingCursor(DC, DR);
        else                      MoveGamepadCursor(DC, DR);
        HeldDirKey      = Key;
        bDirKeyHeld     = true;
        HeldKeyTime     = 0.f;
        LastRepeatCount = 0;
        return FReply::Handled();
    };

    if (Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
    {
        return InKeyEvent.IsRepeat() ? FReply::Handled() : StartDirRepeat(0, -1);
    }
    if (Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
    {
        return InKeyEvent.IsRepeat() ? FReply::Handled() : StartDirRepeat(0, 1);
    }
    if (Key == EKeys::Gamepad_DPad_Left)  return StartDirRepeat(-1,  0);
    if (Key == EKeys::Gamepad_DPad_Right) return StartDirRepeat( 1,  0);

    if (!InKeyEvent.IsRepeat())
    {
        if (Key == EKeys::Gamepad_FaceButton_Bottom)
        {
            if (bCursorInPendingArea) PendingGamepadConfirm();
            else                      GamepadConfirm();
            return FReply::Handled();
        }
        if (Key == EKeys::Gamepad_FaceButton_Right)
        {
            if (bCursorInPendingArea) PendingGamepadCancel();
            else                      GamepadCancel();
            return FReply::Handled();
        }
    }

    // ── 符文旋转 ────────────────────────────────────────────────────────
    if (Key == EKeys::R)
    {
        if (bCursorInPendingArea || PendingSelectedIdx >= 0)
            RotatePendingRune();
        else
            RotateSelectedRune();
        return FReply::Handled();
    }
    if (Key == EKeys::Gamepad_FaceButton_Left) // X 键
    {
        if (bCursorInPendingArea)
            RotatePendingRune();
        else
            RotateSelectedRune();
        return FReply::Handled();
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UBackpackScreenWidget::HandleCombatDeckSelectButtonState(bool bPressed, const TCHAR* Source)
{
    const bool bCanDeck = CombatDeckEditWidget && CombatDeckEditWidget->CanHandleDeckInput();
    UE_LOG(LogTemp, Warning, TEXT("[CombatDeckInput][BackpackAState] Source=%s Pressed=%d WasDown=%d CanDeck=%d"),
        Source,
        bPressed ? 1 : 0,
        bDeckSelectButtonWasDown ? 1 : 0,
        bCanDeck ? 1 : 0);

    if (!bCanDeck)
    {
        bDeckSelectButtonWasDown = bPressed;
        return FReply::Unhandled();
    }

    if (bPressed)
    {
        if (bDeckSelectButtonWasDown)
        {
            return FReply::Handled();
        }

        bDeckSelectButtonWasDown = true;
        return CombatDeckEditWidget->HandleDeckSelectPressed() ? FReply::Handled() : FReply::Unhandled();
    }

    if (!bDeckSelectButtonWasDown)
    {
        return FReply::Handled();
    }

    bDeckSelectButtonWasDown = false;
    return CombatDeckEditWidget->HandleDeckSelectReleased() ? FReply::Handled() : FReply::Unhandled();
}

void UBackpackScreenWidget::PollCombatDeckSelectButtonState()
{
    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    if (bDeckSelectFromVirtualMouse)
    {
        return;
    }

    const bool bPressed = PC->IsInputKeyDown(EKeys::Gamepad_FaceButton_Bottom);
    if (bPressed == bDeckSelectButtonWasDown)
    {
        return;
    }

    HandleCombatDeckSelectButtonState(bPressed, TEXT("TickPoll"));
}

void UBackpackScreenWidget::MoveGamepadCursor(int32 DCol, int32 DRow)
{
    const int32 W = 5;
    const int32 H = 5;

    // DPad Left 从第 0 列出边界 → 进入待放置区
    if (DCol == -1 && GamepadCursorCell.X == 0)
    {
        bCursorInPendingArea = true;
        const int32 PCols = FMath::Max(1, PendingCols);
        const int32 PRows = FMath::Max(1, PendingRows);
        const int32 Row   = FMath::Clamp(GamepadCursorCell.Y, 0, PRows - 1);
        PendingCursorIdx  = Row * PCols + (PCols - 1);

        if (!bGrabbingRune && !bGrabbingFromPending)
        {
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
        }
        RefreshPendingGrid();
        OnGridNeedsRefresh();
        return;
    }

    GamepadCursorCell.X = FMath::Clamp(GamepadCursorCell.X + DCol, 0, W - 1);
    GamepadCursorCell.Y = FMath::Clamp(GamepadCursorCell.Y + DRow, 0, H - 1);

    if (!bGrabbingRune)
    {
        // 仅悬浮高亮（绿框），不触发选中/信息卡
        HoverCol = GamepadCursorCell.X;
        HoverRow = GamepadCursorCell.Y;
        OnGridNeedsRefresh();
    }
    else
    {
        OnGridNeedsRefresh();
    }

    UpdateTooltipForCell(GamepadCursorCell.X, GamepadCursorCell.Y, FVector2D::ZeroVector);
}

void UBackpackScreenWidget::GamepadConfirm()
{
    if (bIsPreviewMode) return;

    if (bGrabbingFromPending || bGrabbingRune)
    {
        GamepadCancel();
        return;
    }

    OnStatusMessage(NSLOCTEXT("Backpack", "GrabEmpty", "该格子没有符文"));
}

void UBackpackScreenWidget::GamepadCancel()
{
    if (bGrabbingFromPending)
    {
        bGrabbingFromPending = false;
        PendingGrabbedIdx    = -1;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "已取消"));
        return;
    }

    if (bGrabbingRune)
    {
        bGrabbingRune   = false;
        GrabbedFromCell = FIntPoint(-1, -1);
        SelectedCell    = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "已取消"));
        return;
    }

    // 未抓取时 B 键直接关闭背包
    DeactivateWidget();
}

// ── 待放置区手柄方法 ──────────────────────────────────────────────────────

void UBackpackScreenWidget::MovePendingCursor(int32 DCol, int32 DRow)
{
    const int32 PCols = FMath::Max(1, PendingCols);
    const int32 PRows = FMath::Max(1, PendingRows);

    int32 Col = PendingCursorIdx % PCols + DCol;
    int32 Row = PendingCursorIdx / PCols + DRow;

    // 向右出边界 → 切换到主格子
    if (DCol == 1 && Col >= PCols)
    {
        bCursorInPendingArea = false;
        const int32 GH = 5;
        GamepadCursorCell = FIntPoint(0, FMath::Clamp(Row, 0, GH - 1));

        if (!bGrabbingRune && !bGrabbingFromPending)
        {
            SelectedCell = GamepadCursorCell;
            OnSelectionChanged();
        }
        else
        {
            OnGridNeedsRefresh();
        }
        RefreshPendingGrid();
        return;
    }

    Col = FMath::Clamp(Col, 0, PCols - 1);
    Row = FMath::Clamp(Row, 0, PRows - 1);
    PendingCursorIdx = Row * PCols + Col;
    RefreshPendingGrid();
}

void UBackpackScreenWidget::PendingGamepadConfirm()
{
    if (bIsPreviewMode) return;  // 只读预览模式：禁止操作

    if (bGrabbingRune)
    {
        GamepadCancel();
        return;
    }

    const bool bHasRune = PendingGrid.IsValidIndex(PendingCursorIdx)
        && PendingGrid[PendingCursorIdx].RuneGuid.IsValid();

    if (!bGrabbingFromPending)
    {
        if (!bHasRune) { OnStatusMessage(NSLOCTEXT("Backpack", "PendingGrabEmpty", "该格子没有符文")); return; }
        if (IsInCombatPhase()) { OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文")); return; }

        bGrabbingFromPending = true;
        PendingGrabbedIdx    = PendingCursorIdx;
        PendingSelectedIdx   = PendingCursorIdx;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "PendingGrabOK", "已抓取符文，DPad→移动，A→放置，B→取消"));
    }
    else
    {
        if (PendingCursorIdx == PendingGrabbedIdx) { PendingGamepadCancel(); return; }

        // 在待放置区内交换
        const FRuneInstance Src = PendingGrid[PendingGrabbedIdx];
        const FRuneInstance Dst = PendingGrid.IsValidIndex(PendingCursorIdx)
            ? PendingGrid[PendingCursorIdx] : FRuneInstance();
        PendingGrid[PendingGrabbedIdx] = Dst;
        if (PendingGrid.IsValidIndex(PendingCursorIdx)) PendingGrid[PendingCursorIdx] = Src;

        SyncPendingToPlayer();
        bGrabbingFromPending = false;
        PendingGrabbedIdx    = -1;
        PendingSelectedIdx   = PendingCursorIdx;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "PendingMoved", "已移动符文"));
    }
}

void UBackpackScreenWidget::PendingGamepadCancel()
{
    if (bGrabbingFromPending)
    {
        bGrabbingFromPending = false;
        PendingGrabbedIdx    = -1;
        RefreshPendingGrid();
        OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "已取消"));
        return;
    }
    // 未抓取时 B 键退出待放置区，回到主格子最左列
    bCursorInPendingArea = false;
    PendingSelectedIdx   = -1;
    const int32 GH = 5;
    const int32 Row = FMath::Clamp(PendingCursorIdx / FMath::Max(1, PendingCols), 0, GH - 1);
    GamepadCursorCell = FIntPoint(0, Row);
    SelectedCell = GamepadCursorCell;
    OnSelectionChanged();
    RefreshPendingGrid();
}

// ============================================================
//  符文旋转
// ============================================================

void UBackpackScreenWidget::RotateSelectedRune()
{
}

void UBackpackScreenWidget::RotatePendingRune()
{
    if (bIsPreviewMode) return;  // 只读预览模式：禁止旋转 pending 符文
    const int32 Idx = bCursorInPendingArea ? PendingCursorIdx : PendingSelectedIdx;
    if (!PendingGrid.IsValidIndex(Idx)) return;
    if (PendingGrid[Idx].RuneGuid == FGuid()) return; // 空格

    PendingGrid[Idx].Rotation = (PendingGrid[Idx].Rotation + 1) % 4;
    SyncPendingToPlayer();
    RefreshPendingGrid();
}

void UBackpackScreenWidget::UpdateTooltipForCell(int32 Col, int32 Row, const FVector2D& LocalPos)
{
    if (!RuneTooltip) return;
    RuneTooltip->HideTooltip();
}
