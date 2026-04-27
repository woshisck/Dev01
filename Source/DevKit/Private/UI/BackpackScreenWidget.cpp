#include "UI/BackpackScreenWidget.h"
#include "UI/BackpackGridWidget.h"
#include "UI/PendingGridWidget.h"
#include "UI/RuneDragDropOperation.h"
#include "UI/RuneTooltipWidget.h"
#include "UI/RuneInfoCardWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Component/BackpackGridComponent.h"
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
//  内部辅助
// ============================================================

UBackpackGridComponent* UBackpackScreenWidget::GetBackpack() const
{
    if (CachedBackpack.IsValid())
        return CachedBackpack.Get();

    APawn* Pawn = GetOwningPlayerPawn();
    if (!Pawn) return nullptr;

    return Pawn->FindComponentByClass<UBackpackGridComponent>();
}

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

    if (APawn* Pawn = GetOwningPlayerPawn())
        CachedBackpack = Pawn->FindComponentByClass<UBackpackGridComponent>();

    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.AddDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.AddDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.AddDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }

    // 子 Widget 初始化（NativeConstruct 时子 Widget 已构建完毕）
    if (BackpackGridWidget)
        BackpackGridWidget->BuildGrid(GetBackpack());

    if (PendingGridWidget)
    {
        PendingGridWidget->BuildSlots();
        PendingCols = PendingGridWidget->PendingGridCols;
        PendingRows = PendingGridWidget->PendingGridRows;
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
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        Backpack->OnRunePlaced.RemoveDynamic(this, &UBackpackScreenWidget::HandleRunePlaced);
        Backpack->OnRuneRemoved.RemoveDynamic(this, &UBackpackScreenWidget::HandleRuneRemoved);
        Backpack->OnRuneActivationChanged.RemoveDynamic(this, &UBackpackScreenWidget::HandleRuneActivationChanged);
    }
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
    if (SelectedCell != FIntPoint(-1, -1))
    {
        if (UBackpackGridComponent* Backpack = GetBackpack())
        {
            if (Backpack->GetRuneIndexAtCell(SelectedCell) == -1)
            {
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
            }
        }
    }
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
        BackpackGridWidget->RefreshCells(
            GetBackpack(),
            SelectedCell,
            FIntPoint(HoverCol, HoverRow),
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
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;
    return Backpack->GetActivationZoneCells().Contains(FIntPoint(Col, Row));
}

bool UBackpackScreenWidget::IsCellOccupied(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return false;
    return Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row)) >= 0;
}

FPlacedRune UBackpackScreenWidget::GetRuneAtCell(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return FPlacedRune();

    int32 Idx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    if (Idx < 0) return FPlacedRune();

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    return Placed.IsValidIndex(Idx) ? Placed[Idx] : FPlacedRune();
}

FRuneInstance UBackpackScreenWidget::GetFocusedRuneInfo() const
{
    if (SelectedCell != FIntPoint(-1, -1))
    {
        FPlacedRune PR = GetRuneAtCell(SelectedCell.X, SelectedCell.Y);
        if (PR.Rune.RuneGuid.IsValid())
            return PR.Rune;
    }
    return GetSelectedRuneInfo();
}

UTexture2D* UBackpackScreenWidget::GetRuneIconAtCell(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return nullptr;

    int32 Idx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    if (Idx < 0) return nullptr;

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(Idx)) return nullptr;

    return Placed[Idx].Rune.RuneConfig.RuneIcon;
}

EBackpackCellState UBackpackScreenWidget::GetCellVisualState(int32 Col, int32 Row) const
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return EBackpackCellState::Empty;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(FIntPoint(Col, Row));
    bool bInZone  = IsCellInActivationZone(Col, Row);

    if (RuneIdx < 0)
        return bInZone ? EBackpackCellState::EmptyActive : EBackpackCellState::Empty;

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (Placed.IsValidIndex(RuneIdx) && Placed[RuneIdx].bIsActivated)
        return EBackpackCellState::OccupiedActive;

    return EBackpackCellState::OccupiedInactive;
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

