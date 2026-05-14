#include "UI/YogEntryMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "UI/YogInputKeyUtils.h"

namespace
{
UTextBlock* MakeEntryText(UWidgetTree* Tree, const FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = Tree ? Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name) : nullptr;
	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.84f, 0.86f, 1.f)));
	TextBlock->SetJustification(ETextJustify::Center);
	return TextBlock;
}

UButton* MakeEntryButton(UWidgetTree* Tree, const FName Name, const FText& Text)
{
	UButton* Button = Tree ? Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name) : nullptr;
	if (!Button)
	{
		return nullptr;
	}

	UTextBlock* Label = MakeEntryText(Tree, FName(*FString::Printf(TEXT("%s_Label"), *Name.ToString())), Text, 26);
	if (Label)
	{
		Button->SetContent(Label);
	}

	return Button;
}
}

void UYogEntryMenuWidget::SetBackgroundTexture(UTexture2D* InTexture)
{
	BackgroundTexture = InTexture;
	if (BackgroundImage && BackgroundTexture)
	{
		FSlateBrush BackgroundBrush;
		BackgroundBrush.SetResourceObject(BackgroundTexture);
		BackgroundBrush.ImageSize = FVector2D(BackgroundTexture->GetSizeX(), BackgroundTexture->GetSizeY());
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
		BackgroundImage->SetBrush(BackgroundBrush);
	}
}

void UYogEntryMenuWidget::FocusQuit()
{
	CacheButtons();
	if (MenuButtons.Num() > 0)
	{
		FocusButton(MenuButtons.Num() - 1);
	}
}

void UYogEntryMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();
	CacheButtons();
	SetVisibility(ESlateVisibility::Collapsed);
	SetIsFocusable(true);

	if (BtnStart)
	{
		BtnStart->OnClicked.RemoveDynamic(this, &UYogEntryMenuWidget::HandleStartClicked);
		BtnStart->OnClicked.AddDynamic(this, &UYogEntryMenuWidget::HandleStartClicked);
	}
	if (BtnContinue)
	{
		BtnContinue->OnClicked.RemoveDynamic(this, &UYogEntryMenuWidget::HandleContinueClicked);
		BtnContinue->OnClicked.AddDynamic(this, &UYogEntryMenuWidget::HandleContinueClicked);
	}
	if (BtnOptions)
	{
		BtnOptions->OnClicked.RemoveDynamic(this, &UYogEntryMenuWidget::HandleOptionsClicked);
		BtnOptions->OnClicked.AddDynamic(this, &UYogEntryMenuWidget::HandleOptionsClicked);
	}
	if (BtnQuit)
	{
		BtnQuit->OnClicked.RemoveDynamic(this, &UYogEntryMenuWidget::HandleQuitClicked);
		BtnQuit->OnClicked.AddDynamic(this, &UYogEntryMenuWidget::HandleQuitClicked);
	}
}

void UYogEntryMenuWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);
	CacheButtons();
	FocusButton(FocusedButtonIndex);
}

void UYogEntryMenuWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeOnDeactivated();
}

