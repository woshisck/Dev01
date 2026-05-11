#include "UI/SacrificeSelectionWidget.h"

#include "Algo/RandomShuffle.h"
#include "Blueprint/WidgetTree.h"
#include "Character/PlayerCharacterBase.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Data/RuneDataAsset.h"
#include "Input/CommonUIInputTypes.h"
#include "InputCoreTypes.h"
#include "Map/AltarActor.h"
#include "UI/YogHUD.h"
#include "UI/YogCommonUITextBlock.h"
#include "UI/YogInputKeyUtils.h"

namespace
{
FText GetSacrificeRuneName(const FAltarSacrificeEntry& Entry)
{
	if (!Entry.GrantedRune)
	{
		return NSLOCTEXT("SacrificeSelection", "MissingRune", "献祭符文");
	}
	return FText::FromName(Entry.GrantedRune->GetRuneName());
}

FText GetCardDisplayName(const FCombatCardInstance& Card)
{
	if (!Card.SourceData)
	{
		return NSLOCTEXT("SacrificeSelection", "MissingCard", "未知卡牌");
	}
	return FText::FromName(Card.SourceData->GetRuneName());
}

FText GetCostFallbackText(ESacrificeOfferingCostType CostType)
{
	switch (CostType)
	{
	case ESacrificeOfferingCostType::SacrificeDeckCard:
		return NSLOCTEXT("SacrificeSelection", "CostDeck", "献祭卡组中的一张卡牌");
	case ESacrificeOfferingCostType::AttackUpDamageTakenUp:
		return NSLOCTEXT("SacrificeSelection", "CostAttack", "攻击力增加 15%，受到的伤害增加 20%");
	case ESacrificeOfferingCostType::CritRateDownCritDamageUp:
		return NSLOCTEXT("SacrificeSelection", "CostCrit", "暴击概率降低 50%，暴击伤害增加 50%");
	}
	return FText::GetEmpty();
}

UTextBlock* MakeText(UWidgetTree* Tree, const FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = Tree->ConstructWidget<UYogCommonUITextBlock>(UYogCommonUITextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	TextBlock->SetAutoWrapText(true);
	return TextBlock;
}

UButton* MakeTextButton(UWidgetTree* Tree, const FName Name, const FText& Text)
{
	UButton* Button = Tree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	const FName LabelName(*FString::Printf(TEXT("%s_Label"), *Name.ToString()));
	UTextBlock* Label = MakeText(Tree, LabelName, Text, 18);
	Label->SetJustification(ETextJustify::Center);
	Button->AddChild(Label);
	return Button;
}
}

TSubclassOf<UTextBlock> USacrificeSelectionWidget::GetMenuTextBlockClassForTests()
{
	return UYogCommonUITextBlock::StaticClass();
}

void USacrificeSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildFallbackLayout();

	if (OptionButton0) OptionButton0->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnOption0Clicked);
	if (OptionButton1) OptionButton1->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnOption1Clicked);
	if (OptionButton2) OptionButton2->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnOption2Clicked);
	if (DeckButton0) DeckButton0->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck0Clicked);
	if (DeckButton1) DeckButton1->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck1Clicked);
	if (DeckButton2) DeckButton2->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck2Clicked);
	if (DeckButton3) DeckButton3->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck3Clicked);
	if (DeckButton4) DeckButton4->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck4Clicked);
	if (DeckButton5) DeckButton5->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck5Clicked);
	if (DeckButton6) DeckButton6->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck6Clicked);
	if (DeckButton7) DeckButton7->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnDeck7Clicked);
	if (BtnConfirm) BtnConfirm->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnConfirmClicked);
	if (BtnCancel) BtnCancel->OnClicked.AddDynamic(this, &USacrificeSelectionWidget::OnCancelClicked);

	SetVisibility(ESlateVisibility::Collapsed);
}

TOptional<FUIInputConfig> USacrificeSelectionWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

void USacrificeSelectionWidget::NativeOnActivated()
{
	Super::NativeOnActivated();
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetIsFocusable(true);

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->BeginPauseEffect();
		}

		PC->SetShowMouseCursor(true);
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(GetCachedWidget());
		PC->SetInputMode(InputMode);
	}
	SetUserFocus(GetOwningPlayer());
	FocusButton(FocusedButtonIndex);
}

