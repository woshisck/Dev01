#include "UI/YogHUD.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/EnemyArrowWidget.h"
#include "UI/GameDialogWidget.h"
#include "UI/DialogContentDA.h"
#include "UI/WeaponThumbnailFlyWidget.h"
#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "UI/WeaponTrailWidget.h"
#include "Tutorial/TutorialManager.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Engine/PostProcessVolume.h"

void AYogHUD::BeginPlay()
{
	Super::BeginPlay();

	PrimaryActorTick.bCanEverTick       = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;

	// ── Tutorial Popup ──────────────────────────
	if (TutorialPopupClass)
		TutorialPopupWidget = CreateWidget<UTutorialPopupWidget>(
			GetOwningPlayerController(), TutorialPopupClass);

	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
		TM->Init(TutorialPopupWidget, DialogContentDA);

	// ── Enemy Arrow ─────────────────────────────
	if (EnemyArrowWidgetClass)
	{
		EnemyArrowWidget = CreateWidget<UEnemyArrowWidget>(
			GetOwningPlayerController(), EnemyArrowWidgetClass);
		if (EnemyArrowWidget)
			EnemyArrowWidget->AddToViewport(0);
	}

	// ── Save Game ───────────────────────────────
	if (UYogSaveSubsystem* SaveSys = GetGameInstance()->GetSubsystem<UYogSaveSubsystem>())
	{
		SaveSys->OnSaveGameLoaded.AddDynamic(this, &AYogHUD::OnSaveGameLoaded);
		if (UYogSaveGame* Current = SaveSys->GetCurrentSave())
			OnSaveGameLoaded(Current);
	}

	// ── Pause PostProcess Volume ─────────────────
	FActorSpawnParameters Params;
	Params.Owner = this;
	PausePPVolume = GetWorld()->SpawnActor<APostProcessVolume>(
		APostProcessVolume::StaticClass(), FTransform::Identity, Params);
	if (PausePPVolume)
	{
		PausePPVolume->bUnbound = true;
		PausePPVolume->BlendWeight = 0.f;
		PausePPVolume->Settings.bOverride_ColorSaturation = true;
		PausePPVolume->Settings.bOverride_ColorGain       = true;
		PausePPVolume->Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, 1.f);
		PausePPVolume->Settings.ColorGain       = FVector4(1.f, 1.f, 1.f, 1.f);
	}

	// ── Backpack ─────────────────────────────────
	if (BackpackScreenClass)
	{
		BackpackWidget = CreateWidget<UBackpackScreenWidget>(
			GetOwningPlayerController(), BackpackScreenClass);
		if (BackpackWidget)
			BackpackWidget->AddToViewport(5);
	}

	// ── Weapon Glass Icon（常驻左下角） ──────────
	if (WeaponGlassIconClass)
	{
		WeaponGlassIconWidget = CreateWidget<UWeaponGlassIconWidget>(
			GetOwningPlayerController(), WeaponGlassIconClass);
		if (WeaponGlassIconWidget)
		{
			WeaponGlassIconWidget->AddToViewport(10);
			WeaponGlassIconWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  背包
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::OpenBackpack()
{
	if (BackpackWidget)
		BackpackWidget->ActivateWidget();
}

// ─────────────────────────────────────────────────────────────────────────────
//  武器拾取：缩略图飞行
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::TriggerWeaponPickup(const UWeaponDefinition* Def, FVector2D StartScreenPos)
{
	if (!Def || !ThumbnailFlyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] TriggerWeaponPickup 中止 — Def=%s ThumbnailFlyClass=%s"),
			Def ? TEXT("OK") : TEXT("NULL"),
			ThumbnailFlyClass ? TEXT("OK") : TEXT("NULL(BP_YogHUD 未赋值 ThumbnailFlyClass)"));
		return;
	}

	UTexture2D* Thumbnail = (Def->WeaponInfo) ? Def->WeaponInfo->Thumbnail : nullptr;
	UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] TriggerWeaponPickup — Def=%s Thumbnail=%s Start=(%.0f,%.0f)"),
		*Def->GetName(),
		Thumbnail ? *Thumbnail->GetName() : TEXT("NULL"),
		StartScreenPos.X, StartScreenPos.Y);

	// 清理上次残留的飞行 Widget
	if (ActiveTrailWidget)
	{
		ActiveTrailWidget->RemoveFromParent();
		ActiveTrailWidget = nullptr;
	}

	// 隐藏旧玻璃图标（换武器时）
	if (WeaponGlassIconWidget)
		WeaponGlassIconWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 创建缩略图飞行 Widget（每次拾取创建一个新实例，动画结束自行销毁）
	UWeaponThumbnailFlyWidget* FlyWidget = CreateWidget<UWeaponThumbnailFlyWidget>(
		GetOwningPlayerController(), ThumbnailFlyClass);
	if (!FlyWidget) return;

	FlyWidget->AddToViewport(10);

	// 创建流光拖尾
	if (TrailWidgetClass)
	{
		ActiveTrailWidget = CreateWidget<UWeaponTrailWidget>(
			GetOwningPlayerController(), TrailWidgetClass);
		if (ActiveTrailWidget)
		{
			ActiveTrailWidget->AddToViewport(9);
			FlyWidget->OnFlyProgress.AddUObject(this, &AYogHUD::OnFlyProgressUpdate);
		}
	}

	FlyWidget->OnFlyComplete.AddDynamic(this, &AYogHUD::OnWeaponFlyComplete);
	FlyWidget->StartFly(Thumbnail, StartScreenPos, GetWeaponGlassIconScreenCenter(), WeaponGlassAnimDA);
}

