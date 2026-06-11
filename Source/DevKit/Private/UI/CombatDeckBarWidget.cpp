#include "UI/CombatDeckBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "UI/CombatDeckCardSlotWidget.h"
#include "UI/YogCommonRichTextBlock.h"

void UCombatDeckBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheDesignerWidgets();
	UpdateShuffleVisuals(0.0f, false);
	if (ConsumedToastText)
	{
		ConsumedToastText->SetRenderOpacity(0.0f);
		ConsumedToastText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (RewardToastText)
	{
		RewardToastText->SetRenderOpacity(0.0f);
		RewardToastText->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DeckEntryHighlightPanel)
	{
		DeckEntryHighlightPanel->SetRenderOpacity(0.0f);
		DeckEntryHighlightPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshDeckSnapshot();
}

void UCombatDeckBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickToast(ConsumedToastText, ConsumedToastTimeRemaining, InDeltaTime);
	TickToast(RewardToastText, RewardToastTimeRemaining, InDeltaTime);
	TickDeckCardsEnteredHighlight(InDeltaTime);
}

void UCombatDeckBarWidget::BindToCombatDeck(UCombatDeckComponent* InCombatDeck)
{
	if (BoundCombatDeck == InCombatDeck)
	{
		RefreshDeckSnapshot();
		return;
	}

	UnbindFromCurrentDeck();
	BoundCombatDeck = InCombatDeck;

	if (BoundCombatDeck)
	{
		BoundCombatDeck->OnDeckLoaded.AddDynamic(this, &UCombatDeckBarWidget::HandleDeckLoaded);
		BoundCombatDeck->OnCardConsumed.AddDynamic(this, &UCombatDeckBarWidget::HandleCardConsumed);
		BoundCombatDeck->OnShuffleStarted.AddDynamic(this, &UCombatDeckBarWidget::HandleShuffleStarted);
		BoundCombatDeck->OnShuffleProgress.AddDynamic(this, &UCombatDeckBarWidget::HandleShuffleProgress);
		BoundCombatDeck->OnShuffleCompleted.AddDynamic(this, &UCombatDeckBarWidget::HandleShuffleCompleted);
		BoundCombatDeck->OnRewardAddedToDeck.AddDynamic(this, &UCombatDeckBarWidget::HandleRewardAddedToDeck);
		BoundCombatDeck->OnDeckCardsEntered.AddDynamic(this, &UCombatDeckBarWidget::HandleDeckCardsEntered);
	}

	RefreshDeckSnapshot();
}

void UCombatDeckBarWidget::RefreshDeckSnapshot()
{
	if (BoundCombatDeck)
	{
		const TArray<FCombatCardInstance> VisibleCards = BoundCombatDeck->GetRemainingDeckSnapshot();
		UpdateDeckVisuals(VisibleCards, BoundCombatDeck->GetDeckState(), BoundCombatDeck->GetCurrentIndex());
		BP_OnDeckSnapshotChanged(VisibleCards, BoundCombatDeck->GetDeckState());
		return;
	}

	TArray<FCombatCardInstance> EmptySnapshot;
	UpdateDeckVisuals(EmptySnapshot, EDeckState::Ready, INDEX_NONE);
	BP_OnDeckSnapshotChanged(EmptySnapshot, EDeckState::Ready);
}

void UCombatDeckBarWidget::PlayDeckCardsEnteredHighlight()
{
	if (IsDesignTime())
	{
		return;
	}

	EntryHighlightElapsed = 0.0f;
	bEntryHighlightAnimating = true;

	if (DeckEntryHighlightPanel)
	{
		DeckEntryHighlightPanel->SetVisibility(ESlateVisibility::HitTestInvisible);
		DeckEntryHighlightPanel->SetRenderOpacity(0.0f);
	}

	SetRenderScale(FVector2D(1.0f, 1.0f));
}

void UCombatDeckBarWidget::NativeDestruct()
{
	UnbindFromCurrentDeck();
	Super::NativeDestruct();
}

