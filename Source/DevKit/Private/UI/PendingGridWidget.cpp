#include "UI/PendingGridWidget.h"
#include "UI/BackpackStyleDataAsset.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/SizeBox.h"

static const FLinearColor PendingEmptyColor (0.40f, 0.40f, 0.42f, 1.00f);

// ============================================================
//  槽位创建
// ============================================================

void UPendingGridWidget::BuildSlots()
{
    if (!PendingRuneGrid) return;

    PendingRuneGrid->ClearChildren();
    CachedPendingBGImages.Empty();
    CachedPendingIcons.Empty();

    const float CellSize    = StyleDA ? StyleDA->CellSize         : 64.f;
    const float CellPadding = StyleDA ? StyleDA->CellPadding      : 2.f;
    const float CornerR     = StyleDA ? StyleDA->CellCornerRadius : 3.f;
    const float IconPad     = StyleDA ? StyleDA->IconPadding      : 6.f;

    PendingRuneGrid->SetMinDesiredSlotWidth(CellSize);
    PendingRuneGrid->SetMinDesiredSlotHeight(CellSize);
    PendingRuneGrid->SetSlotPadding(FMargin(CellPadding));

    const int32 Cols       = FMath::Max(1, PendingGridCols);
    const int32 Rows       = FMath::Max(1, PendingGridRows);

    if (PendingGridSizeBox)
    {
        const float SlotTotal = CellSize + CellPadding * 2.f;
        PendingGridSizeBox->SetWidthOverride(Cols * SlotTotal);
        PendingGridSizeBox->SetHeightOverride(Rows * SlotTotal);
    }

    for (int32 i = 0; i < Cols * Rows; i++)
    {
        const int32 Col = i % Cols;
        const int32 Row = i / Cols;

        UOverlay* CellOverlay = NewObject<UOverlay>(this);

        UImage* BGImage = NewObject<UImage>(this);
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
            Brush.TintColor = FSlateColor(FLinearColor::White);
            Brush.OutlineSettings.CornerRadii  = FVector4(CornerR, CornerR, CornerR, CornerR);
            Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            BGImage->SetBrush(Brush);
        }
        BGImage->SetColorAndOpacity(PendingEmptyColor);

        UOverlaySlot* BGSlot = CellOverlay->AddChildToOverlay(BGImage);
        BGSlot->SetHorizontalAlignment(HAlign_Fill);
        BGSlot->SetVerticalAlignment(VAlign_Fill);

        UImage* Icon = NewObject<UImage>(this);
        Icon->SetVisibility(ESlateVisibility::Collapsed);

        UOverlaySlot* IconSlot = CellOverlay->AddChildToOverlay(Icon);
        IconSlot->SetHorizontalAlignment(HAlign_Fill);
        IconSlot->SetVerticalAlignment(VAlign_Fill);
        IconSlot->SetPadding(FMargin(IconPad));

        UUniformGridSlot* GridSlot = PendingRuneGrid->AddChildToUniformGrid(CellOverlay, Row, Col);
        GridSlot->SetHorizontalAlignment(HAlign_Fill);
        GridSlot->SetVerticalAlignment(VAlign_Fill);

        CachedPendingBGImages.Add(BGImage);
        CachedPendingIcons.Add(Icon);
    }

    UE_LOG(LogTemp, Log, TEXT("[PendingGridWidget] 创建 %d×%d 槽位完成"), Cols, Rows);
}

// ============================================================
//  槽位刷新
// ============================================================

void UPendingGridWidget::RefreshSlots(const TArray<FRuneInstance>& PendingRunes)
{
    if (CachedPendingIcons.Num() == 0) return;

    const FLinearColor HasRuneColor = StyleDA ? StyleDA->PendingHasRuneColor
                                              : FLinearColor(0.12f, 0.08f, 0.22f, 1.f);

    for (int32 i = 0; i < CachedPendingIcons.Num(); i++)
    {
        UImage* BG   = CachedPendingBGImages.IsValidIndex(i) ? CachedPendingBGImages[i] : nullptr;
        UImage* Icon = CachedPendingIcons[i];

        if (PendingRunes.IsValidIndex(i))
        {
            if (BG) BG->SetColorAndOpacity(HasRuneColor);

            if (Icon)
            {
                UTexture2D* Tex = PendingRunes[i].RuneConfig.RuneIcon;
                if (Tex)
                {
                    Icon->SetBrushFromTexture(Tex, false);
                    Icon->SetColorAndOpacity(FLinearColor::White);
                    Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
                }
                else
                {
                    Icon->SetVisibility(ESlateVisibility::Collapsed);
                }
            }
        }
        else
        {
            if (BG)   BG->SetColorAndOpacity(PendingEmptyColor);
            if (Icon) Icon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

void UPendingGridWidget::SetGamepadCursor(int32 Index)
{
    const FLinearColor CursorColor = StyleDA ? StyleDA->SelectedColor
                                             : FLinearColor(1.f, 0.85f, 0.1f, 1.f);
    const FLinearColor HasRuneColor = StyleDA ? StyleDA->PendingHasRuneColor
                                              : FLinearColor(0.12f, 0.08f, 0.22f, 1.f);

    for (int32 i = 0; i < CachedPendingBGImages.Num(); i++)
    {
        UImage* BG = CachedPendingBGImages[i];
        if (!BG) continue;

        const bool bHasRune = CachedPendingIcons.IsValidIndex(i)
            && CachedPendingIcons[i]
            && CachedPendingIcons[i]->GetVisibility() != ESlateVisibility::Collapsed;

        if (i == Index)
            BG->SetColorAndOpacity(CursorColor);
        else if (bHasRune)
            BG->SetColorAndOpacity(HasRuneColor);
        else
            BG->SetColorAndOpacity(PendingEmptyColor);
    }
}

// ============================================================
//  坐标辅助
// ============================================================

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
