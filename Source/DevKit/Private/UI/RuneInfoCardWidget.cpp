#include "UI/RuneInfoCardWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"

// ============================================================
//  点阵颜色常量
// ============================================================

namespace ShapeGridColors
{
    static const FLinearColor Filled (0.20f, 0.60f, 1.00f, 1.0f);  // 亮蓝：已占格
    static const FLinearColor Empty  (0.18f, 0.18f, 0.22f, 0.6f);  // 暗灰：空格
}

// ============================================================
//  显示符文
// ============================================================

void URuneInfoCardWidget::ShowRune(const FRuneInstance& Rune)
{
    SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (CardBG)
    {
        if (Rune.RuneConfig.CardBackground)
        {
            CardBG->SetBrushFromTexture(Rune.RuneConfig.CardBackground, false);
            CardBG->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardBG->SetBrush(FSlateBrush());
            CardBG->SetColorAndOpacity(FLinearColor::Black);
            CardBG->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
    }

    if (CardName)
        CardName->SetText(FText::FromName(Rune.RuneConfig.RuneName));

    if (CardDesc)
        CardDesc->SetText(Rune.RuneConfig.RuneDescription);

    if (CardEffect)
        CardEffect->SetText(Rune.RuneConfig.RuneDescription);

    if (CardIcon)
    {
        if (Rune.RuneConfig.RuneIcon)
        {
            CardIcon->SetBrushFromTexture(Rune.RuneConfig.RuneIcon, false);
            CardIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardIcon->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    if (CardUpgrade)
    {
        if (Rune.UpgradeLevel > 0)
        {
            CardUpgrade->SetText(FText::Format(
                NSLOCTEXT("RuneInfoCard", "LvText", "Lv.{0}"),
                FText::AsNumber(Rune.UpgradeLevel + 1)));
            CardUpgrade->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            CardUpgrade->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    BuildShapeGrid(Rune.Shape);
}

// ============================================================
//  隐藏卡片
// ============================================================

void URuneInfoCardWidget::HideCard()
{
    SetVisibility(ESlateVisibility::Collapsed);
}

// ============================================================
//  点阵生成
// ============================================================

void URuneInfoCardWidget::BuildShapeGrid(const FRuneShape& Shape)
{
    if (!ShapeGrid) return;

    ShapeGrid->ClearChildren();

    if (Shape.Cells.IsEmpty()) return;

    // ── 计算包围盒 ───────────────────────────────────────────────
    int32 MinX = INT_MAX, MaxX = INT_MIN;
    int32 MinY = INT_MAX, MaxY = INT_MIN;
    for (const FIntPoint& Cell : Shape.Cells)
    {
        MinX = FMath::Min(MinX, Cell.X);
        MaxX = FMath::Max(MaxX, Cell.X);
        MinY = FMath::Min(MinY, Cell.Y);
        MaxY = FMath::Max(MaxY, Cell.Y);
    }

    const int32 Cols   = MaxX - MinX + 1;
    const int32 Rows   = MaxY - MinY + 1;
    const int32 MaxDim = FMath::Max(Cols, Rows);

    TSet<FIntPoint> Occupied(Shape.Cells);

    // ── 固定点大小 8px，间隔 2px，居中到 64×64 区域 ────────────
    constexpr float AreaSize = 64.f;
    constexpr float DotSize  = 8.f;
    constexpr float Gap      = 2.f;
    constexpr float StepSize = DotSize + Gap;

    // 整个点阵实际宽高，用于居中
    const float GridW = Cols * StepSize - Gap;
    const float GridH = Rows * StepSize - Gap;
    const float OffX  = (AreaSize - GridW) * 0.5f;
    const float OffY  = (AreaSize - GridH) * 0.5f;

    // ── 逐格创建绝对定位 UImage ─────────────────────────────────
    for (int32 Row = 0; Row < Rows; Row++)
    {
        for (int32 Col = 0; Col < Cols; Col++)
        {
            const FIntPoint AbsCell(Col + MinX, Row + MinY);
            const bool bFilled = Occupied.Contains(AbsCell);

            UImage* Dot = NewObject<UImage>(this);

            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
            Brush.TintColor = FSlateColor(FLinearColor::White);
            Brush.OutlineSettings.CornerRadii  = FVector4(3.f, 3.f, 3.f, 3.f);
            Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
            Dot->SetBrush(Brush);
            Dot->SetColorAndOpacity(bFilled ? ShapeGridColors::Filled : ShapeGridColors::Empty);

            UCanvasPanelSlot* DotSlot = ShapeGrid->AddChildToCanvas(Dot);
            DotSlot->SetAutoSize(false);
            DotSlot->SetPosition(FVector2D(OffX + Col * StepSize, OffY + Row * StepSize));
            DotSlot->SetSize(FVector2D(DotSize, DotSize));
        }
    }
}
