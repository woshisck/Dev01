#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Components/Image.h"
#include "Character/PlayerCharacterBase.h"
#include "UI/BackpackStyleDataAsset.h"

void UWeaponGlassIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 订阅热度阶段变化，并立即同步当前相位
	if (APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
	{
		PC->OnHeatPhaseChanged.RemoveDynamic(this, &UWeaponGlassIconWidget::OnHeatPhaseChanged);
		PC->OnHeatPhaseChanged.AddDynamic(this, &UWeaponGlassIconWidget::OnHeatPhaseChanged);
		RefreshHeatOverlay(PC->GetCurrentHeatPhase());
	}
	else
	{
		RefreshHeatOverlay(0);
	}
}

void UWeaponGlassIconWidget::Show(const UWeaponGlassAnimDA* InAnimDA)
{
	AnimDA         = InAnimDA;
	bExpanding     = false;
	ExpandTimer    = 0.f;
	bWeaponShowing = true;
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetRenderOpacity(1.f);
	{ FWidgetTransform T; T.Scale = FVector2D::UnitVector; SetRenderTransform(T); }

	// 拾武器时立即刷新颜色（防止 NativeConstruct 时 PC 尚未 possess）
	if (APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetOwningPlayerPawn()))
		RefreshHeatOverlay(PC->GetCurrentHeatPhase());
}

void UWeaponGlassIconWidget::StartExpandAndHide()
{
	bExpanding  = true;
	ExpandTimer = 0.f;
}

void UWeaponGlassIconWidget::OnHeatPhaseChanged(int32 Phase)
{
	RefreshHeatOverlay(Phase);
}

void UWeaponGlassIconWidget::RefreshHeatOverlay(int32 Phase)
{
	if (!HeatColorOverlay) return;

	static const FLinearColor BaseColor(0.04f, 0.06f, 0.15f, 0.35f);

	FLinearColor Color = BaseColor;
	if (bWeaponShowing)
	{
		const APlayerCharacterBase* PC = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
		const UBackpackStyleDataAsset* StyleDA = PC ? PC->HeatStyleDA.Get() : nullptr;
		if (StyleDA)
		{
			switch (Phase)
			{
				case 1: Color = StyleDA->HeatZone0Color; break;
				case 2: Color = StyleDA->HeatZone1Color; break;
				case 3: Color = StyleDA->HeatZone2Color; break;
				default: Color = BaseColor; break;
			}
		}
	}

	HeatColorOverlay->SetColorAndOpacity(Color);
	HeatColorOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
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
		RefreshHeatOverlay(0);
		OnHidden.Broadcast();
	}
}
