#include "UI/BackpackGridWidget.h"
#include "UI/RuneSlotWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Component/BackpackGridComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetTree.h"
#include "Rendering/DrawElements.h"

// ============================================================
//  生命周期
// ============================================================

void UBackpackGridWidget::NativeConstruct()
{
    Super::NativeConstruct();


}


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

    const float CellSize    = StyleDA ? StyleDA->CellSize    : 64.f;
    const float CellPadding = StyleDA ? StyleDA->CellPadding : 2.f;

    BackpackGrid->SetMinDesiredSlotWidth(CellSize);
    BackpackGrid->SetMinDesiredSlotHeight(CellSize);
    BackpackGrid->SetSlotPadding(FMargin(CellPadding));

    // 锁定总像素尺寸，确保每格精确为 CellSize×CellSize（1:1）
    if (GridSizeBox)
    {
        const float SlotTotal = CellSize + CellPadding * 2.f;
        GridSizeBox->SetWidthOverride(GW * SlotTotal);
        GridSizeBox->SetHeightOverride(GH * SlotTotal);
    }

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
                                       bool bGrabbing,
                                       int32 PreviewPhase)
{
    const int32 SlotCount = CachedSlots.Num();
    if (SlotCount == 0) return;

    // 缓存供 NativePaint 使用
    CachedBackpackRef    = Backpack;
    CachedBGrabbing      = bGrabbing;
    CachedGrabbedFromCell = GrabbedFromCell;

    // 确定选中符文 GUID
    CachedSelectedGuid = FGuid();
    const FIntPoint CheckCell = bGrabbing ? GrabbedFromCell : SelectedCell;
    if (Backpack && CheckCell != FIntPoint(-1, -1))
    {
        const int32 CheckIdx = Backpack->GetRuneIndexAtCell(CheckCell);
        if (CheckIdx >= 0)
            CachedSelectedGuid = Backpack->GetAllPlacedRunes()[CheckIdx].Rune.RuneGuid;
    }

    // 确定悬浮符文 GUID（无选中时显示绿框）
    CachedHoverGuid = FGuid();
    if (Backpack && HoverCell.X >= 0 && HoverCell.Y >= 0)
    {
        const int32 HIdx = Backpack->GetRuneIndexAtCell(HoverCell);
        if (HIdx >= 0)
        {
            const FGuid HGuid = Backpack->GetAllPlacedRunes()[HIdx].Rune.RuneGuid;
            if (HGuid != CachedSelectedGuid)
                CachedHoverGuid = HGuid;
        }
    }

    // 三阶叠加区格子列表（循环外查询一次，避免逐格重复调用）
    const TArray<FIntPoint> Zone0 = Backpack ? Backpack->GetActivationZoneCellsForPhase(0) : TArray<FIntPoint>{};
    const TArray<FIntPoint> Zone1 = Backpack ? Backpack->GetActivationZoneCellsForPhase(1) : TArray<FIntPoint>{};
    const TArray<FIntPoint> Zone2 = Backpack ? Backpack->GetActivationZoneCellsForPhase(2) : TArray<FIntPoint>{};

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
            const int32 Idx = Backpack->GetRuneIndexAtCell(Cell);
            bOccupied = (Idx >= 0);

            if (bOccupied)
            {
                const auto& Placed = Backpack->GetAllPlacedRunes();
                if (Placed.IsValidIndex(Idx))
                    bActivated = Placed[Idx].bIsActivated;

                State = bActivated ? EBackpackCellState::OccupiedActive
                                   : EBackpackCellState::OccupiedInactive;
            }
            else if (PreviewPhase >= 0)
            {
                // 累积模式：区 0..PreviewPhase 全部显示
                if      (Zone0.Contains(Cell)) { State = EBackpackCellState::EmptyActive; }
                else if (Zone1.Contains(Cell)) { State = EBackpackCellState::EmptyZone1;  }
                else if (Zone2.Contains(Cell)) { State = EBackpackCellState::EmptyZone2;  }
            }
        }

        // ── 推状态给 Slot ─────────────────────────────────────────────────
        const bool bThisSelected = (SelectedCell == Cell);
        const bool bThisHovered  = (Cell == HoverCell);
        const bool bThisGrabbing = (bGrabbing && Cell == GrabbedFromCell);

        const float DimOpacity = StyleDA ? StyleDA->InactiveZoneOpacity : 0.35f;
        float ZoneOpacity = 1.f;
        if (PreviewPhase >= 0)
        {
            int32 CellZone = -1;
            if      (State == EBackpackCellState::EmptyActive) CellZone = 0;
            else if (State == EBackpackCellState::EmptyZone1)  CellZone = 1;
            else if (State == EBackpackCellState::EmptyZone2)  CellZone = 2;

            if (CellZone < 0 || CellZone > PreviewPhase)
                ZoneOpacity = DimOpacity;
        }
        else if (!bOccupied)
        {
            const bool bInAnyZone = (State == EBackpackCellState::EmptyActive ||
                                     State == EBackpackCellState::EmptyZone1  ||
                                     State == EBackpackCellState::EmptyZone2);
            if (!bInAnyZone)
                ZoneOpacity = DimOpacity;
        }

        const bool bGlowZone = (PreviewPhase >= 0) && !bOccupied &&
                               (State == EBackpackCellState::EmptyActive ||
                                State == EBackpackCellState::EmptyZone1  ||
                                State == EBackpackCellState::EmptyZone2) &&
                               (ZoneOpacity >= 1.0f - KINDA_SMALL_NUMBER);

        const bool bCellDisabled = Backpack ? Backpack->IsCellDisabled(Cell) : false;

        RuneSlot->SetSlotState(State, bThisSelected, bThisHovered, bThisGrabbing,
                               StyleDA.Get(), ZoneOpacity, bGlowZone, bCellDisabled);

        // ── 符文图标 ─────────────────────────────────────────────────────
        UTexture2D* Tex = nullptr;
        if (bOccupied && Backpack)
        {
            const int32 Idx    = Backpack->GetRuneIndexAtCell(Cell);
            const auto& Placed = Backpack->GetAllPlacedRunes();
            if (Placed.IsValidIndex(Idx))
            {
                const FPlacedRune& PR = Placed[Idx];
                // DA (0,0) 格（旋转后的主 icon 格）显示符文图标，其余格显示通用图标
                const FIntPoint IconAbsCell = PR.Pivot + PR.Rune.Shape.GetPivotOffset(PR.Rune.Rotation);
                if (Cell == IconAbsCell)
                    Tex = PR.Rune.RuneConfig.RuneIcon;
                else if (StyleDA && StyleDA->CellMultipartIcon)
                    Tex = StyleDA->CellMultipartIcon;
            }
        }

        const float IconOpacity = bThisGrabbing ? 0.30f : 1.f;
        RuneSlot->SetRuneIcon(Tex, IconOpacity);
    }

    // 触发重绘（边框在 NativePaint 中绘制）
    Invalidate(EInvalidateWidgetReason::Paint);
}

