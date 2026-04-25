#include "UI/YogHUD.h"
#include "UI/YogHUDRootWidget.h"
#include "UI/LiquidHealthBarWidget.h"
#include "UI/EnemyArrowWidget.h"
#include "UI/WeaponGlassIconWidget.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/LootSelectionWidget.h"
#include "UI/GameDialogWidget.h"
#include "UI/DialogContentDA.h"
#include "UI/WeaponThumbnailFlyWidget.h"
#include "UI/WeaponGlassAnimDA.h"
#include "UI/WeaponTrailWidget.h"
#include "Character/YogCharacterBase.h"
#include "Character/PlayerCharacterBase.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Tutorial/TutorialManager.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "SaveGame/YogSaveGame.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponInfoDA.h"
#include "Engine/PostProcessVolume.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Data/LevelEndEffectDA.h"
#include "UI/LevelEndRevealWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/InfoPopupWidget.h"
#include "Data/LevelInfoPopupDA.h"

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

	// ── 主 HUD 容器（血条 / 敌人箭头 / 武器图标 等常驻元素）────────
	if (MainHUDClass)
	{
		MainHUDWidget = CreateWidget<UYogHUDRootWidget>(
			GetOwningPlayerController(), MainHUDClass);
		if (MainHUDWidget)
			MainHUDWidget->AddToViewport(1);
	}

	// Attribute 绑定：若 Pawn 已就绪则立即绑；否则等 OnPossessedPawnChanged
	if (APawn* Pawn = GetOwningPawn())
	{
		BindHealthAttributes(Pawn);
	}
	else if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->OnPossessedPawnChanged.AddDynamic(this, &AYogHUD::OnPawnPossessed);
	}

	// ── Backpack ─────────────────────────────────
	if (BackpackScreenClass)
	{
		BackpackWidget = CreateWidget<UBackpackScreenWidget>(
			GetOwningPlayerController(), BackpackScreenClass);
		if (BackpackWidget)
			BackpackWidget->AddToViewport(5);
	}

	// ── Loot Selection（常驻，不走 CommonUI Stack，避免 DeactivateWidget 时被 destroy）
	if (LootSelectionWidgetClass)
	{
		LootSelectionWidget = CreateWidget<ULootSelectionWidget>(
			GetOwningPlayerController(), LootSelectionWidgetClass);
		if (LootSelectionWidget)
			LootSelectionWidget->AddToViewport(15);
	}

	// ── Weapon Thumbnail Fly（按需回退加载） ──────
	if (!ThumbnailFlyClass)
	{
		ThumbnailFlyClass = LoadClass<UWeaponThumbnailFlyWidget>(
			nullptr, TEXT("/Game/UI/Playtest_UI/WeaponInfo/WBP_WeaponThumbnailFly.WBP_WeaponThumbnailFly_C"));
		if (!ThumbnailFlyClass)
			UE_LOG(LogTemp, Warning, TEXT("[YogHUD] WBP_WeaponThumbnailFly 未找到，请在 BP_YogHUD 手动赋值"));
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

void AYogHUD::ShowLootSelectionUI(const TArray<FLootOption>& Options)
{
	// Widget 可能被 CommonUI 框架销毁，重建后重新加入 Viewport
	if (!LootSelectionWidget || !LootSelectionWidget->IsInViewport())
	{
		if (LootSelectionWidgetClass)
		{
			LootSelectionWidget = CreateWidget<ULootSelectionWidget>(
				GetOwningPlayerController(), LootSelectionWidgetClass);
			if (LootSelectionWidget)
				LootSelectionWidget->AddToViewport(15);
		}
	}
	if (LootSelectionWidget)
		LootSelectionWidget->ShowLootUI(Options);
}

// ─────────────────────────────────────────────────────────────────────────────
//  信息提示浮窗
// ─────────────────────────────────────────────────────────────────────────────

UInfoPopupWidget* AYogHUD::GetInfoPopupWidget() const
{
	return MainHUDWidget ? MainHUDWidget->InfoPopup : nullptr;
}

void AYogHUD::ShowInfoPopup(const ULevelInfoPopupDA* DA)
{
	if (UInfoPopupWidget* W = GetInfoPopupWidget())
		W->Show(DA);
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
	UWeaponGlassIconWidget* GlassIcon = MainHUDWidget ? MainHUDWidget->WeaponGlassIcon : nullptr;
	if (GlassIcon)
		GlassIcon->SetVisibility(ESlateVisibility::Collapsed);

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
	if (ActiveTrailWidget)
	{
		ActiveTrailWidget->StartFadeOut();
		ActiveTrailWidget = nullptr;
	}

	UWeaponGlassIconWidget* GlassIcon = MainHUDWidget ? MainHUDWidget->WeaponGlassIcon : nullptr;
	if (!GlassIcon || !WeaponGlassAnimDA) return;

	bHasWeapon = true;
	GlassIcon->Show(WeaponGlassAnimDA);
}

void AYogHUD::NotifyBackpackOpening()
{
	UWeaponGlassIconWidget* GlassIcon = MainHUDWidget ? MainHUDWidget->WeaponGlassIcon : nullptr;
	if (GlassIcon &&
	    GlassIcon->GetVisibility() != ESlateVisibility::Collapsed)
	{
		GlassIcon->StartExpandAndHide();
	}
}

FVector2D AYogHUD::GetWeaponGlassIconScreenCenter() const
{
	FVector2D ViewSize(1920.f, 1080.f);
	if (GetWorld() && GetWorld()->GetGameViewport())
		GetWorld()->GetGameViewport()->GetViewportSize(ViewSize);
	const float DPI = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
	if (DPI > 0.f) ViewSize /= DPI;
	if (!WeaponGlassAnimDA) return ViewSize * FVector2D(0.1f, 0.8f);
	const FVector2D& Off  = WeaponGlassAnimDA->HUDOffsetFromBottomLeft;
	const FVector2D  Size = WeaponGlassAnimDA->GlassIconSize;
	return FVector2D(Off.X + Size.X * 0.5f, ViewSize.Y - Off.Y - Size.Y * 0.5f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  暂停遮罩 Tick
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 关卡结束特效完全接管 PausePPVolume，与暂停菜单系统互不干扰
	if (bLevelEndEffectActive)
	{
		TickLevelEndEffect();
		return;
	}

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

// ─────────────────────────────────────────────────────────────────────────────
//  关卡结束视觉特效
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::TriggerLevelEndEffect(FVector LootWorldPos)
{
	if (bLevelEndEffectActive || !LevelEndEffectDA) return;

	bLevelEndEffectActive       = true;
	bSlowMoPhaseEnded           = false;
	LevelEndEffectStartRealTime = GetWorld()->GetRealTimeSeconds();
	CachedLootWorldPos          = LootWorldPos;

	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), LevelEndEffectDA->SlowMoScale);

	// 直接接管 PausePPVolume，确保从当前状态开始（不叠加暂停计数）
	if (PausePPVolume)
	{
		PausePPVolume->Settings.bOverride_ColorSaturation = true;
		PausePPVolume->Settings.bOverride_ColorGain       = true;
		PausePPVolume->Settings.ColorSaturation           = FVector4(1, 1, 1, 1);
		PausePPVolume->Settings.ColorGain                 = FVector4(1, 1, 1, 1);
		PausePPVolume->BlendWeight                        = 0.f;
	}
}

void AYogHUD::TickLevelEndEffect()
{
	const ULevelEndEffectDA* DA = LevelEndEffectDA.Get();
	if (!DA || !PausePPVolume)
	{
		bLevelEndEffectActive = false;
		return;
	}

	const float RealElapsed = GetWorld()->GetRealTimeSeconds() - LevelEndEffectStartRealTime;

	// ── 阶段 1：变黑渐入 ─────────────────────────────────────────────────
	const float BlackAlpha = FMath::Clamp(
		RealElapsed / FMath::Max(DA->BlackoutFadeDuration, 0.01f), 0.f, 1.f);
	const float Sat  = FMath::Lerp(1.f, DA->BlackoutSaturation, BlackAlpha);
	const float Gain = FMath::Lerp(1.f, DA->BlackoutGain,       BlackAlpha);
	PausePPVolume->Settings.ColorSaturation = FVector4(1, 1, 1, Sat);
	PausePPVolume->Settings.ColorGain       = FVector4(1, 1, 1, Gain);
	PausePPVolume->BlendWeight              = 1.f;

	// ── 阶段 2/3：恢复时速 + 启动揭幕 ──────────────────────────────────
	if (!bSlowMoPhaseEnded && RealElapsed >= DA->SlowMoDuration)
	{
		bSlowMoPhaseEnded = true;
		UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
		StartRevealAnimation();
	}

	// ── 阶段 3：驱动圆形扩散 ────────────────────────────────────────────
	if (bSlowMoPhaseEnded)
	{
		const float RevealElapsed  = RealElapsed - DA->SlowMoDuration;
		const float RevealProgress = FMath::Clamp(
			RevealElapsed / FMath::Max(DA->RevealDuration, 0.01f), 0.f, 1.f);

		if (RevealDynMat)
			RevealDynMat->SetScalarParameterValue(TEXT("RevealProgress"), RevealProgress);

		if (RevealProgress >= 1.f)
		{
			// 揭幕完成：隐藏后处理、移除遮罩
			PausePPVolume->BlendWeight = 0.f;
			if (ActiveRevealWidget)
			{
				ActiveRevealWidget->RemoveFromParent();
				ActiveRevealWidget = nullptr;
			}
			RevealDynMat          = nullptr;
			bLevelEndEffectActive = false;
		}
	}
}

void AYogHUD::StartRevealAnimation()
{
	const ULevelEndEffectDA* DA = LevelEndEffectDA.Get();
	if (!DA || !DA->RevealMaterial || !LevelEndRevealWidgetClass) return;

	ActiveRevealWidget = CreateWidget<ULevelEndRevealWidget>(
		GetOwningPlayerController(), LevelEndRevealWidgetClass);
	if (!ActiveRevealWidget) return;
	ActiveRevealWidget->AddToViewport(20);

	// 将 Loot 世界坐标投影到屏幕 UV（0~1 范围）
	FVector2D LootUV(0.5f, 0.5f);  // 默认屏幕中心
	if (APlayerController* PC = GetOwningPlayerController())
	{
		FVector2D LootScreenPos;
		if (PC->ProjectWorldLocationToScreen(CachedLootWorldPos, LootScreenPos))
		{
			FVector2D ViewportSize(1920.f, 1080.f);
			if (GEngine && GEngine->GameViewport)
				GEngine->GameViewport->GetViewportSize(ViewportSize);
			const float DPI = UWidgetLayoutLibrary::GetViewportScale(GetWorld());
			if (DPI > 0.f)
			{
				LootScreenPos /= DPI;
				ViewportSize  /= DPI;
			}
			if (ViewportSize.X > 0.f && ViewportSize.Y > 0.f)
				LootUV = FVector2D(LootScreenPos.X / ViewportSize.X,
				                   LootScreenPos.Y / ViewportSize.Y);
		}
	}

	RevealDynMat = ActiveRevealWidget->InitReveal(
		DA->RevealMaterial, LootUV, DA->RevealEdgeSharpness);
}

void AYogHUD::OnSaveGameLoaded(UYogSaveGame* SaveGame)
{
	if (UTutorialManager* TM = GetGameInstance()->GetSubsystem<UTutorialManager>())
		TM->LoadFromSave(SaveGame);
}

// ─────────────────────────────────────────────────────────────────────────────
//  液态血条
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::BindHealthAttributes(APawn* Pawn)
{
	UE_LOG(LogTemp, Warning, TEXT("[HealthBar] BindHealthAttributes — Pawn=%s"),
		Pawn ? *Pawn->GetName() : TEXT("NULL"));

	AYogCharacterBase* Char = Cast<AYogCharacterBase>(Pawn);
	if (!Char)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthBar] Cast<AYogCharacterBase> 失败"));
		return;
	}

	UAbilitySystemComponent* ASC = Char->GetAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HealthBar] GetAbilitySystemComponent() 返回 null"));
		return;
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AYogHUD::OnHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &AYogHUD::OnMaxHealthChanged);

	const float MaxHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
	const float CurHP = ASC->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
	UE_LOG(LogTemp, Warning, TEXT("[HealthBar] 绑定成功 — HP=%.1f / MaxHP=%.1f — Widget=%s"),
		CurHP, MaxHP,
		(MainHUDWidget && MainHUDWidget->PlayerHealthBar) ? TEXT("OK") : TEXT("NULL"));

	if (MainHUDWidget && MainHUDWidget->PlayerHealthBar && MaxHP > 0.f)
		MainHUDWidget->PlayerHealthBar->SetHealthPercent(CurHP / MaxHP);

	if (bHasWeapon)
	{
		if (UWeaponGlassIconWidget* GlassIcon = MainHUDWidget ? MainHUDWidget->WeaponGlassIcon : nullptr)
			GlassIcon->Show(WeaponGlassAnimDA);
	}
}