FReply UYogEntryMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key))
	{
		FocusQuit();
		return FReply::Handled();
	}

	const int32 VerticalDirection = YogInputKeys::GetVerticalNavigationDirection(Key);
	if (VerticalDirection != 0)
	{
		MoveFocus(VerticalDirection);
		return FReply::Handled();
	}

	if (YogInputKeys::IsAcceptKey(Key) || Key == EKeys::SpaceBar)
	{
		ActivateFocusedButton();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UYogEntryMenuWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();
	if (Key == EKeys::Gamepad_LeftY && FMath::Abs(Value) >= 0.65f)
	{
		const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		if (Now - LastAnalogNavigationTime >= 0.24f)
		{
			LastAnalogNavigationTime = Now;
			MoveFocus(Value > 0.f ? -1 : 1);
			return FReply::Handled();
		}
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
}

TOptional<FUIInputConfig> UYogEntryMenuWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* UYogEntryMenuWidget::NativeGetDesiredFocusTarget() const
{
	return MenuButtons.IsValidIndex(FocusedButtonIndex) ? MenuButtons[FocusedButtonIndex].Get() : BtnStart.Get();
}

void UYogEntryMenuWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("EntryRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* BackgroundColor = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundColor"));
	BackgroundColor->SetBrushColor(FLinearColor(0.02f, 0.018f, 0.015f, 1.f));
	Root->AddChildToOverlay(BackgroundColor);

	UScaleBox* BackgroundScale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("BackgroundScale"));
	BackgroundScale->SetStretch(EStretch::ScaleToFill);
	BackgroundImage = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BackgroundImage"));
	BackgroundImage->SetBrushColor(FLinearColor::White);
	BackgroundScale->AddChild(BackgroundImage);
	Root->AddChildToOverlay(BackgroundScale);
	SetBackgroundTexture(BackgroundTexture);

	UBorder* OverlayShade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OverlayShade"));
	OverlayShade->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.16f));
	Root->AddChildToOverlay(OverlayShade);

	UBorder* MenuPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MenuPanel"));
	MenuPanel->SetBrushColor(FLinearColor(0.005f, 0.007f, 0.009f, 0.58f));
	MenuPanel->SetPadding(FMargin(24.f, 18.f));
	if (UOverlaySlot* MenuSlot = Root->AddChildToOverlay(MenuPanel))
	{
		MenuSlot->SetHorizontalAlignment(HAlign_Left);
		MenuSlot->SetVerticalAlignment(VAlign_Bottom);
		MenuSlot->SetPadding(FMargin(96.f, 0.f, 0.f, 132.f));
	}

	UVerticalBox* MenuBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuBox"));
	MenuPanel->AddChild(MenuBox);

	BtnStart = MakeEntryButton(WidgetTree, TEXT("BtnStart"), NSLOCTEXT("DevKitFrontend", "StartGame", "Start"));
	BtnContinue = MakeEntryButton(WidgetTree, TEXT("BtnContinue"), NSLOCTEXT("DevKitFrontend", "ContinueGame", "Continue"));
	BtnOptions = MakeEntryButton(WidgetTree, TEXT("BtnOptions"), NSLOCTEXT("DevKitFrontend", "Options", "Options"));
	BtnQuit = MakeEntryButton(WidgetTree, TEXT("BtnQuit"), NSLOCTEXT("DevKitFrontend", "QuitGame", "Quit"));

	for (UButton* Button : { BtnStart.Get(), BtnContinue.Get(), BtnOptions.Get(), BtnQuit.Get() })
	{
		if (!Button)
		{
			continue;
		}
		if (UVerticalBoxSlot* ButtonSlot = MenuBox->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Button == BtnQuit ? 0.f : 8.f));
		}
	}

	UBorder* PromptPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PromptPanel"));
	PromptPanel->SetBrushColor(FLinearColor(0.f, 0.01f, 0.015f, 0.52f));
	PromptPanel->SetPadding(FMargin(14.f, 7.f));
	if (UTextBlock* PromptText = MakeEntryText(WidgetTree, TEXT("PromptText"), NSLOCTEXT("DevKitFrontend", "MainMenuInputPrompt", "A Select    D-Pad / Left Stick Navigate"), 15))
	{
		PromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.78f, 0.86f, 0.92f)));
		PromptPanel->AddChild(PromptText);
	}
	if (UOverlaySlot* PromptSlot = Root->AddChildToOverlay(PromptPanel))
	{
		PromptSlot->SetHorizontalAlignment(HAlign_Left);
		PromptSlot->SetVerticalAlignment(VAlign_Bottom);
		PromptSlot->SetPadding(FMargin(34.f, 0.f, 0.f, 28.f));
	}
}

void UYogEntryMenuWidget::CacheButtons()
{
	MenuButtons.Reset();
	if (BtnStart)
	{
		MenuButtons.Add(BtnStart);
	}
	if (BtnContinue)
	{
		MenuButtons.Add(BtnContinue);
	}
	if (BtnOptions)
	{
		MenuButtons.Add(BtnOptions);
	}
	if (BtnQuit)
	{
		MenuButtons.Add(BtnQuit);
	}

	FocusedButtonIndex = FMath::Clamp(FocusedButtonIndex, 0, FMath::Max(0, MenuButtons.Num() - 1));
}

void UYogEntryMenuWidget::MoveFocus(int32 Direction)
{
	if (MenuButtons.IsEmpty())
	{
		return;
	}
	FocusButton(FocusedButtonIndex + Direction);
}

void UYogEntryMenuWidget::FocusButton(int32 NewIndex)
{
	if (MenuButtons.IsEmpty())
	{
		return;
	}

	FocusedButtonIndex = (NewIndex % MenuButtons.Num() + MenuButtons.Num()) % MenuButtons.Num();
	RefreshButtonVisuals();
	if (UButton* Button = MenuButtons[FocusedButtonIndex])
	{
		Button->SetKeyboardFocus();
	}
}

void UYogEntryMenuWidget::ActivateFocusedButton()
{
	if (MenuButtons.IsValidIndex(FocusedButtonIndex))
	{
		if (UButton* Button = MenuButtons[FocusedButtonIndex])
		{
			Button->OnClicked.Broadcast();
		}
	}
}

void UYogEntryMenuWidget::RefreshButtonVisuals()
{
	for (int32 Index = 0; Index < MenuButtons.Num(); ++Index)
	{
		if (UButton* Button = MenuButtons[Index])
		{
			Button->SetBackgroundColor(Index == FocusedButtonIndex
				? FLinearColor(0.32f, 0.47f, 0.58f, 0.74f)
				: FLinearColor(0.04f, 0.08f, 0.12f, 0.36f));
		}
	}
}

void UYogEntryMenuWidget::HandleStartClicked()
{
	OnStartRequested.Broadcast();
}

void UYogEntryMenuWidget::HandleContinueClicked()
{
	OnContinueRequested.Broadcast();
}

void UYogEntryMenuWidget::HandleOptionsClicked()
{
	OnOptionsRequested.Broadcast();
}

void UYogEntryMenuWidget::HandleQuitClicked()
{
	OnQuitRequested.Broadcast();
}