const TArray<FPlacedRune>& UBackpackScreenWidget::GetAllPlacedRunes() const
{
    static TArray<FPlacedRune> Empty;
    UBackpackGridComponent* Backpack = GetBackpack();
    return Backpack ? Backpack->GetAllPlacedRunes() : Empty;
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
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    FIntPoint Cell(Col, Row);
    int32 RuneIdx = Backpack->GetRuneIndexAtCell(Cell);

    if (RuneIdx >= 0)
    {
        // 预览模式 / 战斗阶段：与战斗一致 — shake + 闪红光，不允许选中
        if (IsInCombatPhase() || bIsPreviewMode)
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
            OnStatusMessage(bIsPreviewMode
                ? NSLOCTEXT("Backpack", "PreviewLock", "预览模式：无法操作符文")
                : NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文"));
            return;
        }
        SelectedCell = Cell;
        SelectedRuneIndex = -1;
        // 焦点彻底切到主背包：清 pending 黄框 + PendingSelectedIdx，避免 R 键仍旋转 pending
        ClearPendingFocus(true);
        OnSelectionChanged();
    }
    else if (bIsPreviewMode)
    {
        // 预览模式：空格点击是"放置"操作（从 pending 放置 / 从列表放置），禁止
        return;
    }
    else if (PendingSelectedIdx >= 0
        && PendingGrid.IsValidIndex(PendingSelectedIdx)
        && PendingGrid[PendingSelectedIdx].RuneGuid.IsValid())
    {
        // 点击主格子空格 → 从待放置区放置选中的符文
        const FRuneInstance Instance = PendingGrid[PendingSelectedIdx];
        if (Backpack->TryPlaceRune(Instance, Cell))
        {
            PendingGrid[PendingSelectedIdx] = FRuneInstance();
            SyncPendingToPlayer();
            PendingSelectedIdx   = -1;
            bCursorInPendingArea = false;
            SelectedCell         = FIntPoint(-1, -1);
            RefreshPendingGrid();
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "PlaceOK", "已放置：{0}"),
                FText::FromName(Instance.RuneConfig.RuneName)));
        }
        else
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
            OnStatusMessage(NSLOCTEXT("Backpack", "PlaceFail", "无法放置：位置被占用"));
        }
    }
    else if (SelectedRuneIndex >= 0)
    {
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        const int32 PendingCount = Player ? Player->PendingRunes.Num() : 0;
        const bool bFromPending  = SelectedRuneIndex < PendingCount;

        FRuneInstance Instance;
        if (bFromPending)
        {
            Instance = Player->PendingRunes[SelectedRuneIndex];
        }
        else
        {
            const int32 AvIdx = SelectedRuneIndex - PendingCount;
            if (!AvailableRunes.IsValidIndex(AvIdx) || !AvailableRunes[AvIdx]) return;
            Instance = AvailableRunes[AvIdx]->CreateInstance();
        }

        if (Backpack->TryPlaceRune(Instance, Cell))
        {
            if (bFromPending && Player)
            {
                Player->PendingRunes.RemoveAt(SelectedRuneIndex);
                OnRuneListChanged();
            }
            SelectedRuneIndex = -1;
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","PlaceOK","已放置：{0}"), FText::FromName(Instance.RuneConfig.RuneName)));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack","PlaceFail","无法放置：位置被占用"));
        }
    }
}

