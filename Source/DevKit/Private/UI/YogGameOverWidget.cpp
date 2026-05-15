#include "UI/YogGameOverWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameModes/YogGameMode.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "System/YogGameInstanceBase.h"
#include "TimerManager.h"
#include "UI/YogInputKeyUtils.h"
#include "UI/YogUIManagerSubsystem.h"

namespace
{
UTextBlock* MakeGameOverText(UWidgetTree* Tree, const FName Name, const FText& Text, int32 FontSize)
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
	TextBlock->SetJustification(ETextJustify::Center);
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}

UButton* MakeGameOverButton(UWidgetTree* Tree, const FName Name, const FText& Text)
{
	UButton* Button = Tree ? Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name) : nullptr;
	if (!Button)
	{
		return nullptr;
	}

	UTextBlock* Label = MakeGameOverText(Tree, FName(*FString::Printf(TEXT("%s_Label"), *Name.ToString())), Text, 23);
	if (Label)
	{
		Label->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.92f, 0.95f, 1.f)));
		Button->SetContent(Label);
	}

	return Button;
}
}

void UYogGameOverWidget::Setup(bool bInCanRevive)
{
	bCanRevive = bInCanRevive;
	if (BtnRevive)
	{
		BtnRevive->SetVisibility(bCanRevive ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	FocusedButtonIndex = 0;
	CacheButtons();
	FocusButton(FocusedButtonIndex);
}

void UYogGameOverWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();
	SetVisibility(ESlateVisibility::Collapsed);
	SetIsFocusable(true);

	if (BtnRevive)
	{
		BtnRevive->OnClicked.RemoveDynamic(this, &UYogGameOverWidget::HandleReviveClicked);
		BtnRevive->OnClicked.AddDynamic(this, &UYogGameOverWidget::HandleReviveClicked);
		BtnRevive->OnHovered.RemoveDynamic(this, &UYogGameOverWidget::HandleReviveHovered);
		BtnRevive->OnHovered.AddDynamic(this, &UYogGameOverWidget::HandleReviveHovered);
	}
	if (BtnRetry)
	{
		BtnRetry->OnClicked.RemoveDynamic(this, &UYogGameOverWidget::HandleRetryClicked);
		BtnRetry->OnClicked.AddDynamic(this, &UYogGameOverWidget::HandleRetryClicked);
		BtnRetry->OnHovered.RemoveDynamic(this, &UYogGameOverWidget::HandleRetryHovered);
		BtnRetry->OnHovered.AddDynamic(this, &UYogGameOverWidget::HandleRetryHovered);
	}
	if (BtnReturnToTitle)
	{
		BtnReturnToTitle->OnClicked.RemoveDynamic(this, &UYogGameOverWidget::HandleReturnToTitleClicked);
		BtnReturnToTitle->OnClicked.AddDynamic(this, &UYogGameOverWidget::HandleReturnToTitleClicked);
		BtnReturnToTitle->OnHovered.RemoveDynamic(this, &UYogGameOverWidget::HandleReturnToTitleHovered);
		BtnReturnToTitle->OnHovered.AddDynamic(this, &UYogGameOverWidget::HandleReturnToTitleHovered);
	}

	CacheButtons();
}

void UYogGameOverWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);
	AcceptInputUnlockTime = GetWorld() ? GetWorld()->GetRealTimeSeconds() + 0.35f : 0.f;
	CacheButtons();
	FocusButton(FocusedButtonIndex);
}

void UYogGameOverWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeOnDeactivated();
}

FReply UYogGameOverWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key))
	{
		if (MenuButtons.Num() > 0)
		{
			FocusButton(MenuButtons.Num() - 1);
		}
		return FReply::Handled();
	}

	const int32 VerticalDirection = YogInputKeys::GetVerticalNavigationDirection(Key);
	if (VerticalDirection != 0)
	{
		FocusButton(FocusedButtonIndex + VerticalDirection);
		return FReply::Handled();
	}

	if (YogInputKeys::IsAcceptKey(Key) || Key == EKeys::SpaceBar)
	{
		if (IsAcceptInputLocked())
		{
			return FReply::Handled();
		}
		ActivateFocusedButton();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UYogGameOverWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
{
	const FKey Key = InAnalogInputEvent.GetKey();
	const float Value = InAnalogInputEvent.GetAnalogValue();
	if (Key == EKeys::Gamepad_LeftY && FMath::Abs(Value) >= 0.65f)
	{
		const float Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.f;
		if (Now - LastAnalogNavigationTime >= 0.20f)
		{
			LastAnalogNavigationTime = Now;
			FocusButton(FocusedButtonIndex + (Value > 0.f ? -1 : 1));
			return FReply::Handled();
		}
	}

	return Super::NativeOnAnalogValueChanged(InGeometry, InAnalogInputEvent);
}

TOptional<FUIInputConfig> UYogGameOverWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

UWidget* UYogGameOverWidget::NativeGetDesiredFocusTarget() const
{
	return MenuButtons.IsValidIndex(FocusedButtonIndex) ? MenuButtons[FocusedButtonIndex].Get() : BtnRetry.Get();
}

void UYogGameOverWidget::BuildFallbackLayout()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("GameOverRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* Shade = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Shade"));
	Shade->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.82f));
	Root->AddChildToOverlay(Shade);

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GameOverPanel"));
	Panel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.022f, 0.90f));
	Panel->SetPadding(FMargin(42.f, 34.f));
	if (UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
	Panel->AddChild(RootBox);

	TitleText = MakeGameOverText(WidgetTree, TEXT("TitleText"), NSLOCTEXT("GameOver", "Title", "Game Over"), 48);
	if (TitleText)
	{
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.28f, 0.22f, 1.f)));
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	SubtitleText = MakeGameOverText(WidgetTree, TEXT("SubtitleText"), NSLOCTEXT("GameOver", "Subtitle", "Your run has ended."), 18);
	if (SubtitleText)
	{
		SubtitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.70f, 0.76f, 1.f)));
		if (UVerticalBoxSlot* SubtitleSlot = RootBox->AddChildToVerticalBox(SubtitleText))
		{
			SubtitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 26.f));
			SubtitleSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	ButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonBox"));
	RootBox->AddChildToVerticalBox(ButtonBox);

	BtnRevive = MakeGameOverButton(WidgetTree, TEXT("BtnRevive"), NSLOCTEXT("GameOver", "Revive", "Revive"));
	BtnRetry = MakeGameOverButton(WidgetTree, TEXT("BtnRetry"), NSLOCTEXT("GameOver", "Retry", "Try Again"));
	BtnReturnToTitle = MakeGameOverButton(WidgetTree, TEXT("BtnReturnToTitle"), NSLOCTEXT("GameOver", "ReturnToTitle", "Return to Title"));

	for (UButton* Button : { BtnRevive.Get(), BtnRetry.Get(), BtnReturnToTitle.Get() })
	{
		if (!Button)
		{
			continue;
		}
		if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, Button == BtnReturnToTitle ? 0.f : 10.f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}
}

