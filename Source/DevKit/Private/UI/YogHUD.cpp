#include "UI/YogHUD.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/EnemyArrowWidget.h"
#include "UI/GameDialogWidget.h"
#include "UI/DialogContentDA.h"
#include "UI/WeaponFloatWidget.h"
#include "UI/WeaponGlassIconWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "Tutorial/TutorialManager.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"
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

	// ── Weapon Float ────────────────────────────
	if (WeaponFloatClass)
	{
		WeaponFloatWidget = CreateWidget<UWeaponFloatWidget>(
			GetOwningPlayerController(), WeaponFloatClass);
		if (WeaponFloatWidget)
		{
			WeaponFloatWidget->AddToViewport(10);
			WeaponFloatWidget->SetVisibility(ESlateVisibility::Collapsed);
			WeaponFloatWidget->OnFlyComplete.AddDynamic(this, &AYogHUD::OnWeaponFlyComplete);
		}
	}

	// ── Weapon Glass Icon ────────────────────────
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
//  武器拾取流程
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::TriggerWeaponPickup(const UWeaponDefinition* Def)
{
	if (!WeaponFloatWidget || !Def) return;

	// 取消上一次未完成的自动折叠计时
	GetWorld()->GetTimerManager().ClearTimer(CollapseTimerHandle);

	WeaponFloatWidget->SetWeaponDefinition(Def);
	WeaponFloatWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	// 隐藏上次的玻璃图标（换武器）
	if (WeaponGlassIconWidget)
		WeaponGlassIconWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 自动延迟折叠
	if (WeaponGlassAnimDA && WeaponGlassAnimDA->AutoCollapseDelay > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			CollapseTimerHandle, this, &AYogHUD::TriggerWeaponCollapse,
			WeaponGlassAnimDA->AutoCollapseDelay, false);
	}
}

void AYogHUD::TriggerWeaponCollapse()
{
	if (!WeaponFloatWidget || !WeaponGlassAnimDA) return;

	GetWorld()->GetTimerManager().ClearTimer(CollapseTimerHandle);
	WeaponFloatWidget->StartCollapseAndFly(GetWeaponGlassIconScreenCenter(), WeaponGlassAnimDA);
}

void AYogHUD::OnWeaponFlyComplete(UTexture2D* Thumbnail)
{
	if (!WeaponGlassIconWidget || !WeaponGlassAnimDA) return;
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
