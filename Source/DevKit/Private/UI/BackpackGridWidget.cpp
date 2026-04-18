#include "UI/BackpackGridWidget.h"
#include "UI/RuneSlotWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "UI/BackpackScreenWidget.h"
#include "Component/BackpackGridComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetTree.h"

// ============================================================
//  格子创建
// ============================================================

void UBackpackGridWidget::BuildGrid(UBackpackGridComponent* InBackpack)
{
    if (!BackpackGrid)
    {
        UE_LOG(LogTemp, Error, TEXT("[BackpackGridWidget] BackpackGrid 未找到！检查 Designer 中 UniformGridPanel 名称"));
        return;
    }

    const int32 GW = InBackpack ? InBackpack->GridWidth  : 5;
    const int32 GH = InBackpack ? InBackpack->GridHeight : 5;
    CachedGridW = GW;
    CachedGridH = GH;

    BackpackGrid->ClearChildren();
    CachedSlots.Empty();

    const float CellSize    = StyleDA ? StyleDA->CellSize        : 64.f;
    const float CellPadding = StyleDA ? StyleDA->CellPadding     : 2.f;

    BackpackGrid->SetMinDesiredSlotWidth(CellSize);
    BackpackGrid->SetMinDesiredSlotHeight(CellSize);
    BackpackGrid->SetSlotPadding(FMargin(CellPadding));

    // 激活区动画材质：所有格子共用一个 DynMat（材质内部用 Time 节点自驱动动画）
    UMaterialInstanceDynamic* ActiveZoneDynMat = nullptr;
    if (StyleDA && StyleDA->ActiveZoneAnimMaterial)
    {
        ActiveZoneDynMat = UMaterialInstanceDynamic::Create(
            StyleDA->ActiveZoneAnimMaterial, this);
    }

    // 确定 Slot 类：有填 WBP_RuneSlot 用填的，否则用 C++ 基类（纯色，无动效）
    TSubclassOf<URuneSlotWidget> SlotClass = RuneSlotClass
        ? RuneSlotClass
        : TSubclassOf<URuneSlotWidget>(URuneSlotWidget::StaticClass());

    for (int32 Row = 0; Row < GH; Row++)
    {
        for (int32 Col = 0; Col < GW; Col++)
        {
            URuneSlotWidget* RuneSlot = CreateWidget<URuneSlotWidget>(GetOwningPlayer(), SlotClass);
            if (!RuneSlot)
            {
                CachedSlots.Add(nullptr);
                continue;
            }

            // 绑定激活区动画材质（RuneSlot 内部 ActiveZoneOverlay 为 null 时跳过）
            if (ActiveZoneDynMat)
                RuneSlot->SetActiveZoneMaterial(ActiveZoneDynMat);

            UUniformGridSlot* GridSlot = BackpackGrid->AddChildToUniformGrid(RuneSlot, Row, Col);
            GridSlot->SetHorizontalAlignment(HAlign_Fill);
            GridSlot->SetVerticalAlignment(VAlign_Fill);

            CachedSlots.Add(RuneSlot);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[BackpackGridWidget] 创建 %d×%d 格子，SlotClass=%s"),
        GW, GH, *SlotClass->GetName());
}

// ============================================================
//  格子刷新
// ============================================================

void UBackpackGridWidget::RefreshCells(UBackpackGridComponent* Backpack,
                                       FIntPoint SelectedCell,
                                       FIntPoint HoverCell,
                                       FIntPoint GrabbedFromCell,
                                       bool bGrabbing)
{
    const int32 SlotCount = CachedSlots.Num();
    if (SlotCount == 0) return;

    for (int32 i = 0; i < SlotCount; i++)
    {
        URuneSlotWidget* RuneSlot = CachedSlots[i];
        if (!RuneSlot) continue;

        const int32 Col = i % CachedGridW;
        const int32 Row = i / CachedGridW;
        const FIntPoint Cell(Col, Row);

        // ── 确定格子状态 ──────────────────────────────────────────────────
        EBackpackCellState State = EBackpackCellState::Empty;
        bool bOccupied  = false;
        bool bActivated = false;

        if (Backpack)
        {
            const bool bInZone = Backpack->GetActivationZoneCells().Contains(Cell);
            const int32 Idx    = Backpack->GetRuneIndexAtCell(Cell);
            bOccupied = (Idx >= 0);

            if (bOccupied)
            {
                const auto& Placed = Backpack->GetAllPlacedRunes();
                if (Placed.IsValidIndex(Idx))
                    bActivated = Placed[Idx].bIsActivated;

                State = bActivated ? EBackpackCellState::OccupiedActive
                                   : EBackpackCellState::OccupiedInactive;
            }
            else
            {
                State = bInZone ? EBackpackCellState::EmptyActive
                                : EBackpackCellState::Empty;
            }
        }

        // ── 推状态给 Slot（Slot 内部只在变化时触发 BP 事件） ─────────────
        const bool bThisSelected = (SelectedCell == Cell);
        const bool bThisHovered  = (Cell == HoverCell);
        const bool bThisGrabbing = (bGrabbing && Cell == GrabbedFromCell);

        RuneSlot->SetSlotState(State, bThisSelected, bThisHovered, bThisGrabbing, StyleDA.Get());

        // ── 符文图标 ─────────────────────────────────────────────────────
        UTexture2D* Tex = nullptr;
        if (bOccupied && Backpack)
        {
            const int32 Idx    = Backpack->GetRuneIndexAtCell(Cell);
            const auto& Placed = Backpack->GetAllPlacedRunes();
            if (Placed.IsValidIndex(Idx))
                Tex = Placed[Idx].Rune.RuneConfig.RuneIcon;
        }

        const float IconOpacity = bThisGrabbing ? 0.30f : 1.f;
        RuneSlot->SetRuneIcon(Tex, IconOpacity);
    }
}

// ============================================================
//  坐标辅助
// ============================================================

bool UBackpackGridWidget::GetCellAtScreenPos(const FVector2D& AbsPos, UBackpackGridComponent* Backpack,
                                              int32& OutCol, int32& OutRow) const
{
    if (!BackpackGrid) return false;

    const FGeometry& GridGeo  = BackpackGrid->GetCachedGeometry();
    const FVector2D  GridSize = GridGeo.GetLocalSize();
    if (GridSize.X <= 0.f || GridSize.Y <= 0.f) return false;

    const FVector2D LocalPos = GridGeo.AbsoluteToLocal(AbsPos);
    const int32 GW = Backpack ? Backpack->GridWidth  : CachedGridW;
    const int32 GH = Backpack ? Backpack->GridHeight : CachedGridH;

    const float CellW = GridSize.X / GW;
    const float CellH = GridSize.Y / GH;

    const int32 Col = FMath::FloorToInt(LocalPos.X / CellW);
    const int32 Row = FMath::FloorToInt(LocalPos.Y / CellH);

    if (Col < 0 || Col >= GW || Row < 0 || Row >= GH) return false;

    OutCol = Col;
    OutRow = Row;
    return true;
}

FGeometry UBackpackGridWidget::GetGridGeometry() const
{
    return BackpackGrid ? BackpackGrid->GetCachedGeometry() : FGeometry();
}
