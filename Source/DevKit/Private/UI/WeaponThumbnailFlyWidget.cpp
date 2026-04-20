#include "UI/WeaponThumbnailFlyWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"

void UWeaponThumbnailFlyWidget::StartFly(UTexture2D* Thumbnail, FVector2D StartAbsPos,
                                          FVector2D EndAbsPos, const UWeaponGlassAnimDA* DA)
{
	if (!Thumbnail || !ThumbnailImage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ThumbnailFly] StartFly 中止 — Thumbnail=%s ThumbnailImage=%s"),
			Thumbnail ? TEXT("OK") : TEXT("NULL"),
			ThumbnailImage ? TEXT("OK") : TEXT("NULL(WBP 未命名 ThumbnailImage)"));
		return;
	}

	CachedThumbnail = Thumbnail;
	FlyStartAbs     = StartAbsPos;
	FlyEndAbs       = EndAbsPos;
	FlyDuration     = DA ? DA->FlyDuration   : 0.45f;
	ImgSize         = DA ? DA->GlassIconSize : FVector2D(64.f, 64.f);
	FlyTimer        = 0.f;
	bFlying         = true;

	ThumbnailImage->SetBrushFromTexture(Thumbnail, true);
	ThumbnailImage->SetRenderOpacity(DA ? DA->ThumbnailFlyOpacity : 0.9f);
	ThumbnailImage->SetVisibility(ESlateVisibility::HitTestInvisible);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ThumbnailImage->Slot))
	{
		CanvasSlot->SetSize(ImgSize);
		CanvasSlot->SetPosition(StartAbsPos - ImgSize * 0.5f);
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[ThumbnailFly] StartFly — Start=(%.0f,%.0f) End=(%.0f,%.0f) Dur=%.2fs ImgSize=(%.0f,%.0f)"),
		StartAbsPos.X, StartAbsPos.Y, EndAbsPos.X, EndAbsPos.Y, FlyDuration, ImgSize.X, ImgSize.Y);
}

void UWeaponThumbnailFlyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bFlying || !ThumbnailImage) return;

	FlyTimer += InDeltaTime;
	const float Alpha = FMath::Clamp(FlyTimer / FMath::Max(FlyDuration, 0.001f), 0.f, 1.f);
	const float Eased = FMath::SmoothStep(0.f, 1.f, Alpha);

	const FVector2D CurPos = FMath::Lerp(FlyStartAbs, FlyEndAbs, Eased);

	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ThumbnailImage->Slot))
		CanvasSlot->SetPosition(CurPos - ImgSize * 0.5f);

	OnFlyProgress.Broadcast(FlyStartAbs, CurPos, Alpha);

	if (Alpha >= 1.f)
	{
		bFlying = false;
		UE_LOG(LogTemp, Warning, TEXT("[ThumbnailFly] Complete → 广播 OnFlyComplete"));
		SetVisibility(ESlateVisibility::Collapsed);
		OnFlyComplete.Broadcast(CachedThumbnail);
		RemoveFromParent();
	}
}
