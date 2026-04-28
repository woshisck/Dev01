#include "UI/AltarMenuWidget.h"
#include "UI/RunePurificationWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "UI/YogHUD.h"
#include "Character/PlayerCharacterBase.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"

void UAltarMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
}

TOptional<FUIInputConfig> UAltarMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UAltarMenuWidget::NativeOnActivated()
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

void UAltarMenuWidget::NativeOnDeactivated()
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

FReply UAltarMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape ||
		Key == EKeys::Gamepad_FaceButton_Right ||
		Key == EKeys::Gamepad_Special_Right)
	{
		CloseMenu();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UAltarMenuWidget::SetupAltar(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer)
{
	AltarData = InData;
	OwningPlayer = InPlayer;

	if (PurificationWidgetClass && !PurificationWidget)
	{
		PurificationWidget = CreateWidget<URunePurificationWidget>(GetOwningPlayer(), PurificationWidgetClass);
		if (PurificationWidget)
			PurificationWidget->AddToViewport(11);
	}
	if (SacrificeWidgetClass && !SacrificeWidget)
	{
		SacrificeWidget = CreateWidget<USacrificeSelectionWidget>(GetOwningPlayer(), SacrificeWidgetClass);
		if (SacrificeWidget)
			SacrificeWidget->AddToViewport(11);
	}
}

void UAltarMenuWidget::OpenPurification()
{
	if (!PurificationWidget || !OwningPlayer.IsValid()) return;
	DeactivateWidget();
	PurificationWidget->Setup(OwningPlayer.Get());
	PurificationWidget->ActivateWidget();
}

void UAltarMenuWidget::OpenSacrifice()
{
	if (!SacrificeWidget || !AltarData || !OwningPlayer.IsValid()) return;
	DeactivateWidget();
	SacrificeWidget->Setup(AltarData, OwningPlayer.Get());
	SacrificeWidget->ActivateWidget();
}

void UAltarMenuWidget::CloseMenu()
{
	DeactivateWidget();
}
