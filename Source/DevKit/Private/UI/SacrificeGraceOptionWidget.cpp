#include "UI/SacrificeGraceOptionWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Input/CommonUIInputTypes.h"
#include "Data/SacrificeGraceDA.h"
#include "Character/PlayerCharacterBase.h"
#include "Map/SacrificeGracePickup.h"
#include "UI/YogHUD.h"
#include "InputCoreTypes.h"

void USacrificeGraceOptionWidget::Setup(USacrificeGraceDA* InDA, APlayerCharacterBase* InPlayer, ASacrificeGracePickup* InPickup)
{
	SacrificeDA  = InDA;
	OwningPlayer = InPlayer;
	SourcePickup = InPickup;

	if (TitleText && InDA)
		TitleText->SetText(InDA->DisplayName);
	if (DescriptionText && InDA)
		DescriptionText->SetText(InDA->Description);

	OnSetup(InDA);
}

void USacrificeGraceOptionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (BtnYes) BtnYes->OnClicked.AddDynamic(this, &USacrificeGraceOptionWidget::OnYesClicked);
	if (BtnNo)  BtnNo->OnClicked.AddDynamic(this, &USacrificeGraceOptionWidget::OnNoClicked);
}

void USacrificeGraceOptionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->BeginPauseEffect();

		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}
	SetUserFocus(GetOwningPlayer());
}

void USacrificeGraceOptionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();
	}
	Super::NativeOnDeactivated();
}

TOptional<FUIInputConfig> USacrificeGraceOptionWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* USacrificeGraceOptionWidget::NativeGetDesiredFocusTarget() const
{
	return BtnYes;
}

FReply USacrificeGraceOptionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape ||
		Key == EKeys::Gamepad_FaceButton_Right ||
		Key == EKeys::Gamepad_Special_Right)
	{
		CancelChoice();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USacrificeGraceOptionWidget::OnYesClicked()
{
	if (APlayerCharacterBase* Player = OwningPlayer.Get())
		Player->AcquireSacrificeGrace(SacrificeDA);

	if (ASacrificeGracePickup* Pickup = SourcePickup.Get())
		Pickup->ConsumeAndDestroy();

	DeactivateWidget();
}

void USacrificeGraceOptionWidget::OnNoClicked()
{
	if (ASacrificeGracePickup* Pickup = SourcePickup.Get())
		Pickup->ResetForSkip(OwningPlayer.Get());

	DeactivateWidget();
}

void USacrificeGraceOptionWidget::CancelChoice()
{
	OnNoClicked();
}
