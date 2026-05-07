#include "UI/RunePurificationWidget.h"
#include "Character/PlayerCharacterBase.h"
#include "UI/YogHUD.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"

TOptional<FUIInputConfig> URunePurificationWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void URunePurificationWidget::NativeOnActivated()
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

void URunePurificationWidget::NativeOnDeactivated()
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

FReply URunePurificationWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape ||
		Key == EKeys::Gamepad_FaceButton_Right ||
		Key == EKeys::Gamepad_Special_Right)
	{
		CancelPurification();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void URunePurificationWidget::Setup(APlayerCharacterBase* InPlayer)
{
	OwningPlayer     = InPlayer;
	Phase            = 0;
	SelectedRuneGuid = FGuid();
	SelectedCell     = FIntPoint(0, 0);

	OnShowRuneList();
}

void URunePurificationWidget::SelectRune(FGuid RuneGuid)
{
}

void URunePurificationWidget::SelectCell(FIntPoint LocalCell)
{
	if (Phase != 1 || LocalCell == FIntPoint(0, 0)) return;
	SelectedCell = LocalCell;
}

void URunePurificationWidget::ConfirmPurification()
{
	OnPurificationFinished(false);
	DeactivateWidget();
}

void URunePurificationWidget::CancelPurification()
{
	OnPurificationFinished(false);
	DeactivateWidget();
}
