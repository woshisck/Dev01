#include "UI/PauseMenuWidget.h"

#include "Character/YogPlayerControllerBase.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "SaveGame/YogSaveSubsystem.h"
#include "System/YogGameInstanceBase.h"
#include "UI/YogHUD.h"
#include "UI/YogInputKeyUtils.h"
#include "UI/YogUIManagerSubsystem.h"

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	SetVisibility(ESlateVisibility::Collapsed);
	CacheButtons();
}

void UPauseMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (BtnControl)
	{
		BtnControl->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleControl);
	}
	if (BtnResume)
	{
		BtnResume->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResume);
	}
	if (BtnContinue)
	{
		BtnContinue->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleResume);
	}
	if (BtnDisplay)
	{
		BtnDisplay->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleDisplay);
	}
	if (BtnSound)
	{
		BtnSound->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSound);
	}
	if (BtnSave)
	{
		BtnSave->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleSave);
	}
	if (BtnQuit)
	{
		BtnQuit->OnClicked.AddDynamic(this, &UPauseMenuWidget::HandleQuit);
	}
}

TOptional<FUIInputConfig> UPauseMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void UPauseMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);
	CacheButtons();

	// FocusButton handles per-button focus; SetUserFocus(player) was targeting the wrong root.
	FocusButton(FocusedButtonIndex);
}

void UPauseMenuWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);

		// Modal layer dropping returns top to Game → Subsystem applies GameOnly + hides cursor.
	Super::NativeOnDeactivated();
}

UWidget* UPauseMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (MenuButtons.IsValidIndex(FocusedButtonIndex))
	{
		return MenuButtons[FocusedButtonIndex].Get();
	}
	if (BtnResume)
	{
		return BtnResume.Get();
	}
	if (BtnContinue)
	{
		return BtnContinue.Get();
	}
	return BtnControl.Get();
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key) || YogInputKeys::IsMenuKey(Key))
	{
		CloseMenu();
		return FReply::Handled();
	}

	const int32 VerticalDirection = YogInputKeys::GetVerticalNavigationDirection(Key);
	if (VerticalDirection != 0)
	{
		FocusButton(FocusedButtonIndex + VerticalDirection);
		return FReply::Handled();
	}

	if (YogInputKeys::IsAcceptKey(Key))
	{
		ActivateFocusedButton();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UPauseMenuWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();
	if (Key == EKeys::Gamepad_LeftY && FMath::Abs(Value) >= 0.65f)
	{
		const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		if (Now - LastAnalogNavigationTime >= 0.18f)
		{
			LastAnalogNavigationTime = Now;
			FocusButton(FocusedButtonIndex + (Value > 0.f ? -1 : 1));
			return FReply::Handled();
		}
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
}

void UPauseMenuWidget::CloseMenu()
{
	if (!UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::PauseMenu))
	{
		DeactivateWidget();
	}
}

void UPauseMenuWidget::HandleControl()
{
	FocusButton(FindButtonIndex(BtnControl));
	SetDescription(NSLOCTEXT("PauseMenu", "ControlDesc", "Controller and keyboard input reference."));
}

void UPauseMenuWidget::HandleResume()
{
	FocusButton(FMath::Max(0, FindButtonIndex(BtnResume ? BtnResume.Get() : BtnContinue.Get())));
	CloseMenu();
}

void UPauseMenuWidget::HandleDisplay()
{
	FocusButton(FindButtonIndex(BtnDisplay));
	SetDescription(NSLOCTEXT("PauseMenu", "DisplayDesc", "Display settings entry."));
}

void UPauseMenuWidget::HandleSound()
{
	FocusButton(FindButtonIndex(BtnSound));
	SetDescription(NSLOCTEXT("PauseMenu", "SoundDesc", "Sound settings entry."));
}

void UPauseMenuWidget::HandleSave()
{
	FocusButton(FindButtonIndex(BtnSave));
	bool bSaved = false;

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UYogSaveSubsystem* SaveSubsystem = GameInstance->GetSubsystem<UYogSaveSubsystem>())
		{
			SaveSubsystem->GetCurrentSave();
			SaveSubsystem->WriteSaveGame();
			bSaved = true;
		}
		else if (UYogGameInstanceBase* YogGI = Cast<UYogGameInstanceBase>(GameInstance))
		{
			bSaved = YogGI->WriteSaveGame();
		}
	}

	SetDescription(bSaved
		? NSLOCTEXT("PauseMenu", "SaveComplete", "Game saved.")
		: NSLOCTEXT("PauseMenu", "SaveUnavailable", "Save data is not available."));
}

