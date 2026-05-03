#include "UI/DamageEdgeFlashWidget.h"

#include "Brushes/SlateColorBrush.h"
#include "Rendering/DrawElements.h"

void UDamageEdgeFlashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDamageEdgeFlashWidget::PlayEdgeFlash(FLinearColor InColor, float InMaxAlpha, float InDuration, float InEdgeThickness)
{
	FlashColor = InColor;
	MaxAlpha = FMath::Clamp(InMaxAlpha, 0.f, 1.f);
	Duration = FMath::Max(0.01f, InDuration);
	Elapsed = 0.f;
	EdgeThickness = FMath::Max(1.f, InEdgeThickness);
	bActive = MaxAlpha > 0.f;

	SetVisibility(bActive ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	Invalidate(EInvalidateWidgetReason::Paint);
}

void UDamageEdgeFlashWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bActive)
	{
		return;
	}

	Elapsed += InDeltaTime;
	if (Elapsed >= Duration)
	{
		bActive = false;
		SetVisibility(ESlateVisibility::Collapsed);
	}

	Invalidate(EInvalidateWidgetReason::Paint);
}

int32 UDamageEdgeFlashWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	const int32 SuperLayer = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId,
		InWidgetStyle,
		bParentEnabled);

	if (!bActive || Duration <= 0.f || MaxAlpha <= 0.f)
	{
		return SuperLayer;
	}

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (Size.X <= 0.f || Size.Y <= 0.f)
	{
		return SuperLayer;
	}

	const float AlphaScale = 1.f - FMath::Clamp(Elapsed / Duration, 0.f, 1.f);
	FLinearColor OuterColor = FlashColor;
	OuterColor.A *= MaxAlpha * AlphaScale;

	FLinearColor InnerColor = FlashColor;
	InnerColor.A *= MaxAlpha * AlphaScale * 0.38f;

	const int32 PaintLayer = SuperLayer + 1;
	const FSlateColorBrush OuterBrush(OuterColor);
	const FSlateColorBrush InnerBrush(InnerColor);

	auto DrawBox = [&](const FVector2D& Position, const FVector2D& BoxSize, const FSlateBrush* Brush, int32 BoxLayer)
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			BoxLayer,
			AllottedGeometry.ToPaintGeometry(BoxSize, FSlateLayoutTransform(Position)),
			Brush,
			ESlateDrawEffect::None,
			FLinearColor::White);
	};

	const float Outer = FMath::Min(EdgeThickness, FMath::Min(Size.X, Size.Y) * 0.45f);
	const float Inner = FMath::Max(1.f, Outer * 0.45f);

	DrawBox(FVector2D(0.f, 0.f), FVector2D(Size.X, Outer), &OuterBrush, PaintLayer);
	DrawBox(FVector2D(0.f, Size.Y - Outer), FVector2D(Size.X, Outer), &OuterBrush, PaintLayer);
	DrawBox(FVector2D(0.f, 0.f), FVector2D(Outer, Size.Y), &OuterBrush, PaintLayer);
	DrawBox(FVector2D(Size.X - Outer, 0.f), FVector2D(Outer, Size.Y), &OuterBrush, PaintLayer);

	DrawBox(FVector2D(0.f, Outer), FVector2D(Size.X, Inner), &InnerBrush, PaintLayer + 1);
	DrawBox(FVector2D(0.f, Size.Y - Outer - Inner), FVector2D(Size.X, Inner), &InnerBrush, PaintLayer + 1);
	DrawBox(FVector2D(Outer, 0.f), FVector2D(Inner, Size.Y), &InnerBrush, PaintLayer + 1);
	DrawBox(FVector2D(Size.X - Outer - Inner, 0.f), FVector2D(Inner, Size.Y), &InnerBrush, PaintLayer + 1);

	return PaintLayer + 1;
}