void AYogHUD::OnFlyProgressUpdate(FVector2D FlyStart, FVector2D CurrentPos, float Alpha)
{
	if (Alpha < 0.01f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] FlyProgress 首帧 — Start=(%.0f,%.0f) Cur=(%.0f,%.0f)"),
			FlyStart.X, FlyStart.Y, CurrentPos.X, CurrentPos.Y);
	}
	if (ActiveTrailWidget)
		ActiveTrailWidget->SetTrailEndpoints(FlyStart, CurrentPos, Alpha);
}

void AYogHUD::OnWeaponFlyComplete(UTexture2D* Thumbnail)
{
	UE_LOG(LogTemp, Warning, TEXT("[WeaponPickup] OnWeaponFlyComplete — Thumbnail=%s"),
		Thumbnail ? *Thumbnail->GetName() : TEXT("NULL"));

	if (ActiveTrailWidget)
	{
		ActiveTrailWidget->StartFadeOut();
		ActiveTrailWidget = nullptr;
	}

	if (!WeaponGlassIconWidget || !WeaponGlassAnimDA) return;

	// 在显示前设置尺寸和位置，防止 BackgroundBlur 铺满全屏
	const FVector2D Size = WeaponGlassAnimDA->GlassIconSize;
	WeaponGlassIconWidget->SetDesiredSizeInViewport(Size);

	FVector2D ViewSize = FVector2D::ZeroVector;
	if (GetWorld() && GetWorld()->GetGameViewport())
		GetWorld()->GetGameViewport()->GetViewportSize(ViewSize);
	const FVector2D& Off = WeaponGlassAnimDA->HUDOffsetFromBottomLeft;
	WeaponGlassIconWidget->SetPositionInViewport(
		FVector2D(Off.X, ViewSize.Y - Off.Y - Size.Y), false);

	WeaponGlassIconWidget->ShowForWeapon(Thumbnail, WeaponGlassAnimDA);
}

void AYogHUD::NotifyBackpackOpening()
{
	if (WeaponGlassIconWidget &&
	    WeaponGlassIconWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		WeaponGlassIconWidget->StartExpandAndHide();
	}
}

FVector2D AYogHUD::GetWeaponGlassIconScreenCenter() const
{
	FVector2D ViewSize = FVector2D::ZeroVector;
	if (GetWorld() && GetWorld()->GetGameViewport())
		GetWorld()->GetGameViewport()->GetViewportSize(ViewSize);

	if (!WeaponGlassAnimDA) return ViewSize * FVector2D(0.1f, 0.8f);

	const FVector2D& Off  = WeaponGlassAnimDA->HUDOffsetFromBottomLeft;
	const FVector2D  Size = WeaponGlassAnimDA->GlassIconSize;
	return FVector2D(Off.X + Size.X * 0.5f,
	                 ViewSize.Y - Off.Y - Size.Y * 0.5f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  暂停遮罩 Tick
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!PausePPVolume) return;

	const float TargetAlpha = (PausePopupCount > 0) ? 1.f : 0.f;
	if (FMath::IsNearlyEqual(PauseEffectAlpha, TargetAlpha, 0.001f))
	{
		PauseEffectAlpha = TargetAlpha;
		return;
	}

	const float Step = (PauseFadeDuration > 0.f) ? (DeltaSeconds / PauseFadeDuration) : 1.f;
	PauseEffectAlpha = FMath::Clamp(
		PauseEffectAlpha + (TargetAlpha > PauseEffectAlpha ? Step : -Step), 0.f, 1.f);

	const float Sat  = FMath::Lerp(1.f, PauseTargetSaturation, PauseEffectAlpha);
	const float Gain = FMath::Lerp(1.f, PauseTargetGain,       PauseEffectAlpha);

	PausePPVolume->Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, Sat);
	PausePPVolume->Settings.ColorGain       = FVector4(1.f, 1.f, 1.f, Gain);
	PausePPVolume->BlendWeight = 1.f;
}

void AYogHUD::BeginPauseEffect()
{
	PausePopupCount = FMath::Max(0, PausePopupCount) + 1;
}

void AYogHUD::EndPauseEffect()
{
	PausePopupCount = FMath::Max(0, PausePopupCount - 1);
}

void AYogHUD::OnSaveGameLoaded(UYogSaveGame* SaveGame)
{
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
		TM->LoadFromSave(SaveGame);
}
