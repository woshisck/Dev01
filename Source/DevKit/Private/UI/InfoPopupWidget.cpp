#include "UI/InfoPopupWidget.h"
#include "Data/LevelInfoPopupDA.h"
#include "CommonRichTextBlock.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Animation/WidgetAnimation.h"

void UInfoPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);

	if (BtnClose)
		BtnClose->OnClicked.AddDynamic(this, &UInfoPopupWidget::OnBtnCloseClicked);
}

void UInfoPopupWidget::Show(const ULevelInfoPopupDA* DA)
{
	if (!DA) return;

	// 填充文字
	if (BodyText) BodyText->SetText(DA->Body);

	if (TitleText)
	{
		const bool bHasTitle = !DA->Title.IsEmpty();
		TitleText->SetVisibility(bHasTitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bHasTitle) TitleText->SetText(DA->Title);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (Anim_FadeIn)
		PlayAnimation(Anim_FadeIn);

	// 自动关闭计时
	if (DA->DisplayDuration > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			AutoCloseTimer,
			this, &UInfoPopupWidget::RequestClose,
			DA->DisplayDuration, false);
	}
}

void UInfoPopupWidget::RequestClose()
{
	GetWorld()->GetTimerManager().ClearTimer(AutoCloseTimer);

	if (Anim_FadeOut)
	{
		FWidgetAnimationDynamicEvent FinishEvent;
		FinishEvent.BindDynamic(this, &UInfoPopupWidget::OnFadeOutFinished);
		BindToAnimationFinished(Anim_FadeOut, FinishEvent);
		PlayAnimation(Anim_FadeOut);
	}
	else
	{
		DoClose();
	}
}

void UInfoPopupWidget::OnBtnCloseClicked()
{
	RequestClose();
}

void UInfoPopupWidget::OnFadeOutFinished()
{
	UnbindAllFromAnimationFinished(Anim_FadeOut);
	DoClose();
}

void UInfoPopupWidget::DoClose()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}