void UBackpackScreenWidget::RemoveRuneAtSelectedCell()
{
    if (bIsPreviewMode) return;  // 只读预览模式：禁止删除符文
    if (SelectedCell == FIntPoint(-1, -1)) return;

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    int32 RuneIdx = Backpack->GetRuneIndexAtCell(SelectedCell);
    if (RuneIdx < 0) { OnStatusMessage(NSLOCTEXT("Backpack","RemoveEmpty","该格子没有符文")); return; }

    const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
    if (!Placed.IsValidIndex(RuneIdx)) return;

    FGuid RuneGuid = Placed[RuneIdx].Rune.RuneGuid;
    FName RuneName = Placed[RuneIdx].Rune.RuneConfig.RuneName;

    if (Backpack->RemoveRune(RuneGuid))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(NSLOCTEXT("Backpack","RemoveOK","已移除：{0}"), FText::FromName(RuneName)));
    }
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
            HUD->BeginPauseEffect();

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetPause(true);
        PC->SetShowMouseCursor(true);
    }

    SetUserFocus(GetOwningPlayer());

    // 每次打开时重建格子，确保武器切换后的 GridWidth/GridHeight 生效
    if (BackpackGridWidget)
    {
        UBackpackGridComponent* BG = GetBackpack();
        UE_LOG(LogTemp, Warning, TEXT("[BackpackScreenWidget] NativeOnActivated BuildGrid: Backpack=%s, W=%d H=%d"),
            BG ? *BG->GetName() : TEXT("null"),
            BG ? BG->GridWidth  : -1,
            BG ? BG->GridHeight : -1);
        BackpackGridWidget->BuildGrid(BG);
    }

    SyncPendingFromPlayer();
    RefreshPendingGrid();

    if (UBackpackGridComponent* BG = GetBackpack())
        PreviewPhase = FMath::Clamp(BG->GetCurrentPhase(), 0, 2);

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
            HUD->EndPauseEffect();

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
    if (bIsPreviewMode) return;  // 只读预览模式：禁止出售
    if (SelectedCell == FIntPoint(-1, -1)) return;
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        FPlacedRune PR = GetRuneAtCell(SelectedCell.X, SelectedCell.Y);
        if (PR.Rune.RuneGuid.IsValid())
            Backpack->SellRune(PR.Rune.RuneGuid);
    }
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
    return BackpackGridWidget->GetCellAtScreenPos(AbsolutePos, GetBackpack(), OutCol, OutRow);
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

    if (bIsGamepadInputMode)
    {
        GamepadConfirm();
        return FReply::Handled();
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

    // ── 已抓取状态下点击另一格：移动或互换 ──────────────────────────────────
    if (bGrabbingRune && ClickCell != GrabbedFromCell)
    {
        if (IsInCombatPhase())
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GrabbedFromCell.X, GrabbedFromCell.Y);
            OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文"));
            return FReply::Handled();
        }

        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack) return FReply::Handled();

        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        const int32 SrcIdx = Backpack->GetRuneIndexAtCell(GrabbedFromCell);
        if (SrcIdx < 0)
        {
            bGrabbingRune   = false;
            GrabbedFromCell = FIntPoint(-1,-1);
            SelectedCell    = FIntPoint(-1,-1);
            OnSelectionChanged();
            return FReply::Handled();
        }

        const FRuneInstance RuneA  = Placed[SrcIdx].Rune;
        const FIntPoint     PivotA = Placed[SrcIdx].Pivot;
        const int32 DstIdx = Backpack->GetRuneIndexAtCell(ClickCell);

        if (DstIdx >= 0)
        {
            // 互换 → 自动抓取被替换的符文
            const FRuneInstance RuneB  = Placed[DstIdx].Rune;
            const FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneA.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);
            Backpack->TryPlaceRune(RuneA, PivotB);
            Backpack->TryPlaceRune(RuneB, PivotA);

            GrabbedFromCell = PivotA;   // RuneB 现在在 PivotA，自动抓取
            SelectedCell    = PivotA;
            // bGrabbingRune 保持 true
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "SwapOK", "已互换：{0} ↔ {1}"),
                FText::FromName(RuneA.RuneConfig.RuneName),
                FText::FromName(RuneB.RuneConfig.RuneName)));
        }
        else
        {
            // 移动到空格 → 结束抓取
            const FIntPoint Offset   = GrabbedFromCell - PivotA;
            const FIntPoint NewPivot = ClickCell - Offset;

            if (Backpack->MoveRune(RuneA.RuneGuid, NewPivot))
            {
                bGrabbingRune   = false;
                GrabbedFromCell = FIntPoint(-1,-1);
                SelectedCell    = FIntPoint(-1,-1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
                    FText::FromName(RuneA.RuneConfig.RuneName)));
            }
            else
            {
                if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(Col, Row);
                OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法放置：目标位置被占用"));
            }
        }
        return FReply::Handled();
    }

    // ── 点击已抓取符文自身 → 取消抓取 ───────────────────────────────────────
    if (bGrabbingRune && ClickCell == GrabbedFromCell)
    {
        bGrabbingRune     = false;
        GrabbedFromCell   = FIntPoint(-1,-1);
        SelectedCell      = FIntPoint(-1,-1);
        OnSelectionChanged();
        return FReply::Handled();
    }

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
                const int32 GW = (CachedBackpack.IsValid() && CachedBackpack->GridWidth > 0) ? CachedBackpack->GridWidth : 5;
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

    if (PendingDragCol < 0 || PendingDragRow < 0)
        return;

    FPlacedRune PR = GetRuneAtCell(PendingDragCol, PendingDragRow);
    if (!PR.Rune.RuneGuid.IsValid())
    {
        PendingDragCol = PendingDragRow = -1;
        return;
    }

    if (IsInCombatPhase())
    {
        PendingDragCol = PendingDragRow = -1;
        return;
    }

    URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
    DragOp->SrcCol      = PendingDragCol;
    DragOp->SrcRow      = PendingDragRow;
    DragOp->SrcPivot    = PR.Pivot;
    DragOp->DraggedRune = PR.Rune;

    bMouseDragging  = true;
    MouseDragTex    = PR.Rune.RuneConfig.RuneIcon;
    LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

    // Phase 2: 主格子拖拽也显完整 Shape，AnchorCell = Pivot 在旋转后 Shape 的位置
    // （与 pending 路径一致 — 用户决策 Q5：Pivot 原点对齐鼠标）
    if (ShapePreviewCanvas && BackpackGridWidget)
    {
        const FIntPoint AnchorCell = PR.Rune.Shape.GetPivotOffset(PR.Rune.Rotation);
        const FVector2D GridSize   = BackpackGridWidget->GetGridGeometry().GetLocalSize();
        const int32 GW = (CachedBackpack.IsValid() && CachedBackpack->GridWidth > 0) ? CachedBackpack->GridWidth : 5;
        const float CellPx = (GridSize.X > 0.f) ? (GridSize.X / GW) : 64.f;
        ShowShapePreview(PR.Rune, AnchorCell, MouseDragTex, CellPx);
        // 首帧立即对位，避免出现在 (0,0) 一帧
        UpdateShapePreviewPosition(InGeometry, InMouseEvent.GetScreenSpacePosition());
    }

    PendingDragCol = PendingDragRow = -1;
    OutOperation = DragOp;
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

        if (bShouldUnplace)
        {
            UBackpackGridComponent* Backpack = GetBackpack();
            if (Backpack)
            {
                // 找目标格子：先尝试最近格，若已占用则找第一个空格
                int32 TargetSlot = -1;
                if (PendingGridWidget)
                {
                    int32 NearestIdx;
                    if (PendingGridWidget->GetNearestSlotAtScreenPos(
                            InDragDropEvent.GetScreenSpacePosition(), NearestIdx)
                        && PendingGrid.IsValidIndex(NearestIdx)
                        && !PendingGrid[NearestIdx].RuneGuid.IsValid())
                    {
                        TargetSlot = NearestIdx;
                    }
                }
                if (TargetSlot < 0)
                {
                    for (int32 i = 0; i < PendingGrid.Num(); i++)
                        if (!PendingGrid[i].RuneGuid.IsValid()) { TargetSlot = i; break; }
                }
                if (TargetSlot < 0)
                {
                    OnStatusMessage(NSLOCTEXT("Backpack", "PendingFull", "待放置区已满"));
                    OnGridNeedsRefresh();
                    return false;
                }

                Backpack->RemoveRune(RuneOp->DraggedRune.RuneGuid);
                PendingGrid[TargetSlot] = RuneOp->DraggedRune;
                SyncPendingToPlayer();
                PendingSelectedIdx = TargetSlot;
                RefreshPendingGrid();
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "UnplaceOK", "已取回：{0}"),
                    FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName)));
                return true;
            }
        }
    }

    if (RuneOp->PendingSourceIndex >= 0)
    {
        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack || !PendingGrid.IsValidIndex(RuneOp->PendingSourceIndex))
        {
            OnGridNeedsRefresh();
            return false;
        }

        // ── 优先检测主格子落点（防止边界模糊误触 pending→pending 路径） ────
        int32 TargetCol, TargetRow;
        if (GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
        {
            const FRuneInstance PendingRune = PendingGrid[RuneOp->PendingSourceIndex];
            if (!PendingRune.RuneGuid.IsValid()) { OnGridNeedsRefresh(); return false; }

            if (Backpack->TryPlaceRune(PendingRune, FIntPoint(TargetCol, TargetRow)))
            {
                PendingGrid[RuneOp->PendingSourceIndex] = FRuneInstance();
                SyncPendingToPlayer();
                PendingSelectedIdx = -1;
                RefreshPendingGrid();
                OnRuneListChanged();
                SelectedCell = FIntPoint(-1, -1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "PendingPlaceOK", "已放置：{0}"),
                    FText::FromName(PendingRune.RuneConfig.RuneName)));
                return true;
            }

            OnStatusMessage(NSLOCTEXT("Backpack", "PendingPlaceFail", "无法放置：位置被占用"));
            OnGridNeedsRefresh();
            return false;
        }

        // ── 主格子未命中：检查是否落在待放置区内（pending → pending 交换）──
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

    int32 TargetCol, TargetRow;
    if (!GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
    {
        OnGridNeedsRefresh();
        return false;
    }

    if (TargetCol == RuneOp->SrcCol && TargetRow == RuneOp->SrcRow)
    {
        OnGridNeedsRefresh();
        return false;
    }

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack)
    {
        OnGridNeedsRefresh();
        return false;
    }

    const FIntPoint GrabOffset = FIntPoint(RuneOp->SrcCol, RuneOp->SrcRow) - RuneOp->SrcPivot;
    const FIntPoint NewPivot   = FIntPoint(TargetCol, TargetRow) - GrabOffset;

    if (Backpack->MoveRune(RuneOp->DraggedRune.RuneGuid, NewPivot))
    {
        SelectedCell = FIntPoint(-1, -1);
        OnSelectionChanged();
        OnStatusMessage(FText::Format(
            NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
            FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName)));
        return true;
    }

    const int32 DstIdx = Backpack->GetRuneIndexAtCell(FIntPoint(TargetCol, TargetRow));
    if (DstIdx >= 0)
    {
        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        if (Placed.IsValidIndex(DstIdx))
        {
            FRuneInstance RuneB  = Placed[DstIdx].Rune;
            FIntPoint     PivotA = RuneOp->SrcPivot;
            FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneOp->DraggedRune.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);

            const bool bPlaceA = Backpack->TryPlaceRune(RuneOp->DraggedRune, PivotB);
            const bool bPlaceB = Backpack->TryPlaceRune(RuneB, PivotA);

            if (bPlaceA && bPlaceB)
            {
                // 互换成功 → 自动抓取被替换的符文（RuneB 现在在 PivotA）
                bGrabbingRune   = true;
                GrabbedFromCell = PivotA;
                SelectedCell    = PivotA;
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "SwapOK", "已互换：{0} ↔ {1}"),
                    FText::FromName(RuneOp->DraggedRune.RuneConfig.RuneName),
                    FText::FromName(RuneB.RuneConfig.RuneName)));
                return true;
            }

            Backpack->TryPlaceRune(RuneOp->DraggedRune, PivotA);
            Backpack->TryPlaceRune(RuneB, PivotB);
            OnStatusMessage(NSLOCTEXT("Backpack", "SwapFail", "无法互换：形状冲突"));
            OnGridNeedsRefresh();
            return false;
        }
    }

    OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法移动：目标位置被占用"));
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
    if (bIsGamepadInputMode)
    {
        bIsGamepadInputMode = false;
        if (BackpackGridWidget)
            BackpackGridWidget->RefreshHeatPhaseButtons(PreviewPhase, false);
    }
    LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

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
                        UBackpackGridComponent* BGComp = GetBackpack();
                        const int32 TGW = BGComp ? BGComp->GridWidth  : 5;
                        const int32 TGH = BGComp ? BGComp->GridHeight : 5;

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

    if (bCursorInPendingArea)
    {
        if      (HeldDirKey == EKeys::Gamepad_DPad_Up)    MovePendingCursor( 0, -1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Down)  MovePendingCursor( 0,  1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Left)  MovePendingCursor(-1,  0);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MovePendingCursor( 1,  0);
    }
    else
    {
        if      (HeldDirKey == EKeys::Gamepad_DPad_Up)    MoveGamepadCursor( 0, -1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Down)  MoveGamepadCursor( 0,  1);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Left)  MoveGamepadCursor(-1,  0);
        else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MoveGamepadCursor( 1,  0);
    }
}

