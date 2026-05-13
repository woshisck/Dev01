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

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->BeginPauseEffect();
			bPauseEffectActive = true;
		}

		PC->SetShowMouseCursor(true);
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}

	// FocusButton handles per-button focus; SetUserFocus(player) was targeting the wrong root.
	FocusButton(FocusedButtonIndex);
}

void UPauseMenuWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);

	if (APlayerController* PC = GetOwningPlayer())
	{
		// Mouse cursor + InputMode are owned by UYogUIManagerSubsystem::ApplyInputModeForLayer.
		// Modal layer dropping returns top to Game → Subsystem applies GameOnly + hides cursor.
		if (bPauseEffectActive)
		{
			if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
			{
				HUD->EndPauseEffect();
			}
			bPauseEffectActive = false;
		}
	}

	Super::NativeOnDeactivated();
}

UWidget* UPauseMenuWidget::NativeGetDesiredFocusTarget() const
{
	return MenuButtons.IsValidIndex(FocusedButtonIndex) ? MenuButtons[FocusedButtonIndex].Get() : BtnControl.Get();
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
	DeactivateWidget();
}

void UPauseMenuWidget::HandleControl()
{
	FocusButton(0);
	SetDescription(NSLOCTEXT("PauseMenu", "ControlDesc", "Controller and keyboard input reference."));
}

void UPauseMenuWidget::HandleDisplay()
{
	FocusButton(1);
	SetDescription(NSLOCTEXT("PauseMenu", "DisplayDesc", "Display settings entry."));
}

void UPauseMenuWidget::HandleSound()
{
	FocusButton(2);
	SetDescription(NSLOCTEXT("PauseMenu", "SoundDesc", "Sound settings entry."));
}

void UPauseMenuWidget::HandleSave()
{
	FocusButton(3);
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
	FocusButton(4);

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

	switch (FocusedButtonIndex)
	{
	case 0:
		SetDescription(NSLOCTEXT("PauseMenu", "ControlDesc", "Controller and keyboard input reference."));
		break;
	case 1:
		SetDescription(NSLOCTEXT("PauseMenu", "DisplayDesc", "Display settings entry."));
		break;
	case 2:
		SetDescription(NSLOCTEXT("PauseMenu", "SoundDesc", "Sound settings entry."));
		break;
	case 3:
		SetDescription(NSLOCTEXT("PauseMenu", "SaveDesc", "Save the current run state."));
		break;
	case 4:
		SetDescription(NSLOCTEXT("PauseMenu", "QuitDesc", "Quit the game and back to the Main Menu."));
		break;
	default:
		break;
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
