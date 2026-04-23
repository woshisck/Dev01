#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Components/Image.h"

void UWeaponGlassIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// 始终可见：玻璃模糊常驻，热度颜色初始透明
	SetVisibility(ESlateVisibility::HitTestInvisible);
	if (HeatColorOverlay)
		HeatColorOverlay->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
}

void UWeaponGlassIconWidget::Show(const UWeaponGlassAnimDA* InAnimDA)
{
	AnimDA      = InAnimDA;
	bExpanding  = false;
	ExpandTimer = 0.f;
	{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }
	// widget 已经常驻可见，只重置动画状态
}

void UWeaponGlassIconWidget::SetHeatColor(FLinearColor Color)
{
	if (HeatColorOverlay)
	{
		HeatColorOverlay->SetColorAndOpacity(Color);
		HeatColorOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
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
		{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }
		SetRenderOpacity(1.f);
		if (HeatColorOverlay)
			HeatColorOverlay->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.f));
		OnHidden.Broadcast();
	}
}
