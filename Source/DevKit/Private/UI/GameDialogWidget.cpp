#include "UI/GameDialogWidget.h"
#include "Kismet/GameplayStatics.h"

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
		DeactivateWidget();
		return;
	}
	RefreshPage();
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
	RefreshPage();
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
		TEXT("[Tutorial] NativeOnActivated — popup should be visible now"));
}

void UTutorialPopupWidget::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();
	UGameplayStatics::SetGamePaused(this, false);
	RemoveFromParent(); // 从 Viewport 彻底移除，下次 ShowPopup 重新添加
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