void USacrificeSelectionWidget::NativeOnDeactivated()
{
	SetVisibility(ESlateVisibility::Collapsed);
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());
		if (AYogHUD* HUD = Cast<AYogHUD>(PC->GetHUD()))
		{
			HUD->EndPauseEffect();
		}
	}
	Super::NativeOnDeactivated();
}

FReply USacrificeSelectionWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (YogInputKeys::IsBackKey(Key) || YogInputKeys::IsMenuKey(Key))
	{
		CancelSacrifice();
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

FReply USacrificeSelectionWidget::NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent)
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

void USacrificeSelectionWidget::Setup(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer, AAltarActor* InSourceAltar)
{
	AltarData = InData;
	OwningPlayer = InPlayer;
	SourceAltar = InSourceAltar;
	Phase = 0;
	SelectedOptionIndex = INDEX_NONE;
	SelectedDeckCardIndex = INDEX_NONE;
	CurrentOptions.Reset();
	FocusedButtonIndex = 0;

	if (!InData || !InData->bSacrificeEnabled)
	{
		RefreshNativeView();
		return;
	}

	TArray<FAltarSacrificeEntry> Pool = InData->SacrificeRunePool;
	for (FAltarSacrificeEntry& Entry : Pool)
	{
		if (!Entry.GrantedRune)
		{
			Entry.GrantedRune = InData->EventSacrificeRune;
		}
		if (Entry.CostDescription.IsEmpty())
		{
			Entry.CostDescription = GetCostFallbackText(Entry.CostType);
		}
	}

	Algo::RandomShuffle(Pool);
	for (const FAltarSacrificeEntry& Entry : Pool)
	{
		if (!Entry.GrantedRune)
		{
			continue;
		}
		CurrentOptions.Add(Entry);
		if (CurrentOptions.Num() >= 3)
		{
			break;
		}
	}

	OnShowSacrificeOptions(CurrentOptions);
	RefreshNativeView();
	FocusButton(FocusedButtonIndex);
}

void USacrificeSelectionWidget::SelectSacrificeOption(int32 OptionIndex)
{
	if (Phase != 0 || !CurrentOptions.IsValidIndex(OptionIndex))
	{
		return;
	}

	SelectedOptionIndex = OptionIndex;
	SelectedDeckCardIndex = INDEX_NONE;
	const FAltarSacrificeEntry& Entry = CurrentOptions[OptionIndex];

	if (Entry.CostType == ESacrificeOfferingCostType::SacrificeDeckCard)
	{
		const TArray<FCombatCardInstance> DeckCards = GetDeckCards();
		if (DeckCards.IsEmpty())
		{
			FailSacrifice(NSLOCTEXT("SacrificeSelection", "NoDeckCards", "当前卡组没有可献祭的卡牌。"));
			return;
		}

		Phase = 2;
		FocusedButtonIndex = 0;
		OnShowDeckCardSelection(DeckCards);
	}
	else
	{
		Phase = 1;
		FocusedButtonIndex = 0;
		OnShowCostConfirmation(Entry);
	}

	RefreshNativeView();
}

void USacrificeSelectionWidget::SelectDeckCardForSacrifice(int32 CardIndex)
{
	if (Phase != 2)
	{
		return;
	}

	const TArray<FCombatCardInstance> DeckCards = GetDeckCards();
	if (!DeckCards.IsValidIndex(CardIndex))
	{
		return;
	}

	SelectedDeckCardIndex = CardIndex;
	Phase = 1;
	FocusedButtonIndex = 0;
	OnShowCostConfirmation(CurrentOptions[SelectedOptionIndex]);
	RefreshNativeView();
}

void USacrificeSelectionWidget::ConfirmSacrifice()
{
	if (Phase != 1 || !CurrentOptions.IsValidIndex(SelectedOptionIndex) || !OwningPlayer.IsValid())
	{
		return;
	}

	if (!PaySelectedCost())
	{
		FailSacrifice(NSLOCTEXT("SacrificeSelection", "CostFailed", "献祭代价未能支付。"));
		return;
	}

	if (!GrantSelectedRune())
	{
		FailSacrifice(NSLOCTEXT("SacrificeSelection", "GrantFailed", "献祭符文未能授予。"));
		return;
	}

	OnSacrificeFinished(true);
	if (AAltarActor* Altar = SourceAltar.Get())
	{
		Altar->ConsumeSacrificeReward();
	}
	DeactivateWidget();
}

