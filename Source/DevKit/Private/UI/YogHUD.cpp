#include "UI/YogHUD.h"
#include "UI/YogHUDRootWidget.h"
#include "UI/LiquidHealthBarWidget.h"
#include "UI/EnemyArrowWidget.h"
#include "UI/WeaponGlassIconWidget.h"
#include "UI/BackpackScreenWidget.h"
#include "UI/LootSelectionWidget.h"
#include "UI/GameDialogWidget.h"
#include "UI/TutorialRegistryDA.h"
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
#include "UI/PortalPreviewWidget.h"
#include "UI/PortalDirectionWidget.h"
#include "Map/RewardPickup.h"
#include "Map/Portal.h"
#include "Map/SacrificeGracePickup.h"
#include "UI/SacrificeGraceOptionWidget.h"
#include "Data/SacrificeGraceDA.h"
#include "System/YogGameInstanceBase.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/Pawn.h"

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
		TM->Init(TutorialPopupWidget, TutorialRegistry);

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
		{
			BackpackWidget->AddToViewport(5);
			BackpackWidget->OnDeactivated().AddLambda([this](){ PopMajorUI(); });
		}
	}

	// ── Loot Selection（常驻，不走 CommonUI Stack，避免 DeactivateWidget 时被 destroy）
	if (LootSelectionWidgetClass)
	{
		LootSelectionWidget = CreateWidget<ULootSelectionWidget>(
			GetOwningPlayerController(), LootSelectionWidgetClass);
		if (LootSelectionWidget)
			LootSelectionWidget->AddToViewport(15);
	}

	// ── SacrificeGrace 确认弹窗（常驻，默认隐藏，按需激活）──────────
	if (SacrificeGraceOptionWidgetClass)
	{
		SacrificeGraceOptionWidget = CreateWidget<USacrificeGraceOptionWidget>(
			GetOwningPlayerController(), SacrificeGraceOptionWidgetClass);
		if (SacrificeGraceOptionWidget)
			SacrificeGraceOptionWidget->AddToViewport(16);
	}

	// ── Weapon Thumbnail Fly（按需回退加载） ──────
	if (!ThumbnailFlyClass)
	{
		ThumbnailFlyClass = LoadClass<UWeaponThumbnailFlyWidget>(
			nullptr, TEXT("/Game/UI/Playtest_UI/WeaponInfo/WBP_WeaponThumbnailFly.WBP_WeaponThumbnailFly_C"));
		if (!ThumbnailFlyClass)
			UE_LOG(LogTemp, Warning, TEXT("[YogHUD] WBP_WeaponThumbnailFly 未找到，请在 BP_YogHUD 手动赋值"));
	}

	// ── Portal 进入过场 Blackout PostProcess（独立 Volume，不与 Pause/LevelEnd 互扰）──
	{
		FActorSpawnParameters BParams;
		BParams.Owner = this;
		BlackoutPPVolume = GetWorld()->SpawnActor<APostProcessVolume>(
			APostProcessVolume::StaticClass(), FTransform::Identity, BParams);
		if (BlackoutPPVolume)
		{
			BlackoutPPVolume->bUnbound = true;
			BlackoutPPVolume->BlendWeight = 0.f;
			BlackoutPPVolume->Settings.bOverride_ColorSaturation = true;
			BlackoutPPVolume->Settings.bOverride_ColorGain       = true;
			BlackoutPPVolume->Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, 1.f);
			BlackoutPPVolume->Settings.ColorGain       = FVector4(1.f, 1.f, 1.f, 1.f);
		}
	}

	// ── v3 跨关淡入：检测 GI 标志，立即贴 Blackout 后线性反向插回 ──
	if (UYogGameInstanceBase* GI = Cast<UYogGameInstanceBase>(GetGameInstance()))
	{
		if (GI->bPlayLevelIntroFadeIn)
		{
			GI->bPlayLevelIntroFadeIn = false;   // 立即清，避免重复触发
			BlackoutAlpha = 1.f;
			ApplyBlackoutPP();
			EndBlackoutFade(PortalBlackoutDuration);
			UE_LOG(LogTemp, Log, TEXT("[Portal] 触发下一关 fade-in（线性 PostProcess 反向插值）"));
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  背包
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::OpenBackpack()
{
	PushMajorUI();
	if (BackpackWidget)
		BackpackWidget->ActivateWidget();
}

bool AYogHUD::CloseTopMostOverlay()
{
	if (TutorialPopupWidget && TutorialPopupWidget->IsActivated())
	{
		TutorialPopupWidget->DeactivateWidget();
		return true;
	}

	if (SacrificeGraceOptionWidget && SacrificeGraceOptionWidget->IsActivated())
	{
		SacrificeGraceOptionWidget->CancelChoice();
		return true;
	}

	if (BackpackWidget && BackpackWidget->IsActivated())
	{
		BackpackWidget->DeactivateWidget();
		return true;
	}

	if (LootSelectionWidget && LootSelectionWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		LootSelectionWidget->SkipSelection();
		return true;
	}

	return false;
}

void AYogHUD::ShowLootSelectionUI(const TArray<FLootOption>& Options)
{
	// 兼容旧路径（无 SourcePickup）：转发到队列入口
	QueueLootSelection(Options, nullptr);
}

void AYogHUD::QueueLootSelection(const TArray<FLootOption>& Options, ARewardPickup* SourcePickup)
{
	// 已有活跃选择 → 入队，等当前选完再弹
	if (bLootSelectionActive)
	{
		FQueuedLootRequest Req;
		Req.Options = Options;
		Req.SourcePickup = SourcePickup;
		LootQueue.Add(MoveTemp(Req));
		UE_LOG(LogTemp, Log, TEXT("[HUD] QueueLootSelection: 已有活跃选择，本次入队（队列长度=%d）"), LootQueue.Num());
		return;
	}

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
	{
		bLootSelectionActive = true;
		PushMajorUI();
		LootSelectionWidget->ShowLootUI(Options, SourcePickup);
	}
}

void AYogHUD::OnLootSelectionFinished()
{
	bLootSelectionActive = false;
	PopMajorUI();

	if (LootQueue.Num() == 0) return;

	FQueuedLootRequest Next = MoveTemp(LootQueue[0]);
	LootQueue.RemoveAt(0);
	UE_LOG(LogTemp, Log, TEXT("[HUD] OnLootSelectionFinished: 弹出队列下一项（剩余=%d）"), LootQueue.Num());

	// 复用 QueueLootSelection 流程（此时 bLootSelectionActive 已置 false，会立即弹）
	QueueLootSelection(Next.Options, Next.SourcePickup.Get());
}

void AYogHUD::ShowSacrificeGraceOption(USacrificeGraceDA* DA, APlayerCharacterBase* Player, ASacrificeGracePickup* Pickup)
{
	if (!DA) return;

	// 按需重建（Widget 被 CommonUI 框架销毁时）
	if (!SacrificeGraceOptionWidget || !SacrificeGraceOptionWidget->IsInViewport())
	{
		if (!SacrificeGraceOptionWidgetClass) return;
		SacrificeGraceOptionWidget = CreateWidget<USacrificeGraceOptionWidget>(
			GetOwningPlayerController(), SacrificeGraceOptionWidgetClass);
		if (SacrificeGraceOptionWidget)
			SacrificeGraceOptionWidget->AddToViewport(16);
	}

	SacrificeGraceOptionWidget->Setup(DA, Player, Pickup);

	// 每次（含重建后）重新注册 MajorUI 关闭监听，避免重复注册
	if (SacrificeGraceMajorUIHandle.IsValid())
	{
		SacrificeGraceOptionWidget->OnDeactivated().Remove(SacrificeGraceMajorUIHandle);
		SacrificeGraceMajorUIHandle.Reset();
	}
	SacrificeGraceMajorUIHandle = SacrificeGraceOptionWidget->OnDeactivated()
		.AddLambda([this](){ PopMajorUI(); });

	PushMajorUI();
	SacrificeGraceOptionWidget->ActivateWidget();
}

void AYogHUD::OpenBackpackForPreview(FSimpleDelegate OnClosed)
{
	if (!BackpackWidget) return;

	// 切到只读模式
	BackpackWidget->SetPreviewMode(true);

	// 监听 OnDeactivated 多播：关闭时触发回调 + 复位 PreviewMode
	BackpackPreviewClosedCallback = OnClosed;
	BackpackPreviewDeactivatedHandle = BackpackWidget->OnDeactivated().AddUObject(
		this, &AYogHUD::OnBackpackPreviewClosed);

	PushMajorUI();
	BackpackWidget->ActivateWidget();
}

void AYogHUD::OnBackpackPreviewClosed()
{
	if (BackpackWidget)
	{
		BackpackWidget->SetPreviewMode(false);
		BackpackWidget->OnDeactivated().Remove(BackpackPreviewDeactivatedHandle);
	}
	BackpackPreviewDeactivatedHandle.Reset();

	if (BackpackPreviewClosedCallback.IsBound())
	{
		FSimpleDelegate Cb = BackpackPreviewClosedCallback;
		BackpackPreviewClosedCallback.Unbind();
		Cb.Execute();
	}
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

	// Portal 引导 + Blackout 独立 Tick — 必须在所有早返回之前调用，
	// 否则常规情况下（PauseEffectAlpha 已稳定 / LevelEndEffect 期间）不会更新
	TickPortalPreview(DeltaSeconds);
	TickBlackoutFade(DeltaSeconds);

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

	// Portal/Blackout Tick 已移到函数顶部统一调用，此处无需重复
}

void AYogHUD::BeginPauseEffect()
{
	PausePopupCount = FMath::Max(0, PausePopupCount) + 1;
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(true);
	}
}

void AYogHUD::EndPauseEffect()
{
	PausePopupCount = FMath::Max(0, PausePopupCount - 1);
	if (APlayerController* PC = GetOwningPlayerController())
	{
		PC->SetPause(PausePopupCount > 0);
	}
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

			// 广播揭幕完成（GameMode 监听后触发 LevelClearRevealed 生命周期事件）
			OnLevelEndEffectFinished.Broadcast();
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

// ─────────────────────────────────────────────────────────────────────────────
//  v3：Portal 引导（单例浮窗 + 方位箭头）+ 进入过场 Blackout
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::ShowPortalGuidance()
{
	bShowPortalGuidance = true;

	// 一次性扫描场景所有 bIsOpen 的 Portal，缓存弱指针避免每帧 GetAllActorsOfClass
	CachedOpenPortals.Reset();
	TArray<AActor*> All;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APortal::StaticClass(), All);
	TArray<APortal*> OpenList;
	for (AActor* A : All)
	{
		if (APortal* P = Cast<APortal>(A))
		{
			if (P->bIsOpen)
			{
				CachedOpenPortals.Add(P);
				OpenList.Add(P);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[Portal] ShowPortalGuidance: 缓存开启门数=%d"), CachedOpenPortals.Num());

	// 浮窗（按需创建，状态默认 collapsed，由 TickPortalPreview 选 Target 后再显示）
	if (!PortalPreviewWidget && PortalPreviewClass)
	{
		PortalPreviewWidget = CreateWidget<UPortalPreviewWidget>(GetOwningPlayerController(), PortalPreviewClass);
		if (PortalPreviewWidget) PortalPreviewWidget->AddToViewport(15);
	}
	if (PortalPreviewWidget)
		PortalPreviewWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 方位箭头（按需创建，启用并传入 Portal 列表）
	if (!PortalDirectionWidget && PortalDirectionClass)
	{
		PortalDirectionWidget = CreateWidget<UPortalDirectionWidget>(GetOwningPlayerController(), PortalDirectionClass);
		if (PortalDirectionWidget) PortalDirectionWidget->AddToViewport(14);
	}
	if (PortalDirectionWidget)
		PortalDirectionWidget->SetActive(true, OpenList);
}

void AYogHUD::HidePortalGuidance()
{
	bShowPortalGuidance = false;
	CurrentPreviewTarget = nullptr;
	CachedOpenPortals.Reset();
	if (PortalPreviewWidget)
		PortalPreviewWidget->SetVisibility(ESlateVisibility::Collapsed);
	if (PortalDirectionWidget)
		PortalDirectionWidget->SetActive(false, {});
}

void AYogHUD::NotifyPlayerInPortalRange(APortal* /*Portal*/)
{
	// 当前实现：TickPortalPreview 已通过读 Player->PendingPortal 自动优先选中
	// 本接口保留作为后续扩展点（如立即加亮、播放进入提示音等）
}

void AYogHUD::NotifyPlayerExitedPortalRange(APortal* /*Portal*/)
{
	// 同上
}

void AYogHUD::TickPortalPreview(float /*DeltaSeconds*/)
{
	if (MajorUICount > 0) return;
	if (!bShowPortalGuidance || !PortalPreviewWidget) return;

	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->GetPawn())
	{
		PortalPreviewWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(PC->GetPawn());
	if (!Player)
	{
		PortalPreviewWidget->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	APortal* Target = nullptr;

	// 优先级 1：玩家在某门 Box 内 → 强制选中此门，保证"按 E"提示稳定
	if (Player->PendingPortal)
	{
		Target = Player->PendingPortal;
	}
	else
	{
		// 扫描缓存：屏幕内可见 OR 距离 < ForceShowDistance，挑距玩家最近一个
		const FVector PlayerPos = Player->GetActorLocation();
		FVector CamLoc; FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);

		FVector2D ViewportSize;
		if (UGameViewportClient* GVC = GetWorld()->GetGameViewport())
			GVC->GetViewportSize(ViewportSize);

		APortal* Closest = nullptr;
		float    ClosestDistSq = FLT_MAX;
		const float ForceDistSq = PortalForceShowDistance * PortalForceShowDistance;

		for (const TWeakObjectPtr<APortal>& W : CachedOpenPortals)
		{
			APortal* P = W.Get();
			if (!P || !P->bIsOpen) continue;

			const FVector DoorPos = P->GetActorLocation();
			const float   DistSq  = FVector::DistSquared(DoorPos, PlayerPos);

			bool bVisible = false;
			FVector2D SP;
			const bool bInFront = FVector::DotProduct(CamRot.Vector(), DoorPos - CamLoc) > 0.f;
			if (bInFront && PC->ProjectWorldLocationToScreen(DoorPos, SP, false))
			{
				bVisible = (SP.X >= 0.f && SP.X <= ViewportSize.X
				         && SP.Y >= 0.f && SP.Y <= ViewportSize.Y);
			}
			const bool bForceShow = (DistSq < ForceDistSq);
			if (!bVisible && !bForceShow) continue;

			if (DistSq < ClosestDistSq)
			{
				ClosestDistSq = DistSq;
				Closest = P;
			}
		}

		// 滞回：当前 Target 仍合法且与新候选差距 < Hysteresis 时不切，防中点抖动
		APortal* CurT = CurrentPreviewTarget.Get();
		if (CurT && Closest && CurT != Closest)
		{
			const float CurDist = FVector::Dist(CurT->GetActorLocation(), PlayerPos);
			const float NewDist = FMath::Sqrt(ClosestDistSq);
			if ((CurDist - NewDist) < PortalSwitchHysteresis)
				Closest = CurT;
		}

		Target = Closest;
	}

	if (!Target)
	{
		if (PortalPreviewWidget->GetVisibility() != ESlateVisibility::Collapsed)
		{
			PortalPreviewWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		CurrentPreviewTarget = nullptr;
		return;
	}

	// Target 切换 → 刷新数据 + 显示
	if (CurrentPreviewTarget.Get() != Target)
	{
		CurrentPreviewTarget = Target;
		PortalPreviewWidget->SetPreviewInfo(Target->CachedPreviewInfo);
		PortalPreviewWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 每帧位置跟随：投影 + 相机右侧避让（投影到屏幕右半→向左偏，反之向右）
	FVector2D ScreenPos;
	if (PC->ProjectWorldLocationToScreen(
			Target->GetActorLocation() + FVector(0.f, 0.f, PortalWidgetZOffset),
			ScreenPos, false))
	{
		FVector2D ViewportSize;
		if (UGameViewportClient* GVC = GetWorld()->GetGameViewport())
			GVC->GetViewportSize(ViewportSize);
		const float EffectiveSideOffset = FMath::Clamp(PortalWidgetSideOffset, 0.f, 48.f);
		const float SideX = (ScreenPos.X > ViewportSize.X * 0.5f)
			? -EffectiveSideOffset : EffectiveSideOffset;
		ScreenPos.X += SideX;
		ScreenPos.X = FMath::Clamp(ScreenPos.X, 24.f, ViewportSize.X - 24.f);
		ScreenPos.Y = FMath::Clamp(ScreenPos.Y, 24.f, ViewportSize.Y - 24.f);

		PortalPreviewWidget->SetAlignmentInViewport(FVector2D(0.5f, 1.0f));
		PortalPreviewWidget->SetPositionInViewport(ScreenPos, false);
	}

	// 交互提示：仅当玩家进入 Target 的 Box 时显示"按 E 进入"
	PortalPreviewWidget->SetInteractHintVisible(Player->PendingPortal == Target);
}

void AYogHUD::BeginBlackoutFade(float Duration)
{
	BlackoutActiveDuration = FMath::Max(Duration, 0.05f);
	BlackoutTargetAlpha    = 1.f;
}

void AYogHUD::EndBlackoutFade(float Duration)
{
	BlackoutActiveDuration = FMath::Max(Duration, 0.05f);
	BlackoutTargetAlpha    = 0.f;
}

void AYogHUD::TickBlackoutFade(float DeltaSeconds)
{
	if (!BlackoutPPVolume) return;
	if (FMath::IsNearlyEqual(BlackoutAlpha, BlackoutTargetAlpha, 0.001f)) return;

	const float Step = DeltaSeconds / BlackoutActiveDuration;
	BlackoutAlpha = FMath::Clamp(
		BlackoutAlpha + (BlackoutTargetAlpha > BlackoutAlpha ? Step : -Step), 0.f, 1.f);
	ApplyBlackoutPP();
}

// ─────────────────────────────────────────────────────────────────────────────
//  主界面遮盖：浮窗自动隐藏 / 恢复
// ─────────────────────────────────────────────────────────────────────────────

void AYogHUD::PushMajorUI()
{
	if (MajorUICount++ == 0)
		ApplyMajorUIVisibility(true);
}

void AYogHUD::PopMajorUI()
{
	MajorUICount = FMath::Max(0, MajorUICount - 1);
	if (MajorUICount == 0)
		ApplyMajorUIVisibility(false);
}

void AYogHUD::ApplyMajorUIVisibility(bool bHide)
{
	// 武器玻璃图标
	if (UWeaponGlassIconWidget* GlassIcon = MainHUDWidget ? MainHUDWidget->WeaponGlassIcon : nullptr)
	{
		GlassIcon->SetVisibility(bHide
			? ESlateVisibility::Collapsed
			: (bHasWeapon ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed));
	}

	// 传送门预览浮窗（隐藏时 Collapse；恢复时由 TickPortalPreview 下帧自动决定）
	if (PortalPreviewWidget && bHide)
		PortalPreviewWidget->SetVisibility(ESlateVisibility::Collapsed);

	// 传送门方位箭头
	if (PortalDirectionWidget)
	{
		PortalDirectionWidget->SetVisibility((!bHide && bShowPortalGuidance)
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	// 信息提示浮窗（一次性触发，关闭即消，不恢复）
	if (bHide)
	{
		if (UInfoPopupWidget* InfoPopup = GetInfoPopupWidget())
			InfoPopup->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void AYogHUD::ApplyBlackoutPP()
{
	if (!BlackoutPPVolume) return;

	// 目标值复用 LevelEndEffectDA（Saturation/Gain）；DA 未配置时回退全黑全灰
	float TargetSat  = 0.f;
	float TargetGain = 0.f;
	if (LevelEndEffectDA)
	{
		TargetSat  = LevelEndEffectDA->BlackoutSaturation;
		TargetGain = LevelEndEffectDA->BlackoutGain;
	}

	const float Sat  = FMath::Lerp(1.f, TargetSat,  BlackoutAlpha);
	const float Gain = FMath::Lerp(1.f, TargetGain, BlackoutAlpha);

	BlackoutPPVolume->Settings.ColorSaturation = FVector4(1.f, 1.f, 1.f, Sat);
	BlackoutPPVolume->Settings.ColorGain       = FVector4(1.f, 1.f, 1.f, Gain);
	BlackoutPPVolume->BlendWeight              = (BlackoutAlpha > 0.001f) ? 1.f : 0.f;
}

