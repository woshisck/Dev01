#include "UI/BackpackGridWidget.h"
#include "UI/RuneSlotWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "UI/BackpackScreenWidget.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Blueprint/WidgetTree.h"

// ============================================================
//  生命周期
// ============================================================

void UBackpackGridWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (HeatPhaseDot0) HeatPhaseDot0->OnClicked.AddDynamic(this, &UBackpackGridWidget::OnHeatPhaseDot0Clicked);
    if (HeatPhaseDot1) HeatPhaseDot1->OnClicked.AddDynamic(this, &UBackpackGridWidget::OnHeatPhaseDot1Clicked);
    if (HeatPhaseDot2) HeatPhaseDot2->OnClicked.AddDynamic(this, &UBackpackGridWidget::OnHeatPhaseDot2Clicked);

    if (GamepadHintLabel)
    {
        GamepadHintLabel->SetText(NSLOCTEXT("Backpack", "GamepadHeat", "L1 / R1   切换热度显示"));
        GamepadHintLabel->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UBackpackGridWidget::RefreshHeatPhaseButtons(int32 PreviewPhase, bool bIsGamepadMode)
{
    static const FLinearColor DotActive  (1.00f, 0.80f, 0.10f, 1.f);
    static const FLinearColor DotInactive(0.28f, 0.28f, 0.32f, 1.f);

    auto ApplyTint = [](UButton* Btn, bool bOn)
    {
        if (Btn) Btn->SetColorAndOpacity(bOn ? DotActive : DotInactive);
    };
    ApplyTint(HeatPhaseDot0, PreviewPhase == 0);
    ApplyTint(HeatPhaseDot1, PreviewPhase == 1);
    ApplyTint(HeatPhaseDot2, PreviewPhase == 2);

    if (GamepadHintLabel)
        GamepadHintLabel->SetVisibility(
            bIsGamepadMode ? ESlateVisibility::SelfHitTestInvisible
                           : ESlateVisibility::Collapsed);
}

void UBackpackGridWidget::OnHeatPhaseDot0Clicked() { OnHeatPhaseButtonClicked.Broadcast(0); }
void UBackpackGridWidget::OnHeatPhaseDot1Clicked() { OnHeatPhaseButtonClicked.Broadcast(1); }
void UBackpackGridWidget::OnHeatPhaseDot2Clicked() { OnHeatPhaseButtonClicked.Broadcast(2); }

// ============================================================
//  格子创建
// ============================================================

void UBackpackGridWidget::BuildGrid()
{
    if (!BackpackGrid)
    {
        UE_LOG(LogTemp, Error, TEXT("[BackpackGridWidget] BackpackGrid 未找到！检查 Designer 中 UniformGridPanel 名称"));
        return;
    }

    const int32 GW = 5;
    const int32 GH = 5;
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

void UBackpackGridWidget::RefreshCells(FIntPoint SelectedCell,
                                       FIntPoint HoverCell,
                                       FIntPoint GrabbedFromCell,
                                       bool bGrabbing,
                                       int32 PreviewPhase)
{
    const int32 SlotCount = CachedSlots.Num();
    if (SlotCount == 0) return;

    CachedBGrabbing       = bGrabbing;
    CachedGrabbedFromCell = GrabbedFromCell;
    CachedSelectedGuid    = FGuid();
    CachedHoverGuid       = FGuid();

    const float DimOpacity = StyleDA ? StyleDA->InactiveZoneOpacity : 0.35f;

    for (int32 i = 0; i < SlotCount; i++)
    {
        URuneSlotWidget* RuneSlot = CachedSlots[i];
        if (!RuneSlot) continue;

        const int32 Col = i % CachedGridW;
        const int32 Row = i / CachedGridW;
        const FIntPoint Cell(Col, Row);

        const bool bThisSelected = (SelectedCell == Cell);
        const bool bThisHovered  = (Cell == HoverCell);
        const bool bThisGrabbing = (bGrabbing && Cell == GrabbedFromCell);

        RuneSlot->SetSlotState(EBackpackCellState::Empty, bThisSelected, bThisHovered,
                               bThisGrabbing, StyleDA.Get(), DimOpacity, false, false);
        RuneSlot->SetRuneIcon(nullptr, 1.f);
    }

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
    return Super::NativePaint(Args, AllottedGeometry, MyCullingRect,
                              OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

// ============================================================
//  坐标辅助
// ============================================================

bool UBackpackGridWidget::GetCellAtScreenPos(const FVector2D& AbsPos,
                                              int32& OutCol, int32& OutRow) const
{
    if (!BackpackGrid) return false;

    const FGeometry& GridGeo  = BackpackGrid->GetCachedGeometry();
    const FVector2D  GridSize = GridGeo.GetLocalSize();
    if (GridSize.X <= 0.f || GridSize.Y <= 0.f) return false;

    const FVector2D LocalPos = GridGeo.AbsoluteToLocal(AbsPos);
    const int32 GW = CachedGridW;
    const int32 GH = CachedGridH;

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
