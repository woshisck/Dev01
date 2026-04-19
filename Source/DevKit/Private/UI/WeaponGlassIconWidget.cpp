#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Components/Image.h"

void UWeaponGlassIconWidget::ShowForWeapon(UTexture2D* Thumbnail, const UWeaponGlassAnimDA* InAnimDA)
{
	AnimDA      = InAnimDA;
	bExpanding  = false;
	ExpandTimer = 0.f;

	{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }
	SetRenderOpacity(1.f);

	if (WeaponThumbnailImg)
	{
		if (Thumbnail)
		{
			WeaponThumbnailImg->SetBrushFromTexture(Thumbnail, true);
			WeaponThumbnailImg->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			WeaponThumbnailImg->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWeaponGlassIconWidget::SetHeatColor(FLinearColor Color)
{
	if (HeatColorOverlay)
		HeatColorOverlay->SetColorAndOpacity(Color);
}

void UWeaponGlassIconWidget::StartExpandAndHide()
{
	bExpanding  = true;
	ExpandTimer = 0.f;
}

void UWeaponGlassIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bExpanding || !AnimDA) return;

	ExpandTimer += InDeltaTime;
	const float Duration = FMath::Max(AnimDA->ExpandDuration, 0.001f);
	const float Alpha    = FMath::Clamp(ExpandTimer / Duration, 0.f, 1.f);

	{ FWidgetTransform T; T.Scale = FVector2D(FMath::Lerp(1.f, AnimDA->ExpandScale, Alpha)); SetRenderTransform(T); }
	SetRenderOpacity(FMath::Lerp(1.f, 0.f, Alpha));

	if (Alpha >= 1.f)
	{
		bExpanding = false;
		SetVisibility(ESlateVisibility::Collapsed);
		{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }
		SetRenderOpacity(1.f);
		OnHidden.Broadcast();
	}
}
