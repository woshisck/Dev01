#include "UI/AltarMenuWidget.h"
#include "UI/RunePurificationWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "UI/YogHUD.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Button.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "UI/YogCommonUITextBlock.h"
#include "UI/YogInputKeyUtils.h"

TSubclassOf<UTextBlock> UAltarMenuWidget::GetMenuTextBlockClassForTests()
{
	return UYogCommonUITextBlock::StaticClass();
}

void UAltarMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetVisibility(ESlateVisibility::Collapsed);
	if (BtnPurification) BtnPurification->OnClicked.AddDynamic(this, &UAltarMenuWidget::OnPurificationClicked);
	if (BtnSacrifice) BtnSacrifice->OnClicked.AddDynamic(this, &UAltarMenuWidget::OnSacrificeClicked);
	if (BtnClose) BtnClose->OnClicked.AddDynamic(this, &UAltarMenuWidget::OnCloseClicked);
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
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	// FocusButton handles per-button focus; SetUserFocus(player) was targeting the wrong root.
	FocusButton(FocusedButtonIndex);
}

void UAltarMenuWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		// Mouse cursor + InputMode are owned by UYogUIManagerSubsystem::ApplyInputModeForLayer.
		// When this Activatable deactivates, top layer drops back to Game and the Subsystem restores GameOnly.
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			HUD->EndPauseEffect();
	}
	Super::NativeOnDeactivated();
}

FReply UAltarMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key) || YogInputKeys::IsMenuKey(Key))
	{
		CloseMenu();
		return FReply::Handled();
	}

	const int32 NavDirection = YogInputKeys::GetVerticalNavigationDirection(Key) != 0
		? YogInputKeys::GetVerticalNavigationDirection(Key)
		: YogInputKeys::GetHorizontalNavigationDirection(Key);
	if (NavDirection != 0)
	{
		MoveFocus(NavDirection);
		return FReply::Handled();
	}

	if (YogInputKeys::IsAcceptKey(Key))
	{
		ActivateFocusedButton();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UAltarMenuWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();
	if ((Key == EKeys::Gamepad_LeftY || Key == EKeys::Gamepad_LeftX) && FMath::Abs(Value) >= 0.65f)
	{
		const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		if (Now - LastAnalogNavigationTime >= 0.18f)
		{
			LastAnalogNavigationTime = Now;
			const int32 Direction = (Key == EKeys::Gamepad_LeftY)
				? (Value > 0.f ? -1 : 1)
				: (Value < 0.f ? -1 : 1);
			MoveFocus(Direction);
			return FReply::Handled();
		}
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
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

TArray<UButton*> UAltarMenuWidget::GetFocusableButtons() const
{
	TArray<UButton*> Buttons;
	if (BtnPurification && BtnPurification->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Buttons.Add(BtnPurification);
	}
	if (BtnSacrifice && BtnSacrifice->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Buttons.Add(BtnSacrifice);
	}
	if (BtnClose && BtnClose->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Buttons.Add(BtnClose);
	}
	return Buttons;
}

void UAltarMenuWidget::FocusButton(int32 NewIndex)
{
	TArray<UButton*> Buttons = GetFocusableButtons();
	if (Buttons.IsEmpty())
	{
		FocusedButtonIndex = FMath::Clamp(NewIndex, 0, 1);
		return;
	}

	FocusedButtonIndex = (NewIndex % Buttons.Num() + Buttons.Num()) % Buttons.Num();
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		Buttons[Index]->SetBackgroundColor(Index == FocusedButtonIndex
			? FLinearColor(0.16f, 0.42f, 0.82f, 0.85f)
			: FLinearColor::White);
	}
	Buttons[FocusedButtonIndex]->SetKeyboardFocus();
}

void UAltarMenuWidget::MoveFocus(int32 Direction)
{
	FocusButton(FocusedButtonIndex + Direction);
}

void UAltarMenuWidget::ActivateFocusedButton()
{
	TArray<UButton*> Buttons = GetFocusableButtons();
	if (Buttons.IsValidIndex(FocusedButtonIndex) && Buttons[FocusedButtonIndex])
	{
		Buttons[FocusedButtonIndex]->OnClicked.Broadcast();
		return;
	}

	if (FocusedButtonIndex <= 0)
	{
		OpenPurification();
	}
	else
	{
		OpenSacrifice();
	}
}

void UAltarMenuWidget::OnPurificationClicked()
{
	OpenPurification();
}

void UAltarMenuWidget::OnSacrificeClicked()
{
	OpenSacrifice();
}

void UAltarMenuWidget::OnCloseClicked()
{
	CloseMenu();
}
