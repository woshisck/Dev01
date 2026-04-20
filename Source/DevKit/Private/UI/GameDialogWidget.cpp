#include "UI/GameDialogWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/TutorialManager.h"
#include "UI/YogHUD.h"

#define LOCTEXT_NAMESPACE "TutorialPopup"

// ————————————————————————————————————————————————————
// 公共接口
// ————————————————————————————————————————————————————

void UTutorialPopupWidget::ShowPopup(const TArray<FTutorialPage>& InPages)
{
	if (InPages.IsEmpty()) return;
	Pages      = InPages;
	CurrentPage = 0;
	TotalPages  = Pages.Num();

	if (!IsInViewport())
		AddToViewport(200);

	if (IsActivated())
		DeactivateWidget();

	ActivateWidget();
}

void UTutorialPopupWidget::OnNextPressed()
{
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] OnNextPressed called — bIsInteractable=%s"),
		bIsInteractable ? TEXT("true") : TEXT("false"));
	if (!bIsInteractable) return;

	CurrentPage++;
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] OnNextPressed PASS: CurrentPage=%d TotalPages=%d"), CurrentPage, TotalPages);
	if (CurrentPage >= Pages.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] Last page — Anim_FadeOut=%s"),
			Anim_FadeOut ? TEXT("VALID") : TEXT("NULL"));
		// 末页：播放渐出动画；动画结束后自动调用 OnFadeOutAnimFinished → ConfirmClose
		if (Anim_FadeOut)
		{
			FWidgetAnimationDynamicEvent FinishEvent;
			FinishEvent.BindDynamic(this, &UTutorialPopupWidget::OnFadeOutAnimFinished);
			BindToAnimationFinished(Anim_FadeOut, FinishEvent);
			PlayAnimation(Anim_FadeOut);
		}
		else
		{
			ConfirmClose();
		}
		return;
	}
	RefreshPage();
}

void UTutorialPopupWidget::ConfirmClose()
{
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] ConfirmClose called"));
	if (UGameInstance* GI = GetGameInstance())
		if (UTutorialManager* TM = GI->GetSubsystem<UTutorialManager>())
			TM->NotifyPopupClosed();

	DeactivateWidget();
}

// ————————————————————————————————————————————————————
// CommonUI 覆写
// ————————————————————————————————————————————————————

void UTutorialPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BtnConfirm)
		BtnConfirm->OnClicked.AddDynamic(this, &UTutorialPopupWidget::OnNextPressed);
}

TOptional<FUIInputConfig> UTutorialPopupWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* UTutorialPopupWidget::NativeGetDesiredFocusTarget() const
{
	// Widget 激活时 CommonUI 自动把焦点给 BtnConfirm，手柄 A 键可直接点击
	return BtnConfirm;
}

void UTutorialPopupWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::Visible);
	UGameplayStatics::SetGamePaused(this, true);

	if (APlayerController* PC = GetOwningPlayer())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();

	// 只管光标，不手动设 InputMode —— CommonUI 通过 GetDesiredInputConfig() 自动处理
	// 手动调 SetInputMode 会覆盖 CommonUI 的焦点路由，导致 NativeGetDesiredFocusTarget 失效
	if (APlayerController* PC = GetOwningPlayer())
		PC->SetShowMouseCursor(true);

	RefreshPage();

	bIsInteractable = false;
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] NativeOnActivated: bIsInteractable=false, Anim_FadeIn=%s"),
		Anim_FadeIn ? TEXT("VALID") : TEXT("NULL"));

	if (Anim_FadeIn)
	{
		FWidgetAnimationDynamicEvent FadeInFinishEvent;
		FadeInFinishEvent.BindDynamic(this, &UTutorialPopupWidget::OnFadeInAnimFinished);
		BindToAnimationFinished(Anim_FadeIn, FadeInFinishEvent);
		PlayAnimation(Anim_FadeIn);
		UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] PlayAnimation(Anim_FadeIn) called, waiting for finish..."));
	}
	else
	{
		bIsInteractable = true;
		UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] No Anim_FadeIn — bIsInteractable=true immediately"));
	}
}

void UTutorialPopupWidget::NativeOnDeactivated()
{
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] NativeOnDeactivated — bIsInteractable=%s"),
		bIsInteractable ? TEXT("true") : TEXT("false"));
	// 安全网：无论何种关闭路径都清除 bPopupShowing
	if (UGameInstance* GI = GetGameInstance())
		if (UTutorialManager* TM = GI->GetSubsystem<UTutorialManager>())
			TM->NotifyPopupClosed();

	if (APlayerController* PC = GetOwningPlayer())
		PC->SetShowMouseCursor(false);

	UGameplayStatics::SetGamePaused(this, false);

	if (APlayerController* PC = GetOwningPlayer())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();

	Super::NativeOnDeactivated();

	// CommonUI 在 Super 里弹出 Menu 模式，但不会主动设回 Game 模式，需要显式恢复
	if (APlayerController* PC = GetOwningPlayer())
		PC->SetInputMode(FInputModeGameOnly());

	RemoveFromParent();
}

// ————————————————————————————————————————————————————
// 私有
// ————————————————————————————————————————————————————

void UTutorialPopupWidget::OnFadeInAnimFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] OnFadeInAnimFinished — FadeOut already playing=%s"),
		(Anim_FadeOut && IsAnimationPlaying(Anim_FadeOut)) ? TEXT("YES_LEAK") : TEXT("no"));
	UnbindAllFromAnimationFinished(Anim_FadeIn);
	bIsInteractable = true;
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] bIsInteractable=true SET"));
}

void UTutorialPopupWidget::OnFadeOutAnimFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("[TutorialPopup] OnFadeOutAnimFinished — calling ConfirmClose"));
	UnbindAllFromAnimationFinished(Anim_FadeOut);
	ConfirmClose();
}

void UTutorialPopupWidget::RefreshPage()
{
	if (!Pages.IsValidIndex(CurrentPage)) return;

	const FTutorialPage& Page = Pages[CurrentPage];
	if (TitleText) TitleText->SetText(Page.Title);
	if (BodyText)  BodyText->SetText(Page.Body);

	const bool bIsLast = (CurrentPage == TotalPages - 1);
	if (BtnConfirmLabel)
	{
		BtnConfirmLabel->SetText(bIsLast
			? LOCTEXT("Close", "知道了")
			: LOCTEXT("Next",  "下一页"));
	}

	if (IllustrationImage)
	{
		if (Page.Illustration)
			IllustrationImage->SetBrushFromTexture(Page.Illustration, true);
		else
			IllustrationImage->SetBrushTintColor(FLinearColor::Black);
	}

	if (BodySubText)
	{
		const bool bHasSub = !Page.SubText.IsEmpty();
		BodySubText->SetVisibility(bHasSub ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bHasSub) BodySubText->SetText(Page.SubText);
	}

	if (PageHint)
	{
		if (TotalPages <= 1)
		{
			PageHint->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			PageHint->SetVisibility(ESlateVisibility::HitTestInvisible);
			PageHint->SetText(FText::Format(
				LOCTEXT("PageFmt", "{0} / {1}"),
				FText::AsNumber(CurrentPage + 1),
				FText::AsNumber(TotalPages)));
		}
	}
}

#undef LOCTEXT_NAMESPACE
