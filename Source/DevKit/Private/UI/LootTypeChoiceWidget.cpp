#include "UI/LootTypeChoiceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "Map/RewardPickup.h"
#include "UI/YogInputKeyUtils.h"
#include "UI/YogUIManagerSubsystem.h"

static FText LootTypeChoice_ResolveLabel(const FLootTypeChoice& Choice)
{
	if (!Choice.DisplayName.IsEmpty())
	{
		return Choice.DisplayName;
	}

	switch (Choice.LootType)
	{
	case ELootType::Rune:
		return NSLOCTEXT("LootTypeChoice", "Rune", "Rune");
	case ELootType::Gold:
		return FText::Format(NSLOCTEXT("LootTypeChoice", "Gold", "Gold x{0}"), FMath::Max(1, Choice.Amount));
	case ELootType::Material:
		return FText::Format(NSLOCTEXT("LootTypeChoice", "Material", "Material x{0}"), FMath::Max(1, Choice.Amount));
	case ELootType::WeaponSkill:
		return NSLOCTEXT("LootTypeChoice", "WeaponSkill", "Weapon Skill");
	default:
		return FText::GetEmpty();
	}
}

static UTextBlock* LootTypeChoice_MakeText(UWidgetTree* Tree, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = Tree ? Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()) : nullptr;
	if (!TextBlock)
	{
		return nullptr;
	}

	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetJustification(ETextJustify::Center);
	return TextBlock;
}

void ULootTypeChoiceClickProxy::Init(ULootTypeChoiceWidget* InOwner, int32 InIndex)
{
	Owner = InOwner;
	ChoiceIndex = InIndex;
}

void ULootTypeChoiceClickProxy::HandleClicked()
{
	if (ULootTypeChoiceWidget* Widget = Owner.Get())
	{
		Widget->ConfirmChoice(ChoiceIndex);
	}
}

void ULootTypeChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();
	SetVisibility(ESlateVisibility::Collapsed);
}

TOptional<FUIInputConfig> ULootTypeChoiceWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void ULootTypeChoiceWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);
	FocusButton(FocusedButtonIndex);
}

void ULootTypeChoiceWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	Super::NativeOnDeactivated();
}

UWidget* ULootTypeChoiceWidget::NativeGetDesiredFocusTarget() const
{
	if (ChoiceButtons.IsValidIndex(FocusedButtonIndex) && ChoiceButtons[FocusedButtonIndex])
	{
		return ChoiceButtons[FocusedButtonIndex];
	}
	return ChoiceButtons.IsValidIndex(0) ? ChoiceButtons[0].Get() : nullptr;
}

void ULootTypeChoiceWidget::Setup(ARewardPickup* InPickup, APlayerCharacterBase* InPlayer)
{
	SourcePickup = InPickup;
	OwningPlayer = InPlayer;
	FocusedButtonIndex = 0;
	RebuildChoiceButtons();
	FocusButton(0);
}

void ULootTypeChoiceWidget::BuildFallbackLayout()
{
	if (!WidgetTree || ChoiceContainer)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LootTypeChoiceRoot"));
	WidgetTree->RootWidget = Root;

	if (UTextBlock* Title = LootTypeChoice_MakeText(WidgetTree,
		NSLOCTEXT("LootTypeChoice", "Title", "Choose your reward"), 28))
	{
		Root->AddChild(Title);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ChoiceContainer"));
	Root->AddChild(Row);
	ChoiceContainer = Row;
}

void ULootTypeChoiceWidget::RebuildChoiceButtons()
{
	ChoiceButtons.Reset();
	ClickProxies.Reset();

	const ARewardPickup* Pickup = SourcePickup.Get();
	if (!ChoiceContainer || !Pickup || !WidgetTree)
	{
		return;
	}

	ChoiceContainer->ClearChildren();

	const TArray<FLootTypeChoice>& Choices = Pickup->LootTypeChoices;
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		const FLootTypeChoice& Choice = Choices[Index];

		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Button->AddChild(Content);

		if (Choice.Icon)
		{
			if (UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass()))
			{
				Icon->SetBrushFromTexture(Choice.Icon);
				Icon->SetDesiredSizeOverride(FVector2D(64.0, 64.0));
				Content->AddChild(Icon);
			}
		}

		if (UTextBlock* Label = LootTypeChoice_MakeText(WidgetTree, LootTypeChoice_ResolveLabel(Choice), 20))
		{
			Content->AddChild(Label);
		}

		ULootTypeChoiceClickProxy* Proxy = NewObject<ULootTypeChoiceClickProxy>(this);
		Proxy->Init(this, Index);
		Button->OnClicked.AddDynamic(Proxy, &ULootTypeChoiceClickProxy::HandleClicked);

		ChoiceContainer->AddChild(Button);
		if (UHorizontalBoxSlot* BoxSlot = Cast<UHorizontalBoxSlot>(Button->Slot))
		{
			BoxSlot->SetPadding(FMargin(12.f));
		}

		ChoiceButtons.Add(Button);
		ClickProxies.Add(Proxy);
	}
}

void ULootTypeChoiceWidget::FocusButton(int32 NewIndex)
{
	if (ChoiceButtons.IsEmpty())
	{
		return;
	}

	FocusedButtonIndex = (NewIndex % ChoiceButtons.Num() + ChoiceButtons.Num()) % ChoiceButtons.Num();
	for (int32 Index = 0; Index < ChoiceButtons.Num(); ++Index)
	{
		if (UButton* Button = ChoiceButtons[Index])
		{
			Button->SetBackgroundColor(Index == FocusedButtonIndex
				? FLinearColor(0.16f, 0.42f, 0.82f, 0.85f)
				: FLinearColor::White);
		}
	}

	if (UButton* Focused = ChoiceButtons[FocusedButtonIndex])
	{
		Focused->SetKeyboardFocus();
	}
}

void ULootTypeChoiceWidget::ActivateFocusedButton()
{
	if (ChoiceButtons.IsValidIndex(FocusedButtonIndex) && ChoiceButtons[FocusedButtonIndex])
	{
		ChoiceButtons[FocusedButtonIndex]->OnClicked.Broadcast();
	}
}

FReply ULootTypeChoiceWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key) || YogInputKeys::IsMenuKey(Key))
	{
		CancelChoice();
		return FReply::Handled();
	}

	int32 NavDirection = YogInputKeys::GetHorizontalNavigationDirection(Key);
	if (NavDirection == 0)
	{
		NavDirection = YogInputKeys::GetVerticalNavigationDirection(Key);
	}
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

void ULootTypeChoiceWidget::ConfirmChoice(int32 Index)
{
	ARewardPickup* Pickup = SourcePickup.Get();
	APlayerCharacterBase* Player = OwningPlayer.Get();
	SourcePickup.Reset();

	// Close first: a Rune choice pushes the LootSelection screen, and two paused menu screens
	// activating in the same call would fight over the input mode.
	CloseScreen();

	if (Pickup)
	{
		Pickup->ResolveLootTypeChoice(Index, Player);
	}
}

void ULootTypeChoiceWidget::CancelChoice()
{
	if (ARewardPickup* Pickup = SourcePickup.Get())
	{
		Pickup->ResetForSkip(OwningPlayer.Get());
	}
	SourcePickup.Reset();

	CloseScreen();
}

void ULootTypeChoiceWidget::CloseScreen()
{
	if (!UYogUIManagerSubsystem::PopManagedScreen(this, EYogUIScreenId::LootTypeChoice))
	{
		DeactivateWidget();
	}
}
