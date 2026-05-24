#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/CombatDeckComponent.h"
#include "CombatDeckBarWidget.generated.h"

UCLASS()
class DEVKIT_API UCombatDeckBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void BindToCombatDeck(UCombatDeckComponent* InCombatDeck);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void RefreshDeckSnapshot();

	UFUNCTION(BlueprintPure, Category = "Combat Deck")
	UCombatDeckComponent* GetBoundCombatDeck() const { return BoundCombatDeck; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck")
	void BP_OnDeckSnapshotChanged(const TArray<FCombatCardInstance>& ActiveSequence, EDeckState DeckState);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck")
	void BP_OnCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck")
	void BP_OnShuffleProgress(float NormalizedProgress);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck")
	void BP_OnRewardAddedToDeck(const FCombatCardInstance& Card);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck")
	void BP_OnDeckCardsEntered(const TArray<FCombatCardInstance>& Cards);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Animation")
	void PlayDeckCardsEnteredHighlight();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Toast", meta = (ClampMin = "0.0"))
	float ToastVisibleDuration = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Toast", meta = (ClampMin = "0.01"))
	float ToastFadeDuration = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Entry Highlight", meta = (ClampMin = "0.0", ToolTip = "Seconds for the deck-entry highlight to fade in when cards enter the 1D deck."))
	float EntryHighlightFadeInDuration = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Entry Highlight", meta = (ClampMin = "0.0", ToolTip = "Seconds for the deck-entry highlight to stay fully visible."))
	float EntryHighlightHoldDuration = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Entry Highlight", meta = (ClampMin = "0.0", ToolTip = "Seconds for the deck-entry highlight to fade out."))
	float EntryHighlightFadeOutDuration = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Entry Highlight", meta = (ClampMin = "1.0", ClampMax = "1.2", ToolTip = "Small code-driven pulse scale applied to the whole deck bar while the entry highlight is visible."))
	float EntryHighlightPeakScale = 1.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Entry Highlight", meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Maximum render opacity applied to DeckEntryHighlightPanel. The panel brush alpha still controls the final visual strength."))
	float EntryHighlightPeakOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_0;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_1;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_2;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_3;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_4;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_5;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_6;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UCombatDeckCardSlotWidget> CardSlot_7;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> ShufflePanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UProgressBar> ShuffleProgressBar;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> ShuffleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> StatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> ConsumedToastText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> RewardToastText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UWidget> DeckEntryHighlightPanel;

private:
	UPROPERTY()
	TObjectPtr<UCombatDeckComponent> BoundCombatDeck;

	UPROPERTY()
	TArray<TObjectPtr<class UCombatDeckCardSlotWidget>> CachedCardSlots;

	void UnbindFromCurrentDeck();
	void CacheDesignerWidgets();
	void UpdateDeckVisuals(const TArray<FCombatCardInstance>& RemainingCards, EDeckState DeckState);
	void UpdateStatusText(int32 RemainingCount, EDeckState DeckState);
	void UpdateShuffleVisuals(float NormalizedProgress, bool bIsShuffling);
	void SetTextIfBound(class UWidget* TextWidget, const FText& Text);
	void ShowToast(class UWidget* ToastWidget, float& ToastTimeRemaining);
	void TickToast(class UWidget* ToastWidget, float& ToastTimeRemaining, float DeltaTime);
	void TickDeckCardsEnteredHighlight(float DeltaTime);
	float GetDeckEntryHighlightDuration() const;

	static FText GetCardDisplayName(const FCombatCardInstance& Card);
	static FText GetConsumedToastText(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result);

	float ConsumedToastTimeRemaining = 0.0f;
	float RewardToastTimeRemaining = 0.0f;
	float EntryHighlightElapsed = 0.0f;
	bool bEntryHighlightAnimating = false;

	UFUNCTION()
	void HandleDeckLoaded(const TArray<FCombatCardInstance>& ActiveSequence);

	UFUNCTION()
	void HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result);

	UFUNCTION()
	void HandleShuffleStarted(const FCombatCardResolveResult& Result);

	UFUNCTION()
	void HandleShuffleProgress(float NormalizedProgress);

	UFUNCTION()
	void HandleShuffleCompleted(const TArray<FCombatCardInstance>& ActiveSequence);

	UFUNCTION()
	void HandleRewardAddedToDeck(const FCombatCardInstance& Card);

	UFUNCTION()
	void HandleDeckCardsEntered(const TArray<FCombatCardInstance>& Cards);
};