// ============================================================
//  NativePaint — 符文包围框边线
// ============================================================

int32 UBackpackGridWidget::NativePaint(const FPaintArgs& Args,
                                        const FGeometry& AllottedGeometry,
                                        const FSlateRect& MyCullingRect,
                                        FSlateWindowElementList& OutDrawElements,
                                        int32 LayerId,
                                        const FWidgetStyle& InWidgetStyle,
                                        bool bParentEnabled) const
{
    const int32 SuperLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
                                                OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    UBackpackGridComponent* Backpack = CachedBackpackRef.Get();
    if (!Backpack || !BackpackGrid) return SuperLayer;

    if (CachedSlots.IsEmpty()) return SuperLayer;

    static const FLinearColor YellowBorder(1.00f, 0.82f, 0.10f, 1.0f);
    static const FLinearColor GrayBorder  (0.40f, 0.40f, 0.42f, 0.7f);

    const int32 BorderLayer = SuperLayer + 1;

    for (const FPlacedRune& PR : Backpack->GetAllPlacedRunes())
    {
        // 计算旋转后的包围盒
        FRuneShape RotShape = PR.Rune.Shape;
        const int32 RotCount = PR.Rune.Rotation % 4;
        for (int32 r = 0; r < RotCount; r++) RotShape = RotShape.Rotate90();
        if (RotShape.Cells.IsEmpty()) continue;

        int32 MinCol = INT32_MAX, MinRow = INT32_MAX;
        int32 MaxCol = INT32_MIN, MaxRow = INT32_MIN;
        for (const FIntPoint& Offset : RotShape.Cells)
        {
            const FIntPoint AbsCell = PR.Pivot + Offset;
            MinCol = FMath::Min(MinCol, AbsCell.X);
            MaxCol = FMath::Max(MaxCol, AbsCell.X);
            MinRow = FMath::Min(MinRow, AbsCell.Y);
            MaxRow = FMath::Max(MaxRow, AbsCell.Y);
        }

        // 直接从边角 slot 的 CachedGeometry 读取屏幕位置，无需手动计算偏移
        const int32 TLIdx = MinRow * CachedGridW + MinCol;
        const int32 BRIdx = MaxRow * CachedGridW + MaxCol;
        if (!CachedSlots.IsValidIndex(TLIdx) || !CachedSlots[TLIdx]) continue;
        if (!CachedSlots.IsValidIndex(BRIdx) || !CachedSlots[BRIdx]) continue;

        const FGeometry& TLGeo = CachedSlots[TLIdx]->GetCachedGeometry();
        const FGeometry& BRGeo = CachedSlots[BRIdx]->GetCachedGeometry();

        const FVector2D TLLocal = AllottedGeometry.AbsoluteToLocal(TLGeo.LocalToAbsolute(FVector2D::ZeroVector));
        const FVector2D BRLocal = AllottedGeometry.AbsoluteToLocal(BRGeo.LocalToAbsolute(BRGeo.GetLocalSize()));

        const float W = BRLocal.X - TLLocal.X;
        const float H = BRLocal.Y - TLLocal.Y;

        static const FLinearColor GreenBorder(0.20f, 0.90f, 0.30f, 0.85f);
        const bool bIsSelected = (PR.Rune.RuneGuid == CachedSelectedGuid);
        const bool bIsHover    = !bIsSelected && (PR.Rune.RuneGuid == CachedHoverGuid);
        const FLinearColor BorderColor = bIsSelected ? YellowBorder
                                       : bIsHover    ? GreenBorder
                                       :               GrayBorder;
        const float BorderWidth = bIsSelected ? 2.5f : (bIsHover ? 2.0f : 1.5f);

        TArray<FVector2D> Points;
        Points.Reserve(5);
        Points.Add(TLLocal);
        Points.Add(TLLocal + FVector2D(W, 0.f));
        Points.Add(TLLocal + FVector2D(W, H));
        Points.Add(TLLocal + FVector2D(0.f, H));
        Points.Add(TLLocal);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            BorderLayer,
            AllottedGeometry.ToPaintGeometry(),
            Points,
            ESlateDrawEffect::None,
            BorderColor,
            true,
            BorderWidth);
    }

    return BorderLayer;
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

void UBackpackGridWidget::FlashAndShakeCell(int32 Col, int32 Row)
{
    const int32 Idx = Row * CachedGridW + Col;
    if (CachedSlots.IsValidIndex(Idx) && CachedSlots[Idx])
        CachedSlots[Idx]->ShakeAndFlash();
}