void UCombatDeckBarWidget::UnbindFromCurrentDeck()
{
	if (!BoundCombatDeck)
	{
		return;
	}

	BoundCombatDeck->OnDeckLoaded.RemoveDynamic(this, &UCombatDeckBarWidget::HandleDeckLoaded);
	BoundCombatDeck->OnCardConsumed.RemoveDynamic(this, &UCombatDeckBarWidget::HandleCardConsumed);
	BoundCombatDeck->OnShuffleStarted.RemoveDynamic(this, &UCombatDeckBarWidget::HandleShuffleStarted);
	BoundCombatDeck->OnShuffleProgress.RemoveDynamic(this, &UCombatDeckBarWidget::HandleShuffleProgress);
	BoundCombatDeck->OnShuffleCompleted.RemoveDynamic(this, &UCombatDeckBarWidget::HandleShuffleCompleted);
	BoundCombatDeck->OnRewardAddedToDeck.RemoveDynamic(this, &UCombatDeckBarWidget::HandleRewardAddedToDeck);
	BoundCombatDeck->OnDeckCardsEntered.RemoveDynamic(this, &UCombatDeckBarWidget::HandleDeckCardsEntered);
	BoundCombatDeck = nullptr;
}

void UCombatDeckBarWidget::CacheDesignerWidgets()
{
	CachedCardSlots.Reset();
	CachedCardSlots.Reserve(8);

	UCombatDeckCardSlotWidget* SlotWidgets[] =
	{
		CardSlot_0,
		CardSlot_1,
		CardSlot_2,
		CardSlot_3,
		CardSlot_4,
		CardSlot_5,
		CardSlot_6,
		CardSlot_7,
	};

	for (UCombatDeckCardSlotWidget* SlotWidget : SlotWidgets)
	{
		if (SlotWidget)
		{
			CachedCardSlots.Add(SlotWidget);
		}
	}
}

