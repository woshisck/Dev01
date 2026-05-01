#include "UI/CombatDeckEditWidget.h"

#include "Character/PlayerCharacterBase.h"
#include "Components/VerticalBox.h"
#include "UI/CombatDeckEditCardSlotWidget.h"
#include "UI/RuneInfoCardWidget.h"

void UCombatDeckEditWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindToOwningPlayerCombatDeck();
}

void UCombatDeckEditWidget::NativeDestruct()
{
	UnbindFromCurrentDeck();
	Super::NativeDestruct();
}

void UCombatDeckEditWidget::BindToOwningPlayerCombatDeck()
{
	APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetOwningPlayerPawn());
	BindToCombatDeck(Player ? Player->CombatDeckComponent : nullptr);
}

void UCombatDeckEditWidget::BindToCombatDeck(UCombatDeckComponent* InCombatDeck)
{
	if (BoundCombatDeck == InCombatDeck)
	{
		RefreshDeckList();
		return;
	}

	UnbindFromCurrentDeck();
	BoundCombatDeck = InCombatDeck;

	if (BoundCombatDeck)
	{
		BoundCombatDeck->OnDeckLoaded.AddDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
		BoundCombatDeck->OnShuffleCompleted.AddDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
		BoundCombatDeck->OnCardConsumed.AddDynamic(this, &UCombatDeckEditWidget::HandleCardConsumed);
		BoundCombatDeck->OnRewardAddedToDeck.AddDynamic(this, &UCombatDeckEditWidget::HandleRewardAddedToDeck);
	}

	SelectedCardIndex = INDEX_NONE;
	RefreshDeckList();
}

void UCombatDeckEditWidget::UnbindFromCurrentDeck()
{
	if (!BoundCombatDeck)
	{
		return;
	}

	BoundCombatDeck->OnDeckLoaded.RemoveDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
	BoundCombatDeck->OnShuffleCompleted.RemoveDynamic(this, &UCombatDeckEditWidget::HandleDeckChanged);
	BoundCombatDeck->OnCardConsumed.RemoveDynamic(this, &UCombatDeckEditWidget::HandleCardConsumed);
	BoundCombatDeck->OnRewardAddedToDeck.RemoveDynamic(this, &UCombatDeckEditWidget::HandleRewardAddedToDeck);
	BoundCombatDeck = nullptr;
}

void UCombatDeckEditWidget::RefreshDeckList()
{
	if (!CardListBox)
	{
		RefreshSelectedCardInfo();
		return;
	}

	CardListBox->ClearChildren();
	if (!BoundCombatDeck || !CardSlotClass)
	{
		RefreshSelectedCardInfo();
		return;
	}

	const TArray<FCombatCardInstance> Cards = BoundCombatDeck->GetFullDeckSnapshot();
	if (!Cards.IsValidIndex(SelectedCardIndex))
	{
		SelectedCardIndex = Cards.IsEmpty() ? INDEX_NONE : 0;
	}

	for (int32 Index = 0; Index < Cards.Num(); ++Index)
	{
		UCombatDeckEditCardSlotWidget* CardSlotWidget = CreateWidget<UCombatDeckEditCardSlotWidget>(GetOwningPlayer(), CardSlotClass);
		if (!CardSlotWidget)
		{
			continue;
		}

		CardSlotWidget->SetCard(this, Cards[Index], Index, Index == SelectedCardIndex);
		CardListBox->AddChildToVerticalBox(CardSlotWidget);
	}

	RefreshSelectedCardInfo();
}

void UCombatDeckEditWidget::SelectCard(int32 CardIndex)
{
	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	if (!Cards.IsValidIndex(CardIndex))
	{
		SelectedCardIndex = INDEX_NONE;
	}
	else
	{
		SelectedCardIndex = CardIndex;
	}

	RefreshDeckList();
}

bool UCombatDeckEditWidget::MoveCard(int32 FromIndex, int32 InsertIndex)
{
	if (!BoundCombatDeck || bInteractionLocked)
	{
		return false;
	}

	const bool bMoved = BoundCombatDeck->MoveCardInDeck(FromIndex, InsertIndex);
	if (bMoved)
	{
		SelectedCardIndex = FromIndex < InsertIndex ? InsertIndex - 1 : InsertIndex;
		RefreshDeckList();
	}
	return bMoved;
}

bool UCombatDeckEditWidget::ToggleLinkOrientation(int32 CardIndex)
{
	if (!BoundCombatDeck || bInteractionLocked)
	{
		return false;
	}

	const bool bChanged = BoundCombatDeck->ToggleCardLinkOrientationByIndex(CardIndex);
	if (bChanged)
	{
		SelectedCardIndex = CardIndex;
		RefreshDeckList();
	}
	return bChanged;
}

bool UCombatDeckEditWidget::ToggleSelectedLinkOrientation()
{
	return ToggleLinkOrientation(SelectedCardIndex);
}

void UCombatDeckEditWidget::SetInteractionLocked(bool bLocked)
{
	bInteractionLocked = bLocked;
}

void UCombatDeckEditWidget::RefreshSelectedCardInfo()
{
	const TArray<FCombatCardInstance> Cards = BoundCombatDeck ? BoundCombatDeck->GetFullDeckSnapshot() : TArray<FCombatCardInstance>();
	const FCombatCardInstance SelectedCard = Cards.IsValidIndex(SelectedCardIndex) ? Cards[SelectedCardIndex] : FCombatCardInstance();

	if (RuneInfoCard)
	{
		if (SelectedCard.SourceData)
		{
			RuneInfoCard->ShowRune(SelectedCard.SourceData->RuneInfo);
		}
		else
		{
			RuneInfoCard->HideCard();
		}
	}

	BP_OnSelectedCardChanged(SelectedCard, SelectedCardIndex);
}

void UCombatDeckEditWidget::HandleDeckChanged(const TArray<FCombatCardInstance>& ActiveSequence)
{
	RefreshDeckList();
}

void UCombatDeckEditWidget::HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result)
{
	RefreshDeckList();
}

void UCombatDeckEditWidget::HandleRewardAddedToDeck(const FCombatCardInstance& Card)
{
	RefreshDeckList();
}
