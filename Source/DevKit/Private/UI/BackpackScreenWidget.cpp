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
#include "InputCoreTypes.h"

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

void UBackpackScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SetVisibility(ESlateVisibility::Collapsed);
    bIsFocusable = true;

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
        PendingGridWidget->BuildSlots();

    RefreshPendingRuneSlots();

    if (SellButton)
        SellButton->OnClicked.AddDynamic(this, &UBackpackScreenWidget::OnSellButtonClicked);

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
            bGrabbingRune);
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
        SelectedCell = Cell;
        SelectedRuneIndex = -1;
        OnSelectionChanged();
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
    if (!PendingGridWidget) return;

    APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
    const TArray<FRuneInstance>& PendingRunes = Player ? Player->PendingRunes : TArray<FRuneInstance>();
    PendingGridWidget->RefreshSlots(PendingRunes);
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
    {
        PC->SetPause(true);
        PC->SetShowMouseCursor(true);
    }

    SetUserFocus(GetOwningPlayer());
    RefreshPendingRuneSlots();
    OnGridNeedsRefresh();
    OnSelectionChanged();
}

void UBackpackScreenWidget::NativeOnDeactivated()
{
    SetVisibility(ESlateVisibility::Collapsed);

    bGrabbingRune   = false;
    GrabbedFromCell = FIntPoint(-1, -1);
    ClearSelection();

    if (APlayerController* PC = GetOwningPlayer())
    {
        PC->SetPause(false);
        PC->SetShowMouseCursor(false);
    }

    Super::NativeOnDeactivated();
}

// ============================================================
//  出售按钮
// ============================================================

