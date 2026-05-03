#include "UI/DamageEdgeFlashWidget.h"

#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/CoreStyle.h"

void UDamageEdgeFlashWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

void UDamageEdgeFlashWidget::PlayEdgeFlash(FLinearColor InColor, float InMaxAlpha, float InDuration, float InEdgeWidthRatio)
{
	FlashColor = InColor;
	MaxAlpha = FMath::Clamp(InMaxAlpha, 0.f, 1.f);
	Duration = FMath::Max(0.01f, InDuration);
	Elapsed = 0.f;
	EdgeWidthRatio = FMath::Clamp(InEdgeWidthRatio, 0.01f, 0.45f);
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

	const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
	if (!WhiteBrush)
	{
		return SuperLayer;
	}

	const float AlphaScale = FMath::Square(1.f - FMath::Clamp(Elapsed / Duration, 0.f, 1.f));
	const float EdgeX = FMath::Clamp(Size.X * EdgeWidthRatio, 1.f, Size.X * 0.45f);
	const float EdgeY = FMath::Clamp(Size.Y * EdgeWidthRatio, 1.f, Size.Y * 0.45f);
	const int32 DrawLayer = SuperLayer + 1;

	auto DrawRect = [&](const FVector2D& Position, const FVector2D& RectSize, float Alpha)
	{
		if (RectSize.X <= 0.f || RectSize.Y <= 0.f || Alpha <= 0.f)
		{
			return;
		}

		FLinearColor Tint = FlashColor;
		Tint.A *= Alpha;
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			DrawLayer,
			AllottedGeometry.ToPaintGeometry(RectSize, FSlateLayoutTransform(Position)),
			WhiteBrush,
			ESlateDrawEffect::None,
			Tint);
	};

	DrawRect(FVector2D::ZeroVector, Size, MaxAlpha * AlphaScale * 0.06f);

	constexpr int32 BandCount = 8;
	for (int32 BandIndex = 0; BandIndex < BandCount; ++BandIndex)
	{
		const float InnerT = static_cast<float>(BandIndex) / static_cast<float>(BandCount);
		const float OuterT = static_cast<float>(BandIndex + 1) / static_cast<float>(BandCount);
		const float BandAlpha = MaxAlpha * AlphaScale * FMath::Pow(1.f - InnerT, 1.6f);

		const float LeftA = EdgeX * InnerT;
		const float LeftB = EdgeX * OuterT;
		const float TopA = EdgeY * InnerT;
		const float TopB = EdgeY * OuterT;

		DrawRect(FVector2D(LeftA, 0.f), FVector2D(LeftB - LeftA, Size.Y), BandAlpha);
		DrawRect(FVector2D(Size.X - LeftB, 0.f), FVector2D(LeftB - LeftA, Size.Y), BandAlpha);
		DrawRect(FVector2D(0.f, TopA), FVector2D(Size.X, TopB - TopA), BandAlpha);
		DrawRect(FVector2D(0.f, Size.Y - TopB), FVector2D(Size.X, TopB - TopA), BandAlpha);
	}

	return DrawLayer;
}