void UCombatDeckBarWidget::UpdateDeckVisuals(const TArray<FCombatCardInstance>& RemainingCards, EDeckState DeckState, int32 CurrentIndex)
{
	CacheDesignerWidgets();

	const bool bShouldShowDeckBar = !RemainingCards.IsEmpty();
	SetVisibility(bShouldShowDeckBar ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	for (int32 SlotIndex = 0; SlotIndex < CachedCardSlots.Num(); ++SlotIndex)
	{
		UCombatDeckCardSlotWidget* SlotWidget = CachedCardSlots[SlotIndex];
		if (!SlotWidget)
		{
			continue;
		}

		if (RemainingCards.IsValidIndex(SlotIndex))
		{
			SlotWidget->SetCard(RemainingCards[SlotIndex], SlotIndex == CurrentIndex);
		}
		else
		{
			SlotWidget->ClearSlot();
		}
	}

	UpdateStatusText(RemainingCards.Num(), DeckState, CurrentIndex);
}

void UCombatDeckBarWidget::UpdateStatusText(int32 RemainingCount, EDeckState DeckState, int32 CurrentIndex)
{
	if (RemainingCount <= 0)
	{
		SetTextIfBound(StatusText, FText::GetEmpty());
		return;
	}

	const int32 DisplayIndex = FMath::Clamp(CurrentIndex + 1, 1, RemainingCount);
	SetTextIfBound(
		StatusText,
		FText::Format(
			FText::FromString(TEXT("\u987a\u5e8f: {0}/{1}")),
			FText::AsNumber(DisplayIndex),
			FText::AsNumber(RemainingCount)));
	return;

	if (DeckState == EDeckState::EmptyShuffling)
	{
		SetTextIfBound(StatusText, FText::FromString(TEXT("\u56de\u586b\u4e2d")));
		return;
	}

	SetTextIfBound(
		StatusText,
		FText::Format(FText::FromString(TEXT("\u987a\u5e8f: {0}")), FText::AsNumber(RemainingCount)));
	return;

	if (DeckState == EDeckState::EmptyShuffling)
	{
		SetTextIfBound(StatusText, FText::FromString(TEXT("卡组装填中")));
		return;
	}

	SetTextIfBound(
		StatusText,
		FText::Format(FText::FromString(TEXT("卡组: {0}")), FText::AsNumber(RemainingCount)));
}

void UCombatDeckBarWidget::UpdateShuffleVisuals(float NormalizedProgress, bool bIsShuffling)
{
	if (ShufflePanel)
	{
		ShufflePanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ShuffleProgressBar)
	{
		ShuffleProgressBar->SetPercent(0.0f);
	}

	SetTextIfBound(ShuffleText, FText::GetEmpty());
	return;

	if (ShufflePanel)
	{
		ShufflePanel->SetVisibility(bIsShuffling ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ShuffleProgressBar)
	{
		ShuffleProgressBar->SetPercent(FMath::Clamp(NormalizedProgress, 0.0f, 1.0f));
	}

	const FText CompactShuffleLabel = bIsShuffling
		? FText::Format(
			FText::FromString(TEXT("\u5361\u7ec4\u56de\u586b\u4e2d {0}")),
			FText::AsPercent(NormalizedProgress))
		: FText::GetEmpty();
	SetTextIfBound(ShuffleText, CompactShuffleLabel);
	return;

	const FText ShuffleLabel = bIsShuffling
		? FText::Format(
			FText::FromString(TEXT("卡组装填中：可继续攻击，但暂时不会触发卡牌效果。 {0}")),
			FText::AsPercent(NormalizedProgress))
		: FText::GetEmpty();
	SetTextIfBound(ShuffleText, ShuffleLabel);
}

void UCombatDeckBarWidget::SetTextIfBound(UWidget* TextWidget, const FText& Text)
{
	if (UTextBlock* TextBlock = Cast<UTextBlock>(TextWidget))
	{
		TextBlock->SetText(Text);
		return;
	}

	if (UYogCommonRichTextBlock* RichTextBlock = Cast<UYogCommonRichTextBlock>(TextWidget))
	{
		RichTextBlock->SetText(Text);
	}
}

void UCombatDeckBarWidget::ShowToast(UWidget* ToastWidget, float& ToastTimeRemaining)
{
	if (!ToastWidget)
	{
		return;
	}

	ToastTimeRemaining = FMath::Max(0.0f, ToastVisibleDuration) + FMath::Max(0.01f, ToastFadeDuration);
	ToastWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	ToastWidget->SetRenderOpacity(1.0f);
}

void UCombatDeckBarWidget::TickToast(UWidget* ToastWidget, float& ToastTimeRemaining, float DeltaTime)
{
	if (!ToastWidget || ToastTimeRemaining <= 0.0f)
	{
		return;
	}

	ToastTimeRemaining = FMath::Max(0.0f, ToastTimeRemaining - FMath::Max(0.0f, DeltaTime));
	const float FadeDuration = FMath::Max(0.01f, ToastFadeDuration);
	const float NewOpacity = ToastTimeRemaining <= FadeDuration
		? FMath::Clamp(ToastTimeRemaining / FadeDuration, 0.0f, 1.0f)
		: 1.0f;

	ToastWidget->SetRenderOpacity(NewOpacity);
	if (ToastTimeRemaining <= 0.0f)
	{
		ToastWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

float UCombatDeckBarWidget::GetDeckEntryHighlightDuration() const
{
	return FMath::Max(0.0f, EntryHighlightFadeInDuration)
		+ FMath::Max(0.0f, EntryHighlightHoldDuration)
		+ FMath::Max(0.0f, EntryHighlightFadeOutDuration);
}

void UCombatDeckBarWidget::TickDeckCardsEnteredHighlight(float DeltaTime)
{
	if (!bEntryHighlightAnimating)
	{
		return;
	}

	const float FadeIn = FMath::Max(0.0f, EntryHighlightFadeInDuration);
	const float Hold = FMath::Max(0.0f, EntryHighlightHoldDuration);
	const float FadeOut = FMath::Max(0.0f, EntryHighlightFadeOutDuration);
	const float Duration = FMath::Max(0.01f, GetDeckEntryHighlightDuration());

	EntryHighlightElapsed = FMath::Min(EntryHighlightElapsed + FMath::Max(0.0f, DeltaTime), Duration);

	float HighlightAlpha = 1.0f;
	if (FadeIn > 0.0f && EntryHighlightElapsed < FadeIn)
	{
		HighlightAlpha = FMath::Clamp(EntryHighlightElapsed / FadeIn, 0.0f, 1.0f);
	}
	else if (FadeOut > 0.0f && EntryHighlightElapsed > FadeIn + Hold)
	{
		const float FadeOutElapsed = EntryHighlightElapsed - FadeIn - Hold;
		HighlightAlpha = FMath::Clamp(1.0f - (FadeOutElapsed / FadeOut), 0.0f, 1.0f);
	}

	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, HighlightAlpha, 2.0f);
	const float PeakScale = FMath::Max(1.0f, EntryHighlightPeakScale);
	const float CurrentScale = FMath::Lerp(1.0f, PeakScale, SmoothAlpha);
	SetRenderScale(FVector2D(CurrentScale, CurrentScale));

	if (DeckEntryHighlightPanel)
	{
		DeckEntryHighlightPanel->SetRenderOpacity(SmoothAlpha * FMath::Clamp(EntryHighlightPeakOpacity, 0.0f, 1.0f));
	}

	if (EntryHighlightElapsed >= Duration)
	{
		bEntryHighlightAnimating = false;
		EntryHighlightElapsed = 0.0f;
		SetRenderScale(FVector2D(1.0f, 1.0f));
		if (DeckEntryHighlightPanel)
		{
			DeckEntryHighlightPanel->SetRenderOpacity(0.0f);
			DeckEntryHighlightPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

FText UCombatDeckBarWidget::GetCardDisplayName(const FCombatCardInstance& Card)
{
	if (!Card.Config.DisplayName.IsEmpty())
	{
		return Card.Config.DisplayName;
	}

	if (Card.SourceData)
	{
		return FText::FromString(Card.SourceData->GetName());
	}

	return FText::FromString(TEXT("Card"));
}

FText UCombatDeckBarWidget::GetConsumedToastText(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result)
{
	if (Result.bCardTemporarilyLocked)
	{
		return FText::Format(
			FText::FromString(TEXT("Finisher locked {0}/{1}")),
			FText::AsNumber(Result.TemporaryUnlockCurrentCompletedBattles),
			FText::AsNumber(Result.TemporaryUnlockRequiredCompletedBattles));
	}

	return FText::Format(FText::FromString(TEXT("\u89e6\u53d1 {0}")), GetCardDisplayName(Card));
}

int32 UCombatDeckBarWidget::FindVisibleCardIndex(const FCombatCardInstance& Card) const
{
	if (!BoundCombatDeck)
	{
		return INDEX_NONE;
	}

	const TArray<FCombatCardInstance> VisibleCards = BoundCombatDeck->GetRemainingDeckSnapshot();
	if (Card.InstanceGuid.IsValid())
	{
		for (int32 Index = 0; Index < VisibleCards.Num(); ++Index)
		{
			if (VisibleCards[Index].InstanceGuid == Card.InstanceGuid)
			{
				return Index;
			}
		}
	}

	const int32 FallbackUsedIndex = BoundCombatDeck->GetCurrentIndex() - 1;
	return VisibleCards.IsValidIndex(FallbackUsedIndex) ? FallbackUsedIndex : INDEX_NONE;
}

void UCombatDeckBarWidget::HandleDeckLoaded(const TArray<FCombatCardInstance>& ActiveSequence)
{
	UpdateShuffleVisuals(0.0f, false);
	UpdateDeckVisuals(ActiveSequence, EDeckState::Ready, BoundCombatDeck ? BoundCombatDeck->GetCurrentIndex() : 0);
	BP_OnDeckSnapshotChanged(ActiveSequence, EDeckState::Ready);
}

void UCombatDeckBarWidget::HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result)
{
	const int32 UsedCardIndex = FindVisibleCardIndex(Card);

	if (Result.bCardTemporarilyLocked)
	{
		SetTextIfBound(ConsumedToastText, GetConsumedToastText(Card, Result));
		ShowToast(ConsumedToastText, ConsumedToastTimeRemaining);
		RefreshDeckSnapshot();
		if (CachedCardSlots.IsValidIndex(UsedCardIndex) && CachedCardSlots[UsedCardIndex])
		{
			CachedCardSlots[UsedCardIndex]->PlayUseFlipAnimation();
		}
		BP_OnCardConsumed(Card, Result);
		return;
	}

	SetTextIfBound(
		ConsumedToastText,
		FText::Format(FText::FromString(TEXT("\u89e6\u53d1 {0}")), GetCardDisplayName(Card)));
	ShowToast(ConsumedToastText, ConsumedToastTimeRemaining);
	RefreshDeckSnapshot();
	if (CachedCardSlots.IsValidIndex(UsedCardIndex) && CachedCardSlots[UsedCardIndex])
	{
		CachedCardSlots[UsedCardIndex]->PlayUseFlipAnimation();
	}
	BP_OnCardConsumed(Card, Result);
	return;

	SetTextIfBound(
		ConsumedToastText,
		FText::Format(FText::FromString(TEXT("消耗: {0}")), GetCardDisplayName(Card)));
	ShowToast(ConsumedToastText, ConsumedToastTimeRemaining);
	RefreshDeckSnapshot();
	BP_OnCardConsumed(Card, Result);
}

void UCombatDeckBarWidget::HandleShuffleStarted(const FCombatCardResolveResult& Result)
{
	TArray<FCombatCardInstance> EmptySnapshot;
	UpdateShuffleVisuals(0.0f, true);
	UpdateDeckVisuals(EmptySnapshot, EDeckState::EmptyShuffling, INDEX_NONE);
	BP_OnDeckSnapshotChanged(EmptySnapshot, EDeckState::EmptyShuffling);
}

void UCombatDeckBarWidget::HandleShuffleProgress(float NormalizedProgress)
{
	UpdateShuffleVisuals(NormalizedProgress, true);
	BP_OnShuffleProgress(NormalizedProgress);
}

void UCombatDeckBarWidget::HandleShuffleCompleted(const TArray<FCombatCardInstance>& ActiveSequence)
{
	UpdateShuffleVisuals(1.0f, false);
	UpdateDeckVisuals(ActiveSequence, EDeckState::Ready, BoundCombatDeck ? BoundCombatDeck->GetCurrentIndex() : 0);
	BP_OnDeckSnapshotChanged(ActiveSequence, EDeckState::Ready);
}

void UCombatDeckBarWidget::HandleRewardAddedToDeck(const FCombatCardInstance& Card)
{
	SetTextIfBound(
		RewardToastText,
		FText::Format(FText::FromString(TEXT("\u5165\u7ec4 {0}")), GetCardDisplayName(Card)));
	ShowToast(RewardToastText, RewardToastTimeRemaining);
	BP_OnRewardAddedToDeck(Card);
	RefreshDeckSnapshot();
	return;

	SetTextIfBound(
		RewardToastText,
		FText::Format(FText::FromString(TEXT("已入组: {0}")), GetCardDisplayName(Card)));
	ShowToast(RewardToastText, RewardToastTimeRemaining);
	BP_OnRewardAddedToDeck(Card);
	RefreshDeckSnapshot();
}

void UCombatDeckBarWidget::HandleDeckCardsEntered(const TArray<FCombatCardInstance>& Cards)
{
	if (!Cards.IsEmpty())
	{
		PlayDeckCardsEnteredHighlight();
	}
	BP_OnDeckCardsEntered(Cards);
}
