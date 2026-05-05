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
	RefreshDeckSnapshot();
}

void UCombatDeckBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	TickToast(ConsumedToastText, ConsumedToastTimeRemaining, InDeltaTime);
	TickToast(RewardToastText, RewardToastTimeRemaining, InDeltaTime);
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
	}

	RefreshDeckSnapshot();
}

void UCombatDeckBarWidget::RefreshDeckSnapshot()
{
	if (BoundCombatDeck)
	{
		const TArray<FCombatCardInstance> RemainingCards = BoundCombatDeck->GetRemainingDeckSnapshot();
		UpdateDeckVisuals(RemainingCards, BoundCombatDeck->GetDeckState());
		BP_OnDeckSnapshotChanged(RemainingCards, BoundCombatDeck->GetDeckState());
		return;
	}

	TArray<FCombatCardInstance> EmptySnapshot;
	UpdateDeckVisuals(EmptySnapshot, EDeckState::Ready);
	BP_OnDeckSnapshotChanged(EmptySnapshot, EDeckState::Ready);
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

void UCombatDeckBarWidget::UpdateDeckVisuals(const TArray<FCombatCardInstance>& RemainingCards, EDeckState DeckState)
{
	CacheDesignerWidgets();

	for (int32 SlotIndex = 0; SlotIndex < CachedCardSlots.Num(); ++SlotIndex)
	{
		UCombatDeckCardSlotWidget* SlotWidget = CachedCardSlots[SlotIndex];
		if (!SlotWidget)
		{
			continue;
		}

		if (RemainingCards.IsValidIndex(SlotIndex))
		{
			SlotWidget->SetCard(RemainingCards[SlotIndex], SlotIndex == 0);
		}
		else
		{
			SlotWidget->ClearSlot();
		}
	}

	UpdateStatusText(RemainingCards.Num(), DeckState);
}

void UCombatDeckBarWidget::UpdateStatusText(int32 RemainingCount, EDeckState DeckState)
{
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
		ShufflePanel->SetVisibility(bIsShuffling ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	if (ShuffleProgressBar)
	{
		ShuffleProgressBar->SetPercent(FMath::Clamp(NormalizedProgress, 0.0f, 1.0f));
	}

	const FText ShuffleLabel = bIsShuffling
		? FText::Format(
			FText::FromString(TEXT("卡组装填中：这段空窗仍可攻击，但暂时不会触发卡牌效果。 {0}")),
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

void UCombatDeckBarWidget::HandleDeckLoaded(const TArray<FCombatCardInstance>& ActiveSequence)
{
	UpdateShuffleVisuals(0.0f, false);
	UpdateDeckVisuals(ActiveSequence, EDeckState::Ready);
	BP_OnDeckSnapshotChanged(ActiveSequence, EDeckState::Ready);
}

void UCombatDeckBarWidget::HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result)
{
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
	UpdateDeckVisuals(EmptySnapshot, EDeckState::EmptyShuffling);
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
	UpdateDeckVisuals(ActiveSequence, EDeckState::Ready);
	BP_OnDeckSnapshotChanged(ActiveSequence, EDeckState::Ready);
}

void UCombatDeckBarWidget::HandleRewardAddedToDeck(const FCombatCardInstance& Card)
{
	SetTextIfBound(
		RewardToastText,
		FText::Format(FText::FromString(TEXT("已入组: {0}")), GetCardDisplayName(Card)));
	ShowToast(RewardToastText, RewardToastTimeRemaining);
	BP_OnRewardAddedToDeck(Card);
	RefreshDeckSnapshot();
}
