#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Character/PlayerCharacterBase.h"
#include "UI/WidgetReflectorDebugUtils.h"

void UWeaponGlassIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
}

void UWeaponGlassIconWidget::Show(const UWeaponGlassAnimDA* InAnimDA)
{
	AnimDA         = InAnimDA;
	bExpanding     = false;
	ExpandTimer    = 0.f;
	bWeaponShowing = true;
	SetVisibility(YogWidgetReflectorDebug::GetInspectableVisibility(ESlateVisibility::HitTestInvisible));
	SetRenderOpacity(1.f);
	{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }
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
		bExpanding     = false;
		bWeaponShowing = false;
		{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }
		SetRenderOpacity(1.f);
		OnHidden.Broadcast();
	}
}