void UBackpackScreenWidget::OnSellButtonClicked()
{
    if (SelectedCell == FIntPoint(-1, -1)) return;
    if (UBackpackGridComponent* Backpack = GetBackpack())
    {
        FPlacedRune PR = GetRuneAtCell(SelectedCell.X, SelectedCell.Y);
        if (PR.Rune.RuneGuid.IsValid())
            Backpack->SellRune(PR.Rune.RuneGuid);
    }
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
            APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
            if (Player && PendingIdx < Player->PendingRunes.Num())
            {
                PendingDragIndex = PendingIdx;
                return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
            }
            return FReply::Handled();
        }
    }

    int32 Col, Row;
    if (!GetGridCellAtScreenPos(InMouseEvent.GetScreenSpacePosition(), Col, Row))
        return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    if (IsCellOccupied(Col, Row))
    {
        SelectedCell      = FIntPoint(Col, Row);
        SelectedRuneIndex = -1;
        GamepadCursorCell = FIntPoint(Col, Row);
        OnSelectionChanged();

        PendingDragCol = Col;
        PendingDragRow = Row;

        return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
    }
    else
    {
        PendingDragCol = -1;
        PendingDragRow = -1;
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
    if (PendingDragIndex >= 0)
    {
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        if (Player && PendingDragIndex < Player->PendingRunes.Num())
        {
            const FRuneInstance& PendingRune = Player->PendingRunes[PendingDragIndex];

            URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
            DragOp->SrcCol            = -1;
            DragOp->SrcRow            = -1;
            DragOp->SrcPivot          = FIntPoint(-1, -1);
            DragOp->DraggedRune       = PendingRune;
            DragOp->PendingSourceIndex = PendingDragIndex;

            bMouseDragging  = true;
            MouseDragTex    = PendingRune.RuneConfig.RuneIcon;
            LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

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

    URuneDragDropOperation* DragOp = NewObject<URuneDragDropOperation>(this);
    DragOp->SrcCol      = PendingDragCol;
    DragOp->SrcRow      = PendingDragRow;
    DragOp->SrcPivot    = PR.Pivot;
    DragOp->DraggedRune = PR.Rune;

    bMouseDragging  = true;
    MouseDragTex    = PR.Rune.RuneConfig.RuneIcon;
    LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();

    PendingDragCol = PendingDragRow = -1;
    OutOperation = DragOp;
}

bool UBackpackScreenWidget::NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    if (!Cast<URuneDragDropOperation>(InOperation))
        return false;

    LastMouseAbsPos = InDragDropEvent.GetScreenSpacePosition();

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
    bMouseDragging = false;
    MouseDragTex   = nullptr;
    HoverCol = HoverRow = -1;

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
            APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
            if (Backpack && Player)
            {
                // 通过 Y 坐标（X 钳制到面板内）确定目标槽位，再 Insert 到对应位置
                int32 TargetSlot = Player->PendingRunes.Num();
                if (PendingGridWidget)
                {
                    int32 NearestIdx;
                    if (PendingGridWidget->GetNearestSlotAtScreenPos(
                            InDragDropEvent.GetScreenSpacePosition(), NearestIdx))
                        TargetSlot = FMath::Min(NearestIdx, Player->PendingRunes.Num());
                }

                Backpack->RemoveRune(RuneOp->DraggedRune.RuneGuid);
                Player->PendingRunes.Insert(RuneOp->DraggedRune, TargetSlot);
                RefreshPendingRuneSlots();
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
        APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
        UBackpackGridComponent* Backpack = GetBackpack();
        if (!Player || !Backpack || RuneOp->PendingSourceIndex >= Player->PendingRunes.Num())
        {
            OnGridNeedsRefresh();
            return false;
        }

        int32 TargetCol, TargetRow;
        if (!GetGridCellAtScreenPos(InDragDropEvent.GetScreenSpacePosition(), TargetCol, TargetRow))
        {
            OnGridNeedsRefresh();
            return false;
        }

        UE_LOG(LogTemp, Warning, TEXT("[BackpackDrop] Pending->Grid Drop pos=(%f,%f) Cell=(%d,%d)"),
            InDragDropEvent.GetScreenSpacePosition().X, InDragDropEvent.GetScreenSpacePosition().Y,
            TargetCol, TargetRow);

        const FRuneInstance PendingRune = Player->PendingRunes[RuneOp->PendingSourceIndex];
        if (Backpack->TryPlaceRune(PendingRune, FIntPoint(TargetCol, TargetRow)))
        {
            Player->PendingRunes.RemoveAt(RuneOp->PendingSourceIndex);
            RefreshPendingRuneSlots();
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
                SelectedCell = FIntPoint(-1, -1);
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
    OnGridNeedsRefresh();
}

// ============================================================
//  手柄输入
// ============================================================

FReply UBackpackScreenWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    bIsGamepadInputMode = false;
    LastMouseAbsPos = InMouseEvent.GetScreenSpacePosition();
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

    if (GrabbedRuneIcon)
    {
        const UBackpackStyleDataAsset* Style = BackpackGridWidget ? BackpackGridWidget->StyleDA.Get() : nullptr;
        const float CellPx  = Style ? Style->CellSize : 64.f;
        const float HalfPx  = CellPx * 0.5f;
        const float FloatY  = FMath::Sin(GetWorld()->GetTimeSeconds() * 3.f) * 5.f;

        if (bMouseDragging && MouseDragTex)
        {
            GrabbedRuneIcon->SetBrushFromTexture(MouseDragTex, false);
            GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

            const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(LastMouseAbsPos);
            FWidgetTransform T;
            T.Translation = LocalPos + FVector2D(-HalfPx, -HalfPx + FloatY);
            GrabbedRuneIcon->SetRenderTransform(T);
        }
        else if (bGrabbingRune && BackpackGridWidget)
        {
            UTexture2D* Tex = GetRuneIconAtCell(GrabbedFromCell.X, GrabbedFromCell.Y);
            if (Tex)
            {
                GrabbedRuneIcon->SetBrushFromTexture(Tex, false);
                GrabbedRuneIcon->SetVisibility(ESlateVisibility::HitTestInvisible);

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

    if      (HeldDirKey == EKeys::Gamepad_DPad_Up)    MoveGamepadCursor( 0, -1);
    else if (HeldDirKey == EKeys::Gamepad_DPad_Down)  MoveGamepadCursor( 0,  1);
    else if (HeldDirKey == EKeys::Gamepad_DPad_Left)  MoveGamepadCursor(-1,  0);
    else if (HeldDirKey == EKeys::Gamepad_DPad_Right) MoveGamepadCursor( 1,  0);
}

FReply UBackpackScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const FKey Key = InKeyEvent.GetKey();

    bIsGamepadInputMode = true;

    if (Key == EKeys::Gamepad_RightStick_Up   || Key == EKeys::Gamepad_RightStick_Down  ||
        Key == EKeys::Gamepad_RightStick_Left  || Key == EKeys::Gamepad_RightStick_Right ||
        Key == EKeys::Gamepad_LeftStick_Up     || Key == EKeys::Gamepad_LeftStick_Down   ||
        Key == EKeys::Gamepad_LeftStick_Left   || Key == EKeys::Gamepad_LeftStick_Right)
    {
        return FReply::Handled();
    }

    if (Key == EKeys::Gamepad_Special_Left || Key == EKeys::Tab)
    {
        DeactivateWidget();
        return FReply::Handled();
    }

    auto StartDirRepeat = [&](int32 DC, int32 DR)
    {
        MoveGamepadCursor(DC, DR);
        HeldDirKey     = Key;
        bDirKeyHeld    = true;
        HeldKeyTime    = 0.f;
        LastRepeatCount = 0;
        return FReply::Handled();
    };

    if (Key == EKeys::Gamepad_DPad_Up)    return StartDirRepeat( 0, -1);
    if (Key == EKeys::Gamepad_DPad_Down)  return StartDirRepeat( 0,  1);
    if (Key == EKeys::Gamepad_DPad_Left)  return StartDirRepeat(-1,  0);
    if (Key == EKeys::Gamepad_DPad_Right) return StartDirRepeat( 1,  0);

    if (!InKeyEvent.IsRepeat())
    {
        if (Key == EKeys::Gamepad_FaceButton_Bottom) { GamepadConfirm(); return FReply::Handled(); }
        if (Key == EKeys::Gamepad_FaceButton_Right)  { GamepadCancel();  return FReply::Handled(); }
        if (Key == EKeys::Gamepad_FaceButton_Top)
        {
            SelectedCell = GamepadCursorCell;
            RemoveRuneAtSelectedCell();
            return FReply::Handled();
        }
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UBackpackScreenWidget::MoveGamepadCursor(int32 DCol, int32 DRow)
{
    UBackpackGridComponent* Backpack = GetBackpack();
    const int32 W = Backpack ? Backpack->GridWidth  : 5;
    const int32 H = Backpack ? Backpack->GridHeight : 5;

    GamepadCursorCell.X = FMath::Clamp(GamepadCursorCell.X + DCol, 0, W - 1);
    GamepadCursorCell.Y = FMath::Clamp(GamepadCursorCell.Y + DRow, 0, H - 1);

    if (!bGrabbingRune)
    {
        SelectedCell = GamepadCursorCell;
        OnSelectionChanged();
    }
    else
    {
        OnGridNeedsRefresh();
    }

    UpdateTooltipForCell(GamepadCursorCell.X, GamepadCursorCell.Y, FVector2D::ZeroVector);
}

void UBackpackScreenWidget::GamepadConfirm()
{
    UBackpackGridComponent* Backpack = GetBackpack();
    if (!Backpack) return;

    if (!bGrabbingRune)
    {
        int32 RuneIdx = Backpack->GetRuneIndexAtCell(GamepadCursorCell);
        if (RuneIdx >= 0)
        {
            bGrabbingRune    = true;
            GrabbedFromCell  = GamepadCursorCell;
            SelectedCell     = GamepadCursorCell;
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

        FRuneInstance RuneA = Placed[SrcIdx].Rune;

        if (DstIdx >= 0)
        {
            FRuneInstance RuneB  = Placed[DstIdx].Rune;
            FIntPoint     PivotA = Placed[SrcIdx].Pivot;
            FIntPoint     PivotB = Placed[DstIdx].Pivot;

            Backpack->RemoveRune(RuneA.RuneGuid);
            Backpack->RemoveRune(RuneB.RuneGuid);
            Backpack->TryPlaceRune(RuneA, PivotB);
            Backpack->TryPlaceRune(RuneB, PivotA);

            OnStatusMessage(FText::Format(
                NSLOCTEXT("Backpack", "SwapOK", "已互换：{0} ↔ {1}"),
                FText::FromName(RuneA.RuneConfig.RuneName),
                FText::FromName(RuneB.RuneConfig.RuneName)));
        }
        else
        {
            FIntPoint SrcPivot = Placed[SrcIdx].Pivot;
            FIntPoint Offset   = GrabbedFromCell - SrcPivot;
            FIntPoint NewPivot = GamepadCursorCell - Offset;

            const bool bMoved = Backpack->MoveRune(RuneA.RuneGuid, NewPivot);
            if (bMoved)
            {
                OnStatusMessage(FText::Format(
                    NSLOCTEXT("Backpack", "MoveOK", "已移动：{0}"),
                    FText::FromName(RuneA.RuneConfig.RuneName)));
            }
            else
            {
                OnStatusMessage(NSLOCTEXT("Backpack", "MoveFail", "无法放置：目标位置被占用"));
            }
        }

        bGrabbingRune   = false;
        GrabbedFromCell = FIntPoint(-1, -1);
        SelectedCell    = GamepadCursorCell;
        OnGridNeedsRefresh();
    }
}

void UBackpackScreenWidget::GamepadCancel()
{
    bGrabbingRune   = false;
    GrabbedFromCell = FIntPoint(-1, -1);
    SelectedCell    = FIntPoint(-1, -1);
    OnSelectionChanged();
    OnStatusMessage(NSLOCTEXT("Backpack", "GrabCancelled", "已取消"));
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
