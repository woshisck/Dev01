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

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

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
	TObjectPtr<class UTextBlock> ShuffleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> StatusText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> ConsumedToastText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<class UTextBlock> RewardToastText;

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
	void SetTextIfBound(class UTextBlock* TextBlock, const FText& Text);

	static FText GetCardDisplayName(const FCombatCardInstance& Card);

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
};