void AYogHUD::OnPawnPossessed(APawn* OldPawn, APawn* NewPawn)
{
	UE_LOG(LogTemp, Warning, TEXT("[HealthBar] OnPawnPossessed — New=%s"),
		NewPawn ? *NewPawn->GetName() : TEXT("NULL"));
	if (NewPawn)
		BindHealthAttributes(NewPawn);
}

void AYogHUD::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!MainHUDWidget || !MainHUDWidget->PlayerHealthBar) return;
	if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(GetOwningPawn()))
	{
		const float MaxHP = Char->GetAbilitySystemComponent()
			->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		const float Pct = (MaxHP > 0.f) ? (Data.NewValue / MaxHP) : 0.f;
		if (MaxHP > 0.f)
			MainHUDWidget->PlayerHealthBar->SetHealthPercent(Pct);
	}
}

void AYogHUD::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (!MainHUDWidget || !MainHUDWidget->PlayerHealthBar || Data.NewValue <= 0.f) return;
	if (AYogCharacterBase* Char = Cast<AYogCharacterBase>(GetOwningPawn()))
	{
		const float CurHP = Char->GetAbilitySystemComponent()
			->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		UE_LOG(LogTemp, Warning, TEXT("[HealthBar] OnMaxHealthChanged — MaxHP=%.1f CurHP=%.1f"),
			Data.NewValue, CurHP);
		MainHUDWidget->PlayerHealthBar->SetHealthPercent(CurHP / Data.NewValue);
	}
}

