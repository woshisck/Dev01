#include "UI/PendingGridWidget.h"
#include "UI/RuneSlotWidget.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"

// ============================================================
//  格子创建
// ============================================================

void UPendingGridWidget::BuildSlots()
{
    if (!PendingRuneGrid) return;

    PendingRuneGrid->ClearChildren();
    CachedSlots.Empty();

    const float CellSize    = StyleDA ? StyleDA->CellSize    : 64.f;
    const float CellPadding = StyleDA ? StyleDA->CellPadding : 2.f;

    PendingRuneGrid->SetMinDesiredSlotWidth(CellSize);
    PendingRuneGrid->SetMinDesiredSlotHeight(CellSize);
    PendingRuneGrid->SetSlotPadding(FMargin(CellPadding));

    const int32 Cols = FMath::Max(1, PendingGridCols);
    const int32 Rows = FMath::Max(1, PendingGridRows);

    if (PendingGridSizeBox)
    {
        const float SlotTotal = CellSize + CellPadding * 2.f;
        PendingGridSizeBox->SetWidthOverride(Cols * SlotTotal);
        PendingGridSizeBox->SetHeightOverride(Rows * SlotTotal);
    }

    TSubclassOf<URuneSlotWidget> SlotClass = RuneSlotClass
        ? RuneSlotClass
        : TSubclassOf<URuneSlotWidget>(URuneSlotWidget::StaticClass());

    for (int32 Row = 0; Row < Rows; Row++)
    {
        for (int32 Col = 0; Col < Cols; Col++)
        {
            URuneSlotWidget* RuneSlot = CreateWidget<URuneSlotWidget>(GetOwningPlayer(), SlotClass);
            if (!RuneSlot) { CachedSlots.Add(nullptr); continue; }

            UUniformGridSlot* GridSlot = PendingRuneGrid->AddChildToUniformGrid(RuneSlot, Row, Col);
            GridSlot->SetHorizontalAlignment(HAlign_Fill);
            GridSlot->SetVerticalAlignment(VAlign_Fill);
            CachedSlots.Add(RuneSlot);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[PendingGridWidget] 创建 %d×%d 格子完成"), Cols, Rows);
}

// ============================================================
//  格子刷新
// ============================================================

void UPendingGridWidget::RefreshSlots(const TArray<FRuneInstance>& Grid,
                                      int32 CursorIdx,
                                      int32 GrabbedIdx)
{
    const float DimOpacity = StyleDA ? StyleDA->InactiveZoneOpacity : 0.35f;

    for (int32 i = 0; i < CachedSlots.Num(); i++)
    {
        URuneSlotWidget* RuneSlot = CachedSlots[i];
        if (!RuneSlot) continue;

        const bool bHasRune  = Grid.IsValidIndex(i) && Grid[i].RuneGuid.IsValid();
        const EBackpackCellState State = bHasRune
            ? EBackpackCellState::OccupiedInactive
            : EBackpackCellState::Empty;

        const bool bIsSelected = (i == CursorIdx);
        const bool bIsGrabbing = (i == GrabbedIdx);
        const float ZoneOpacity = bHasRune ? 1.f : DimOpacity;

        RuneSlot->SetSlotState(State, bIsSelected, false, bIsGrabbing, StyleDA.Get(), ZoneOpacity);

        UTexture2D* Tex = (bHasRune && Grid[i].RuneConfig.RuneIcon)
            ? Grid[i].RuneConfig.RuneIcon.Get() : nullptr;
        RuneSlot->SetRuneIcon(Tex, bIsGrabbing ? 0.3f : 1.f);
    }
}

// ============================================================
//  坐标辅助
// ============================================================

bool UPendingGridWidget::GetSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const
{
    if (!PendingRuneGrid) return false;

    const FGeometry& PanelGeo  = PendingRuneGrid->GetCachedGeometry();
    const FVector2D  PanelSize = PanelGeo.GetLocalSize();
    if (PanelSize.X <= 0.f || PanelSize.Y <= 0.f) return false;

    const FVector2D LocalPos = PanelGeo.AbsoluteToLocal(AbsPos);
    const int32 Cols = FMath::Max(1, PendingGridCols);
    const int32 Rows = FMath::Max(1, PendingGridRows);
    const float CellW = PanelSize.X / Cols;
    const float CellH = PanelSize.Y / Rows;

    const int32 Col = FMath::FloorToInt(LocalPos.X / CellW);
    const int32 Row = FMath::FloorToInt(LocalPos.Y / CellH);
    if (Col < 0 || Col >= Cols || Row < 0 || Row >= Rows) return false;

    OutIndex = Row * Cols + Col;
    return true;
}

bool UPendingGridWidget::GetNearestSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const
{
    if (!PendingRuneGrid) return false;

    const FGeometry& PanelGeo  = PendingRuneGrid->GetCachedGeometry();
    const FVector2D  PanelSize = PanelGeo.GetLocalSize();
    if (PanelSize.X <= 0.f || PanelSize.Y <= 0.f) return false;

    FVector2D LocalPos = PanelGeo.AbsoluteToLocal(AbsPos);
    LocalPos.X = FMath::Clamp(LocalPos.X, 0.f, PanelSize.X - 1.f);
    LocalPos.Y = FMath::Clamp(LocalPos.Y, 0.f, PanelSize.Y - 1.f);

    const int32 Cols = FMath::Max(1, PendingGridCols);
    const int32 Rows = FMath::Max(1, PendingGridRows);
    const float CellW = PanelSize.X / Cols;
    const float CellH = PanelSize.Y / Rows;

    const int32 Col = FMath::Clamp(FMath::FloorToInt(LocalPos.X / CellW), 0, Cols - 1);
    const int32 Row = FMath::Clamp(FMath::FloorToInt(LocalPos.Y / CellH), 0, Rows - 1);

    OutIndex = Row * Cols + Col;
    return true;
}
