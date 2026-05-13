#include "UI/SacrificeGraceOptionWidget.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Input/CommonUIInputTypes.h"
#include "Data/SacrificeGraceDA.h"
#include "Character/PlayerCharacterBase.h"
#include "Map/SacrificeGracePickup.h"
#include "UI/YogHUD.h"
#include "InputCoreTypes.h"
#include "UI/YogInputKeyUtils.h"

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
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	// FocusButton handles per-button focus; SetUserFocus(player) was targeting the wrong root.
	FocusButton(FocusedButtonIndex);
}

void USacrificeGraceOptionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		// Mouse cursor + InputMode are owned by UYogUIManagerSubsystem::ApplyInputModeForLayer.
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
	if (YogInputKeys::IsBackKey(Key) || YogInputKeys::IsMenuKey(Key))
	{
		CancelChoice();
		return FReply::Handled();
	}
	const int32 NavDirection = YogInputKeys::GetHorizontalNavigationDirection(Key) != 0
		? YogInputKeys::GetHorizontalNavigationDirection(Key)
		: YogInputKeys::GetVerticalNavigationDirection(Key);
	if (NavDirection != 0)
	{
		FocusButton(FocusedButtonIndex + NavDirection);
		return FReply::Handled();
	}
	if (YogInputKeys::IsAcceptKey(Key))
	{
		ActivateFocusedButton();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply USacrificeGraceOptionWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
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
			FocusButton(FocusedButtonIndex + Direction);
			return FReply::Handled();
		}
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
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

void USacrificeGraceOptionWidget::FocusButton(int32 NewIndex)
{
	TArray<UButton*> Buttons;
	if (BtnYes) Buttons.Add(BtnYes);
	if (BtnNo) Buttons.Add(BtnNo);
	if (Buttons.IsEmpty())
	{
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

void USacrificeGraceOptionWidget::ActivateFocusedButton()
{
	if (FocusedButtonIndex <= 0)
	{
		OnYesClicked();
	}
	else
	{
		OnNoClicked();
	}
}