FReply UBackpackScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    // 摇杆轴事件不触发输入模式切换，直接忽略
    if (Key == EKeys::Gamepad_RightStick_Up   || Key == EKeys::Gamepad_RightStick_Down  ||
        Key == EKeys::Gamepad_RightStick_Left  || Key == EKeys::Gamepad_RightStick_Right ||
        Key == EKeys::Gamepad_LeftStick_Up     || Key == EKeys::Gamepad_LeftStick_Down   ||
        Key == EKeys::Gamepad_LeftStick_Left   || Key == EKeys::Gamepad_LeftStick_Right)
    {
        return FReply::Handled();
    }

    // 首次切换到手柄模式时立刻显示操作提示
    const bool bWasGamepad = bIsGamepadInputMode;
    bIsGamepadInputMode = true;
    if (!bWasGamepad && BackpackGridWidget)
        BackpackGridWidget->RefreshHeatPhaseButtons(PreviewPhase, true);

    if (Key == EKeys::Gamepad_Special_Left || Key == EKeys::Tab)
    {
        DeactivateWidget();
        return FReply::Handled();
    }

    // ── 热度阶段预览切换 ────────────────────────────────────────────────
    {
        const int32 MaxPhase = GetBackpack()
            ? GetBackpack()->ActivationZoneConfig.ZoneShapes.Num() - 1
            : 2;

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

    if (Key == EKeys::Gamepad_DPad_Up)    return StartDirRepeat( 0, -1);
    if (Key == EKeys::Gamepad_DPad_Down)  return StartDirRepeat( 0,  1);
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
        if (Key == EKeys::Gamepad_FaceButton_Top && !bCursorInPendingArea)
        {
            SelectedCell = GamepadCursorCell;
            RemoveRuneAtSelectedCell();
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

void UBackpackScreenWidget::MoveGamepadCursor(int32 DCol, int32 DRow)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 W = Backpack ? Backpack->GridWidth  : 5;
    const int32 H = Backpack ? Backpack->GridHeight : 5;

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
    if (bIsPreviewMode) return;  // 只读预览模式：禁止抓取/放置

    // 从待放置区抓起后，在主格子落点
    if (bGrabbingFromPending)
    {
        if (!PendingGrid.IsValidIndex(PendingGrabbedIdx) || !PendingGrid[PendingGrabbedIdx].RuneGuid.IsValid())
        {
            bGrabbingFromPending = false; PendingGrabbedIdx = -1; return;
        }
        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack) return;

        const FRuneInstance PendingRune = PendingGrid[PendingGrabbedIdx];
        if (Backpack->TryPlaceRune(PendingRune, GamepadCursorCell))
        {
            PendingGrid[PendingGrabbedIdx] = FRuneInstance();
            SyncPendingToPlayer();
            bGrabbingFromPending = false;
            PendingGrabbedIdx    = -1;
            PendingSelectedIdx   = -1;
            RefreshPendingGrid();
            SelectedCell = FIntPoint(-1, -1);
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "PendingPlaceOK", "已放置：{0}"),
                FText::FromName(PendingRune.RuneConfig.RuneName)));
        }
        else
        {
            if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GamepadCursorCell.X, GamepadCursorCell.Y);
            OnStatusMessage(NSLOCTEXT("Backpack", "PendingPlaceFail", "无法放置：位置被占用"));
        }
        return;
    }

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    if (!bGrabbingRune)
    {
        int32 RuneIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);
        if (RuneIdx >= 0)
        {
            if (IsInCombatPhase())
            {
                if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GamepadCursorCell.X, GamepadCursorCell.Y);
                OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文"));
                return;
            }

            bGrabbingRune    = true;
            GrabbedFromCell  = GamepadCursorCell;
            SelectedCell     = GamepadCursorCell;
            HoverCol = HoverRow = -1;
            OnSelectionChanged();
            OnStatusMessage(NSLOCTEXT("Backpack", "GrabOK", "已抓取符文，移动光标后按 A 放置"));
        }
        else
        {
            OnStatusMessage(NSLOCTEXT("Backpack", "GrabEmpty", "该格子没有符文"));
        }
    }
    else
    {
        if (GamepadCursorCell == GrabbedFromCell)
        {
            GamepadCancel();
            return;
        }

        const TArray<FPlacedRune>& Placed = Backpack->GetAllPlacedRunes();
        int32 SrcIdx = Backpack->GetRuneIndexAtCell(GrabbedFromCell);
        int32 DstIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);

        if (SrcIdx < 0)
        {
            bGrabbingRune = false;
            GrabbedFromCell = FIntPoint(-1, -1);
            return;
        }

        const FRuneInstance RuneA  = Placed[SrcIdx].Rune;
        const FIntPoint     PivotA = Placed[SrcIdx].Pivot;

        if (DstIdx >= 0)
        {
            // 互换 → 自动抓取被替换符文（RuneB 现在在 PivotA）
            const FRuneInstance RuneB  = Placed[DstIdx].Rune;
            const FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneA.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);
            Backpack->TryPlaceRune(RuneA, PivotB);
            Backpack->TryPlaceRune(RuneB, PivotA);

            GrabbedFromCell   = PivotA;
            SelectedCell      = PivotA;
            GamepadCursorCell = PivotA;
            // bGrabbingRune 保持 true
            OnSelectionChanged();
            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "SwapOK", "已互换：{0} ↔ {1}"),
                FText::FromName(RuneA.RuneConfig.RuneName),
                FText::FromName(RuneB.RuneConfig.RuneName)));
            OnGridNeedsRefresh();
        }
        else
        {
            // 移动到空格 → 放置成功后结束抓取
            const FIntPoint Offset   = GrabbedFromCell - PivotA;
            const FIntPoint NewPivot = GamepadCursorCell - Offset;

            if (Backpack->MoveRune(RuneA.RuneGuid, NewPivot))
            {
                bGrabbingRune   = false;
                GrabbedFromCell = FIntPoint(-1,-1);
                SelectedCell    = FIntPoint(-1,-1);
                OnSelectionChanged();
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
                    FText::FromName(RuneA.RuneConfig.RuneName)));
            }
            else
            {
                if (BackpackGridWidget) BackpackGridWidget->FlashAndShakeCell(GamepadCursorCell.X, GamepadCursorCell.Y);
                OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法放置：目标位置被占用"));
                OnGridNeedsRefresh();
            }
        }
    }
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
        UBackpackGridComponent* Backpack = GetBackpack();
        const int32 GH = Backpack ? Backpack->GridHeight : 5;
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

    // 主格子抓取状态下进入待放置区：A 键将符文送回待放置槽
    if (bGrabbingRune)
    {
        if (IsInCombatPhase()) { OnStatusMessage(NSLOCTEXT("Backpack", "CombatLock", "战斗阶段无法移动符文")); return; }

        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Backpack) return;

        FPlacedRune PR = GetRuneAtCell(GrabbedFromCell.X, GrabbedFromCell.Y);
        if (!PR.Rune.RuneGuid.IsValid()) { bGrabbingRune = false; return; }

        // 优先放入光标格（若为空），否则找第一个空格
        int32 TargetSlot = -1;
        if (PendingGrid.IsValidIndex(PendingCursorIdx) && !PendingGrid[PendingCursorIdx].RuneGuid.IsValid())
            TargetSlot = PendingCursorIdx;
        if (TargetSlot < 0)
            for (int32 i = 0; i < PendingGrid.Num(); i++)
                if (!PendingGrid[i].RuneGuid.IsValid()) { TargetSlot = i; break; }

        if (TargetSlot < 0) { OnStatusMessage(NSLOCTEXT("Backpack", "PendingFull", "待放置区已满")); return; }

        Backpack->RemoveRune(PR.Rune.RuneGuid);
        PendingGrid[TargetSlot] = PR.Rune;
        SyncPendingToPlayer();

        bGrabbingRune      = false;
        GrabbedFromCell    = FIntPoint(-1, -1);
        SelectedCell       = FIntPoint(-1, -1);
        PendingSelectedIdx = TargetSlot;
        PendingCursorIdx   = TargetSlot;
        RefreshPendingGrid();
        OnSelectionChanged();
        OnStatusMessage(FText::Format(
            NSLOCTEXT("Backpack", "UnplaceOK", "已取回：{0}"),
            FText::FromName(PR.Rune.RuneConfig.RuneName)));
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
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 GH = Backpack ? Backpack->GridHeight : 5;
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
    if (bIsPreviewMode) return;  // 只读预览模式：禁止旋转

    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    const FIntPoint TargetCell = bGrabbingRune ? GrabbedFromCell : SelectedCell;
    if (TargetCell == FIntPoint(-1, -1)) return;

    const int32 Idx = Backpack->GetRuneIndexAtCell(TargetCell);
    const TArray<FPlacedRune>& AllPlaced = Backpack->GetAllPlacedRunes();
    if (!AllPlaced.IsValidIndex(Idx)) return;

    const FPlacedRune PR = AllPlaced[Idx];
    FRuneInstance NewRune = PR.Rune;
    NewRune.Rotation = (NewRune.Rotation + 1) % 4;

    // 以符文 (0,0) 格为旋转中心：计算旋转后使 icon 格保持原位置的新 Pivot
    const FIntPoint IconAbsCell = PR.Pivot + PR.Rune.Shape.GetPivotOffset(PR.Rune.Rotation);
    const FIntPoint NewPivot    = IconAbsCell - NewRune.Shape.GetPivotOffset(NewRune.Rotation);

    Backpack->RemoveRune(PR.Rune.RuneGuid);
    bool bSuccess = Backpack->TryPlaceRune(NewRune, NewPivot);
    if (!bSuccess)
        bSuccess = Backpack->TryPlaceRune(NewRune, PR.Pivot);  // 退而求其次：原 Pivot
    if (!bSuccess)
        Backpack->TryPlaceRune(PR.Rune, PR.Pivot);              // 还原

    // icon 格保持在 IconAbsCell，将选中/抓取指针更新到该格
    if (bSuccess)
    {
        SelectedCell = IconAbsCell;
        if (bGrabbingRune) GrabbedFromCell = IconAbsCell;
        OnSelectionChanged();
    }
    OnGridNeedsRefresh();
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

    if (IsCellOccupied(Col, Row))
    {
        FPlacedRune PR = GetRuneAtCell(Col, Row);
        RuneTooltip->ShowRuneInfo(PR.Rune);

        const FVector2D Offset(16.f, -8.f);
        RuneTooltip->SetRenderTranslation(LocalPos + Offset);
    }
    else
    {
        RuneTooltip->HideTooltip();
    }
}