void USacrificeSelectionWidget::CancelSacrifice()
{
	if (Phase == 2)
	{
		Phase = 0;
		SelectedOptionIndex = INDEX_NONE;
		SelectedDeckCardIndex = INDEX_NONE;
		FocusedButtonIndex = 0;
		OnShowSacrificeOptions(CurrentOptions);
		RefreshNativeView();
		return;
	}

	if (Phase == 1)
	{
		Phase = 0;
		SelectedOptionIndex = INDEX_NONE;
		SelectedDeckCardIndex = INDEX_NONE;
		FocusedButtonIndex = 0;
		OnShowSacrificeOptions(CurrentOptions);
		RefreshNativeView();
	}
	else
	{
		OnSacrificeFinished(false);
		DeactivateWidget();
	}
}

void USacrificeSelectionWidget::BuildFallbackLayout()
{
	if (!WidgetTree || OptionBox)
	{
		return;
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SacrificeRoot"));
	WidgetTree->RootWidget = Root;

	TitleText = MakeText(WidgetTree, TEXT("TitleText"), NSLOCTEXT("SacrificeSelection", "Title", "献祭圣坛"), 24);
	Root->AddChildToVerticalBox(TitleText);

	DescriptionText = MakeText(WidgetTree, TEXT("DescriptionText"), NSLOCTEXT("SacrificeSelection", "Desc", "选择一个代价，获得全局献祭符文。"), 16);
	Root->AddChildToVerticalBox(DescriptionText);

	OptionBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OptionBox"));
	Root->AddChildToVerticalBox(OptionBox);

	OptionButton0 = MakeTextButton(WidgetTree, TEXT("OptionButton0"), FText::GetEmpty());
	OptionButton1 = MakeTextButton(WidgetTree, TEXT("OptionButton1"), FText::GetEmpty());
	OptionButton2 = MakeTextButton(WidgetTree, TEXT("OptionButton2"), FText::GetEmpty());
	OptionBox->AddChildToVerticalBox(OptionButton0);
	OptionBox->AddChildToVerticalBox(OptionButton1);
	OptionBox->AddChildToVerticalBox(OptionButton2);

	DeckCardBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeckCardBox"));
	Root->AddChildToVerticalBox(DeckCardBox);

	DeckButton0 = MakeTextButton(WidgetTree, TEXT("DeckButton0"), FText::GetEmpty());
	DeckButton1 = MakeTextButton(WidgetTree, TEXT("DeckButton1"), FText::GetEmpty());
	DeckButton2 = MakeTextButton(WidgetTree, TEXT("DeckButton2"), FText::GetEmpty());
	DeckButton3 = MakeTextButton(WidgetTree, TEXT("DeckButton3"), FText::GetEmpty());
	DeckButton4 = MakeTextButton(WidgetTree, TEXT("DeckButton4"), FText::GetEmpty());
	DeckButton5 = MakeTextButton(WidgetTree, TEXT("DeckButton5"), FText::GetEmpty());
	DeckButton6 = MakeTextButton(WidgetTree, TEXT("DeckButton6"), FText::GetEmpty());
	DeckButton7 = MakeTextButton(WidgetTree, TEXT("DeckButton7"), FText::GetEmpty());
	for (UButton* DeckButton : GetDeckButtons())
	{
		DeckCardBox->AddChildToVerticalBox(DeckButton);
	}

	CostText = MakeText(WidgetTree, TEXT("CostText"), FText::GetEmpty(), 16);
	Root->AddChildToVerticalBox(CostText);

	BtnConfirm = MakeTextButton(WidgetTree, TEXT("BtnConfirm"), NSLOCTEXT("SacrificeSelection", "Confirm", "确认献祭"));
	BtnCancel = MakeTextButton(WidgetTree, TEXT("BtnCancel"), NSLOCTEXT("SacrificeSelection", "Cancel", "取消"));
	Root->AddChildToVerticalBox(BtnConfirm);
	Root->AddChildToVerticalBox(BtnCancel);
}

void USacrificeSelectionWidget::RefreshNativeView()
{
	if (TitleText)
	{
		TitleText->SetText(NSLOCTEXT("SacrificeSelection", "Title", "献祭圣坛"));
	}

	if (DescriptionText)
	{
		if (CurrentOptions.IsValidIndex(SelectedOptionIndex))
		{
			DescriptionText->SetText(GetSacrificeRuneName(CurrentOptions[SelectedOptionIndex]));
		}
		else if (AltarData && AltarData->EventSacrificeRune)
		{
			DescriptionText->SetText(FText::FromName(AltarData->EventSacrificeRune->GetRuneName()));
		}
		else
		{
			DescriptionText->SetText(NSLOCTEXT("SacrificeSelection", "Desc", "选择一个代价，获得全局献祭符文。"));
		}
	}

	RefreshOptionButtons();
	RefreshDeckButtons();

	if (CostText)
	{
		if (CurrentOptions.IsValidIndex(SelectedOptionIndex))
		{
			const FAltarSacrificeEntry& Entry = CurrentOptions[SelectedOptionIndex];
			FText Cost = Entry.CostDescription.IsEmpty() ? GetCostFallbackText(Entry.CostType) : Entry.CostDescription;
			if (Entry.CostType == ESacrificeOfferingCostType::SacrificeDeckCard && SelectedDeckCardIndex != INDEX_NONE)
			{
				const TArray<FCombatCardInstance> DeckCards = GetDeckCards();
				if (DeckCards.IsValidIndex(SelectedDeckCardIndex))
				{
					Cost = FText::Format(
						NSLOCTEXT("SacrificeSelection", "CostDeckSelected", "献祭卡牌：{0}"),
						GetCardDisplayName(DeckCards[SelectedDeckCardIndex]));
				}
			}
			CostText->SetText(Cost);
			CostText->SetVisibility(Phase == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
		else
		{
			CostText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (BtnConfirm)
	{
		BtnConfirm->SetVisibility(Phase == 1 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	FocusButton(FocusedButtonIndex);
}

void USacrificeSelectionWidget::RefreshOptionButtons()
{
	const TArray<UButton*> Buttons = GetOptionButtons();
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		UButton* Button = Buttons[Index];
		if (!Button)
		{
			continue;
		}

		const bool bValid = CurrentOptions.IsValidIndex(Index);
		Button->SetVisibility(Phase == 0 && bValid ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bValid)
		{
			const FAltarSacrificeEntry& Entry = CurrentOptions[Index];
			const FText Cost = Entry.CostDescription.IsEmpty() ? GetCostFallbackText(Entry.CostType) : Entry.CostDescription;
			SetButtonLabel(Button, FText::Format(
				NSLOCTEXT("SacrificeSelection", "OptionFormat", "{0}\n{1}"),
				GetSacrificeRuneName(Entry),
				Cost));
		}
	}
}

void USacrificeSelectionWidget::RefreshDeckButtons()
{
	const TArray<UButton*> Buttons = GetDeckButtons();
	const TArray<FCombatCardInstance> DeckCards = GetDeckCards();
	for (int32 Index = 0; Index < Buttons.Num(); ++Index)
	{
		UButton* Button = Buttons[Index];
		if (!Button)
		{
			continue;
		}

		const bool bValid = DeckCards.IsValidIndex(Index);
		Button->SetVisibility(Phase == 2 && bValid ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bValid)
		{
			SetButtonLabel(Button, GetCardDisplayName(DeckCards[Index]));
		}
	}
}

void USacrificeSelectionWidget::SetButtonLabel(UButton* Button, const FText& Text) const
{
	if (!Button)
	{
		return;
	}

	if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
	{
		Label->SetText(Text);
	}
}

TArray<UButton*> USacrificeSelectionWidget::GetOptionButtons() const
{
	return { OptionButton0, OptionButton1, OptionButton2 };
}

TArray<UButton*> USacrificeSelectionWidget::GetDeckButtons() const
{
	return { DeckButton0, DeckButton1, DeckButton2, DeckButton3, DeckButton4, DeckButton5, DeckButton6, DeckButton7 };
}

TArray<UButton*> USacrificeSelectionWidget::GetFocusableButtons() const
{
	TArray<UButton*> Buttons;
	if (Phase == 0)
	{
		for (UButton* Button : GetOptionButtons())
		{
			if (Button && Button->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Buttons.Add(Button);
			}
		}
	}
	else if (Phase == 2)
	{
		for (UButton* Button : GetDeckButtons())
		{
			if (Button && Button->GetVisibility() != ESlateVisibility::Collapsed)
			{
				Buttons.Add(Button);
			}
		}
	}
	else if (Phase == 1 && BtnConfirm && BtnConfirm->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Buttons.Add(BtnConfirm);
	}

	if (BtnCancel && BtnCancel->GetVisibility() != ESlateVisibility::Collapsed)
	{
		Buttons.Add(BtnCancel);
	}
	return Buttons;
}

TArray<FCombatCardInstance> USacrificeSelectionWidget::GetDeckCards() const
{
	APlayerCharacterBase* Player = OwningPlayer.Get();
	return (Player && Player->CombatDeckComponent)
		? Player->CombatDeckComponent->GetFullDeckSnapshot()
		: TArray<FCombatCardInstance>();
}

bool USacrificeSelectionWidget::PaySelectedCost()
{
	APlayerCharacterBase* Player = OwningPlayer.Get();
	if (!Player || !CurrentOptions.IsValidIndex(SelectedOptionIndex))
	{
		return false;
	}

	return Player->ApplySacrificeOfferingCost(CurrentOptions[SelectedOptionIndex], SelectedDeckCardIndex);
}

bool USacrificeSelectionWidget::GrantSelectedRune()
{
	APlayerCharacterBase* Player = OwningPlayer.Get();
	if (!Player || !CurrentOptions.IsValidIndex(SelectedOptionIndex))
	{
		return false;
	}

	URuneDataAsset* Rune = CurrentOptions[SelectedOptionIndex].GrantedRune;
	if (!Rune && AltarData)
	{
		Rune = AltarData->EventSacrificeRune;
	}
	if (!Rune)
	{
		return false;
	}

	Player->AddRuneToInventory(Rune->CreateInstance());
	return true;
}

void USacrificeSelectionWidget::FailSacrifice(const FText& Reason)
{
	OnSacrificeFailed(Reason);
	if (CostText)
	{
		CostText->SetText(Reason);
		CostText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void USacrificeSelectionWidget::FocusButton(int32 NewIndex)
{
	TArray<UButton*> Buttons = GetFocusableButtons();
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

void USacrificeSelectionWidget::MoveFocus(int32 Direction)
{
	FocusButton(FocusedButtonIndex + Direction);
}

void USacrificeSelectionWidget::ActivateFocusedButton()
{
	TArray<UButton*> Buttons = GetFocusableButtons();
	if (Buttons.IsValidIndex(FocusedButtonIndex) && Buttons[FocusedButtonIndex])
	{
		Buttons[FocusedButtonIndex]->OnClicked.Broadcast();
	}
}

void USacrificeSelectionWidget::OnOption0Clicked() { SelectSacrificeOption(0); }
void USacrificeSelectionWidget::OnOption1Clicked() { SelectSacrificeOption(1); }
void USacrificeSelectionWidget::OnOption2Clicked() { SelectSacrificeOption(2); }
void USacrificeSelectionWidget::OnDeck0Clicked() { SelectDeckCardForSacrifice(0); }
void USacrificeSelectionWidget::OnDeck1Clicked() { SelectDeckCardForSacrifice(1); }
void USacrificeSelectionWidget::OnDeck2Clicked() { SelectDeckCardForSacrifice(2); }
void USacrificeSelectionWidget::OnDeck3Clicked() { SelectDeckCardForSacrifice(3); }
void USacrificeSelectionWidget::OnDeck4Clicked() { SelectDeckCardForSacrifice(4); }
void USacrificeSelectionWidget::OnDeck5Clicked() { SelectDeckCardForSacrifice(5); }
void USacrificeSelectionWidget::OnDeck6Clicked() { SelectDeckCardForSacrifice(6); }
void USacrificeSelectionWidget::OnDeck7Clicked() { SelectDeckCardForSacrifice(7); }
void USacrificeSelectionWidget::OnConfirmClicked() { ConfirmSacrifice(); }
void USacrificeSelectionWidget::OnCancelClicked() { CancelSacrifice(); }
