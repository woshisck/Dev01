#include "UI/RuneSlotWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"

// ============================================================
//  内部颜色默认值（StyleDA 为 null 时使用）
// ============================================================
namespace SlotDefaults
{
    static const FLinearColor Empty          (0.40f, 0.40f, 0.42f, 1.f);
    static const FLinearColor EmptyActive    (0.15f, 0.35f, 0.75f, 1.f);
    static const FLinearColor OccupiedActive (0.10f, 0.55f, 1.00f, 1.f);
    static const FLinearColor OccupiedInact  (0.55f, 0.35f, 0.05f, 1.f);
    static const FLinearColor Selected       (1.00f, 0.82f, 0.10f, 1.f);
    static const FLinearColor Hover          (0.10f, 0.80f, 0.20f, 1.f);
    static const FLinearColor GrabbedSource  (0.25f, 0.15f, 0.03f, 1.f);
}

// ============================================================
//  格子状态设置
// ============================================================

void URuneSlotWidget::SetSlotState(EBackpackCellState State,
                                   bool bSelected,
                                   bool bHovered,
                                   bool bGrabbing,
                                   const UBackpackStyleDataAsset* Style)
{
    // ── 确定背景颜色和纹理 ────────────────────────────────────────────────
    FLinearColor BGColor;
    UTexture2D*  BGTex = nullptr;

    if (bSelected)
    {
        BGColor = Style ? Style->SelectedColor        : SlotDefaults::Selected;
        BGTex   = Style ? Style->CellSelectedTexture.Get()    : nullptr;
    }
    else if (bGrabbing)
    {
        BGColor = Style ? Style->GrabbedSourceColor   : SlotDefaults::GrabbedSource;
        BGTex   = Style ? Style->CellGrabbedSourceTexture.Get() : nullptr;
    }
    else if (bHovered)
    {
        BGColor = Style ? Style->HoverColor           : SlotDefaults::Hover;
        BGTex   = Style ? Style->CellHoverTexture.Get()       : nullptr;
    }
    else
    {
        switch (State)
        {
        case EBackpackCellState::EmptyActive:
            BGColor = Style ? Style->HeatZone0Color            : SlotDefaults::EmptyActive;
            BGTex   = Style ? Style->CellActiveTexture.Get()          : nullptr;
            break;
        case EBackpackCellState::EmptyZone1:
            BGColor = Style ? Style->HeatZone1Color            : FLinearColor(0.15f, 0.35f, 0.75f, 1.f);
            BGTex   = nullptr;
            break;
        case EBackpackCellState::EmptyZone2:
            BGColor = Style ? Style->HeatZone2Color            : FLinearColor(0.08f, 0.20f, 0.48f, 1.f);
            BGTex   = nullptr;
            break;
        case EBackpackCellState::OccupiedActive:
            BGColor = Style ? Style->OccupiedActiveColor       : SlotDefaults::OccupiedActive;
            BGTex   = Style ? Style->CellOccupiedActiveTexture.Get()  : nullptr;
            break;
        case EBackpackCellState::OccupiedInactive:
            BGColor = Style ? Style->OccupiedInactiveColor     : SlotDefaults::OccupiedInact;
            BGTex   = Style ? Style->CellOccupiedInactiveTexture.Get(): nullptr;
            break;
        default:
            BGColor = Style ? Style->EmptyColor                : SlotDefaults::Empty;
            BGTex   = Style ? Style->CellEmptyTexture.Get()           : nullptr;
            break;
        }
    }

    // ── 更新背景 UImage ────────────────────────────────────────────────────
    if (CellBG)
    {
        if (BGTex)
            CellBG->SetBrushFromTexture(BGTex, false);
        // 颜色作为 Tint 叠加在纹理上（纯色时 Tint 即为最终颜色）
        CellBG->SetColorAndOpacity(BGColor);
    }

    // ── 激活区特效层：仅在激活区格子上显示 ──────────────────────────────
    if (ActiveZoneOverlay)
    {
        const bool bInActiveZone = (State == EBackpackCellState::EmptyActive  ||
                                    State == EBackpackCellState::EmptyZone1   ||
                                    State == EBackpackCellState::EmptyZone2   ||
                                    State == EBackpackCellState::OccupiedActive);
        if (bInActiveZone)
        {
            const float Opacity = Style ? Style->ActiveZoneOverlayOpacity : 0.6f;
            ActiveZoneOverlay->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Opacity));
            ActiveZoneOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            ActiveZoneOverlay->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // ── 触发 Blueprint 动效事件（仅状态变化时触发，避免每帧重复） ─────────
    if (State != PrevState)
    {
        OnCellStateChanged(State);
        PrevState = State;
    }

    if (bSelected != bPrevSelected)
    {
        OnSelectedChanged(bSelected);
        bPrevSelected = bSelected;
    }

    if (bHovered != bPrevHovered)
    {
        OnHoveredChanged(bHovered);
        bPrevHovered = bHovered;
    }

    if (bGrabbing != bPrevGrabbing)
    {
        OnGrabbingChanged(bGrabbing);
        bPrevGrabbing = bGrabbing;
    }
}

// ============================================================
//  符文图标
// ============================================================

void URuneSlotWidget::SetRuneIcon(UTexture2D* Tex, float Opacity)
{
    if (!CellIcon) return;

    if (Tex)
    {
        CellIcon->SetBrushFromTexture(Tex, false);
        CellIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, Opacity));
        CellIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
    }
    else
    {
        CellIcon->SetVisibility(ESlateVisibility::Collapsed);
    }

    OnRuneIconSet(Tex, Opacity);
}

// ============================================================
//  激活区动画材质（BuildGrid 时由 BackpackGridWidget 调用一次）
// ============================================================

void URuneSlotWidget::SetActiveZoneMaterial(UMaterialInstanceDynamic* DynMat)
{
    if (ActiveZoneOverlay && DynMat)
        ActiveZoneOverlay->SetBrushFromMaterial(DynMat);
}
