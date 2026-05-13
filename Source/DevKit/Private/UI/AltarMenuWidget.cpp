#include "UI/AltarMenuWidget.h"
#include "UI/RunePurificationWidget.h"
#include "UI/SacrificeSelectionWidget.h"
#include "UI/YogHUD.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Button.h"
#include "Engine/LocalPlayer.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "UI/YogCommonUITextBlock.h"
#include "UI/YogInputKeyUtils.h"
#include "UI/YogUIManagerSubsystem.h"

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
	// FocusButton handles per-button focus; SetUserFocus(player) was targeting the wrong root.
	FocusButton(FocusedButtonIndex);
}

void UAltarMenuWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
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

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				if (PurificationWidgetClass)
				{
					UIManager->SetWidgetClassOverride(EYogUIScreenId::RunePurification, PurificationWidgetClass);
				}
				if (SacrificeWidgetClass)
				{
					UIManager->SetWidgetClassOverride(EYogUIScreenId::SacrificeSelection, SacrificeWidgetClass);
				}
			}
		}
	}
}

void UAltarMenuWidget::OpenPurification()
{
	if (!OwningPlayer.IsValid()) return;

	URunePurificationWidget* ManagedWidget = nullptr;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				ManagedWidget = Cast<URunePurificationWidget>(UIManager->EnsureWidget(EYogUIScreenId::RunePurification));
				if (ManagedWidget)
				{
					ManagedWidget->Setup(OwningPlayer.Get());
					UIManager->PushScreen(EYogUIScreenId::RunePurification);
					UIManager->PopScreen(EYogUIScreenId::AltarMenu);
					return;
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AltarMenu] RunePurification class is missing."));
}

void UAltarMenuWidget::OpenSacrifice()
{
	if (!AltarData || !OwningPlayer.IsValid()) return;

	USacrificeSelectionWidget* ManagedWidget = nullptr;
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UYogUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UYogUIManagerSubsystem>())
			{
				ManagedWidget = Cast<USacrificeSelectionWidget>(UIManager->EnsureWidget(EYogUIScreenId::SacrificeSelection));
				if (ManagedWidget)
				{
					ManagedWidget->Setup(AltarData, OwningPlayer.Get());
					UIManager->PushScreen(EYogUIScreenId::SacrificeSelection);
					UIManager->PopScreen(EYogUIScreenId::AltarMenu);
					return;
				}
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AltarMenu] SacrificeSelection class is missing."));
}

void UAltarMenuWidget::CloseMenu()
{
	if (!UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::AltarMenu))
	{
		DeactivateWidget();
	}
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