void UYogGameOverWidget::CacheButtons()
{
	MenuButtons.Reset();
	if (bCanRevive && BtnRevive && BtnRevive->GetVisibility() != ESlateVisibility::Collapsed)
	{
		MenuButtons.Add(BtnRevive);
	}
	if (BtnRetry)
	{
		MenuButtons.Add(BtnRetry);
	}
	if (BtnReturnToTitle)
	{
		MenuButtons.Add(BtnReturnToTitle);
	}

	FocusedButtonIndex = FMath::Clamp(FocusedButtonIndex, 0, FMath::Max(0, MenuButtons.Num() - 1));
	RefreshButtonVisuals();
}

void UYogGameOverWidget::FocusButton(int32 NewIndex)
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

void UYogGameOverWidget::ActivateFocusedButton()
{
	if (IsAcceptInputLocked())
	{
		return;
	}

	if (MenuButtons.IsValidIndex(FocusedButtonIndex))
	{
		if (UButton* Button = MenuButtons[FocusedButtonIndex])
		{
			Button->OnClicked.Broadcast();
		}
	}
}

void UYogGameOverWidget::RefreshButtonVisuals()
{
	for (int32 Index = 0; Index < MenuButtons.Num(); ++Index)
	{
		if (UButton* Button = MenuButtons[Index])
		{
			Button->SetBackgroundColor(Index == FocusedButtonIndex
				? FLinearColor(0.52f, 0.12f, 0.10f, 0.92f)
				: FLinearColor(0.05f, 0.06f, 0.07f, 0.72f));
		}
	}
}

void UYogGameOverWidget::CloseManagedScreen()
{
	if (!UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::GameOver))
	{
		DeactivateWidget();
	}
}

bool UYogGameOverWidget::IsAcceptInputLocked() const
{
	return GetWorld() && GetWorld()->GetRealTimeSeconds() < AcceptInputUnlockTime;
}

void UYogGameOverWidget::HandleReviveClicked()
{
	if (IsAcceptInputLocked())
	{
		return;
	}

	if (!bCanRevive)
	{
		return;
	}

	UWorld* World = GetWorld();
	TWeakObjectPtr<AYogGameMode> WeakGameMode(Cast<AYogGameMode>(UGameplayStatics::GetGameMode(this)));
	if (!World || !WeakGameMode.IsValid())
	{
		return;
	}

	CloseManagedScreen();
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([WeakGameMode]()
	{
		if (AYogGameMode* GM = WeakGameMode.Get())
		{
			GM->RevivePlayerFromDeath();
		}
	}));
}

void UYogGameOverWidget::HandleReviveHovered()
{
	const int32 Index = MenuButtons.IndexOfByKey(BtnRevive);
	if (Index != INDEX_NONE)
	{
		FocusButton(Index);
	}
}

void UYogGameOverWidget::HandleRetryClicked()
{
	if (IsAcceptInputLocked())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] Retry selected."));
	CloseManagedScreen();
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	if (UYogGameInstanceBase* GI = GetGameInstance<UYogGameInstanceBase>())
	{
		GI->StartNewRunFromFrontend();
	}
}

void UYogGameOverWidget::HandleRetryHovered()
{
	const int32 Index = MenuButtons.IndexOfByKey(BtnRetry);
	if (Index != INDEX_NONE)
	{
		FocusButton(Index);
	}
}

void UYogGameOverWidget::HandleReturnToTitleClicked()
{
	if (IsAcceptInputLocked())
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[GameOver] ReturnToTitle selected."));
	CloseManagedScreen();
	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::SetGamePaused(World, false);
	}
	if (UYogGameInstanceBase* GI = GetGameInstance<UYogGameInstanceBase>())
	{
		GI->ReturnToMainMenu();
	}
}

void UYogGameOverWidget::HandleReturnToTitleHovered()
{
	const int32 Index = MenuButtons.IndexOfByKey(BtnReturnToTitle);
	if (Index != INDEX_NONE)
	{
		FocusButton(Index);
	}
}
