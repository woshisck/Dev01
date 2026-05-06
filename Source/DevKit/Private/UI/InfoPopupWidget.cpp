#include "UI/InfoPopupWidget.h"
#include "Data/LevelInfoPopupDA.h"
#include "RuneHudTextUtils.h"
#include "CommonRichTextBlock.h"
#include "Components/Image.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"

void UInfoPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetVisibility(ESlateVisibility::Collapsed);
	SetRenderOpacity(0.f);

	if (BtnClose)
		BtnClose->OnClicked.AddDynamic(this, &UInfoPopupWidget::OnBtnCloseClicked);
}

void UInfoPopupWidget::Show(const ULevelInfoPopupDA* DA)
{
	if (!DA) return;

	if (BodyText) BodyText->SetText(RuneHudTextUtils::GetLevelInfoHudSummary(*DA, 58));

	if (TitleText)
	{
		const bool bHasTitle = !DA->Title.IsEmpty();
		TitleText->SetVisibility(bHasTitle ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bHasTitle) TitleText->SetText(DA->Title);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);
	FadeAlpha = 0.f;
	FadeDirection = 1.f;
	SetRenderOpacity(0.f);

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
	UE_LOG(LogTemp, Log, TEXT("[InfoPopup] RequestClose() called, FadeDirection=%.1f FadeAlpha=%.2f"),
		FadeDirection, FadeAlpha);
	GetWorld()->GetTimerManager().ClearTimer(AutoCloseTimer);

	if (FadeDirection >= 0.f)
		FadeDirection = -1.f;
}

void UInfoPopupWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FadeDirection == 0.f) return;

	FadeAlpha = FMath::Clamp(FadeAlpha + FadeDirection * (InDeltaTime / FadeDuration), 0.f, 1.f);
	SetRenderOpacity(FadeAlpha);

	if (FadeDirection > 0.f && FadeAlpha >= 1.f)
	{
		FadeDirection = 0.f;
	}
	else if (FadeDirection < 0.f && FadeAlpha <= 0.f)
	{
		FadeDirection = 0.f;
		DoClose();
	}
}

void UInfoPopupWidget::OnBtnCloseClicked()
{
	RequestClose();
}

void UInfoPopupWidget::DoClose()
{
	SetVisibility(ESlateVisibility::Collapsed);
	OnClosed.Broadcast();
}