void UPauseMenuWidget::HandleQuit()
{
	FocusButton(FindButtonIndex(BtnQuit));

	UYogGameInstanceBase* YogGI = GetGameInstance() ? Cast<UYogGameInstanceBase>(GetGameInstance()) : nullptr;
	CloseMenu();

	if (YogGI)
	{
		YogGI->ReturnToMainMenu();
	}
}

void UPauseMenuWidget::CacheButtons()
{
	MenuButtons.Reset();
	if (BtnResume)
	{
		MenuButtons.Add(BtnResume);
	}
	else if (BtnContinue)
	{
		MenuButtons.Add(BtnContinue);
	}
	if (BtnControl)
	{
		MenuButtons.Add(BtnControl);
	}
	if (BtnDisplay)
	{
		MenuButtons.Add(BtnDisplay);
	}
	if (BtnSound)
	{
		MenuButtons.Add(BtnSound);
	}
	if (BtnSave)
	{
		MenuButtons.Add(BtnSave);
	}
	if (BtnQuit)
	{
		MenuButtons.Add(BtnQuit);
	}

	FocusedButtonIndex = FMath::Clamp(FocusedButtonIndex, 0, FMath::Max(0, MenuButtons.Num() - 1));
}

int32 UPauseMenuWidget::FindButtonIndex(const UButton* Button) const
{
	if (!Button)
	{
		return FocusedButtonIndex;
	}
	for (int32 Index = 0; Index < MenuButtons.Num(); ++Index)
	{
		if (MenuButtons[Index] == Button)
		{
			return Index;
		}
	}
	return FocusedButtonIndex;
}

void UPauseMenuWidget::FocusButton(int32 NewIndex)
{
	if (MenuButtons.Num() == 0)
	{
		return;
	}

	FocusedButtonIndex = (NewIndex % MenuButtons.Num() + MenuButtons.Num()) % MenuButtons.Num();
	for (int32 ButtonIndex = 0; ButtonIndex < MenuButtons.Num(); ++ButtonIndex)
	{
		if (UButton* Button = MenuButtons[ButtonIndex])
		{
			const bool bFocused = ButtonIndex == FocusedButtonIndex;
			Button->SetBackgroundColor(bFocused
				? FLinearColor(0.18f, 0.48f, 0.85f, 0.90f)
				: FLinearColor(0.02f, 0.02f, 0.025f, 0.45f));
		}
	}

	if (UButton* Button = MenuButtons[FocusedButtonIndex])
	{
		Button->SetKeyboardFocus();
	}

	UButton* FocusedButton = MenuButtons.IsValidIndex(FocusedButtonIndex) ? MenuButtons[FocusedButtonIndex].Get() : nullptr;
	if (FocusedButton == BtnResume || FocusedButton == BtnContinue)
	{
		SetDescription(NSLOCTEXT("PauseMenu", "ResumeDesc", "Return to the current run."));
	}
	else if (FocusedButton == BtnControl)
	{
		SetDescription(NSLOCTEXT("PauseMenu", "ControlDesc", "Controller and keyboard input reference."));
	}
	else if (FocusedButton == BtnDisplay)
	{
		SetDescription(NSLOCTEXT("PauseMenu", "DisplayDesc", "Display settings entry."));
	}
	else if (FocusedButton == BtnSound)
	{
		SetDescription(NSLOCTEXT("PauseMenu", "SoundDesc", "Sound settings entry."));
	}
	else if (FocusedButton == BtnSave)
	{
		SetDescription(NSLOCTEXT("PauseMenu", "SaveDesc", "Save the current run state."));
	}
	else if (FocusedButton == BtnQuit)
	{
		SetDescription(NSLOCTEXT("PauseMenu", "QuitDesc", "Quit the game and back to the Main Menu."));
	}
}

void UPauseMenuWidget::ActivateFocusedButton()
{
	if (!MenuButtons.IsValidIndex(FocusedButtonIndex))
	{
		return;
	}

	if (UButton* Button = MenuButtons[FocusedButtonIndex])
	{
		Button->OnClicked.Broadcast();
	}
}

void UPauseMenuWidget::SetDescription(const FText& Text)
{
	if (DescriptionText)
	{
		DescriptionText->SetText(Text);
	}
}
