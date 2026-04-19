#include "UI/GameDialogWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Tutorial/TutorialManager.h"
#include "UI/YogHUD.h"

#define LOCTEXT_NAMESPACE "TutorialPopup"

void UTutorialPopupWidget::ShowPopup(const TArray<FTutorialPage>& InPages)
{
	if (InPages.IsEmpty()) return;
	Pages = InPages;
	CurrentPage = 0;
	TotalPages = Pages.Num();

	// 每次显示时重新加入 Viewport（关闭时 RemoveFromParent 移除了）
	if (!IsInViewport())
	{
		AddToViewport(200);
	}
	// 重置激活状态确保 NativeOnActivated 一定被调用
	if (IsActivated())
	{
		DeactivateWidget();
	}
	ActivateWidget();
}

void UTutorialPopupWidget::OnNextPressed()
{
	CurrentPage++;
	if (CurrentPage >= Pages.Num())
	{
		// 触发 WBP 渐出动画；动画结束后 WBP 调用 ConfirmClose()
		BP_OnPopupClosing();
		return;
	}
	RefreshPage();
}

void UTutorialPopupWidget::ConfirmClose()
{
	// 通知 TutorialManager 弹窗已关闭（清除 IsPopupShowing 标志，让浮窗恢复显示）
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTutorialManager* TM = GI->GetSubsystem<UTutorialManager>())
		{
			TM->NotifyPopupClosed();
		}
	}
	DeactivateWidget();
}

void UTutorialPopupWidget::BP_OnPopupClosing_Implementation()
{
	ConfirmClose();
}

void UTutorialPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (BtnConfirm)
	{
		BtnConfirm->OnClicked.AddDynamic(this, &UTutorialPopupWidget::OnNextPressed);
	}
}

TOptional<FUIInputConfig> UTutorialPopupWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UTutorialPopupWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::Visible);
	UGameplayStatics::SetGamePaused(this, true);

	if (APlayerController* PC = GetOwningPlayer())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();

	// 明确显示光标 + 切换到 UI 输入模式（不依赖 CommonUI Stack 自动管理）
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(true);
		PC->SetInputMode(FInputModeUIOnly());
	}

	RefreshPage();
	BP_OnPopupShown();
}

void UTutorialPopupWidget::NativeOnDeactivated()
{
	// 安全网：确保 bPopupShowing 一定被清除（防止 WBP 未调用 ConfirmClose 时标志滞留）
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UTutorialManager* TM = GI->GetSubsystem<UTutorialManager>())
		{
			TM->NotifyPopupClosed();
		}
	}

	// 先还原输入/光标，再调 Super —— 防止 Super 的 CommonUI 回调覆盖还原结果
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
	}
	UGameplayStatics::SetGamePaused(this, false);

	if (APlayerController* PC = GetOwningPlayer())
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();

	Super::NativeOnDeactivated();
	RemoveFromParent();
}

void UTutorialPopupWidget::RefreshPage()
{
	if (!Pages.IsValidIndex(CurrentPage)) return;

	const FTutorialPage& Page = Pages[CurrentPage];
	if (TitleText)   TitleText->SetText(Page.Title);
	if (BodyText)    BodyText->SetText(Page.Body);

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
		{
			IllustrationImage->SetBrushFromTexture(Page.Illustration, true);
		}
		else
		{
			// 无插图：保持黑色背景（Brush 不变，不设置贴图）
			IllustrationImage->SetBrushTintColor(FLinearColor::Black);
		}
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
