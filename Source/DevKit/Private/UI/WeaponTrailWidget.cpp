#include "UI/WeaponTrailWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Materials/MaterialInstanceDynamic.h"

void UWeaponTrailWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] TrailWidget::NativeConstruct — TrailLine=%s TrailMaterial=%s"),
		TrailLine ? TEXT("OK") : TEXT("NULL(未绑定WBP)"),
		TrailMaterial ? *TrailMaterial->GetName() : TEXT("NULL(未赋值)"));

	if (TrailMaterial && TrailLine)
	{
		TrailDynMat = UMaterialInstanceDynamic::Create(TrailMaterial, this);
		TrailLine->SetBrushFromMaterial(TrailDynMat);
	}

	if (TrailLine)
		TrailLine->SetVisibility(ESlateVisibility::Collapsed);
}

void UWeaponTrailWidget::SetTrailEndpoints(FVector2D Start, FVector2D Current, float Alpha)
{
	if (!TrailLine) return;

	const FVector2D Delta = Current - Start;
	const float Dist = Delta.Size();

	if (Dist < 1.f)
	{
		TrailLine->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TrailLine->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 线段起点放在 Start，尺寸 = (length, lineHeight)
	if (UCanvasPanelSlot* PanelSlot = Cast<UCanvasPanelSlot>(TrailLine->Slot))
	{
		PanelSlot->SetPosition(Start - FVector2D(0.f, LineHeight * 0.5f));
		PanelSlot->SetSize(FVector2D(Dist, LineHeight));
	}

	// 绕左中点旋转指向 Current
	const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
	TrailLine->SetRenderTransformPivot(FVector2D(0.f, 0.5f));
	FWidgetTransform WT;
	WT.Angle = AngleDeg;
	TrailLine->SetRenderTransform(WT);

	// 材质参数
	if (TrailDynMat)
	{
		TrailDynMat->SetScalarParameterValue(TEXT("Alpha"),    Alpha);
		TrailDynMat->SetScalarParameterValue(TEXT("Progress"), Alpha);
	}

	TrailLine->SetRenderOpacity(1.f);
}

void UWeaponTrailWidget::StartFadeOut()
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] TrailWidget::StartFadeOut"));
	bFadingOut = true;
	FadeTimer  = 0.f;
}

void UWeaponTrailWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bFadingOut || !TrailLine) return;

	FadeTimer += InDeltaTime;
	const float Opacity = FMath::Clamp(1.f - FadeTimer / FadeDuration, 0.f, 1.f);
	TrailLine->SetRenderOpacity(Opacity);

	if (FadeTimer >= FadeDuration)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] TrailWidget::淡出完成，从视口移除"));
		RemoveFromParent();
	}
}
