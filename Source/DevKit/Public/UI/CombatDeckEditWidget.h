#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/CombatDeckComponent.h"
#include "CombatDeckEditWidget.generated.h"

class UVerticalBox;
class URuneInfoCardWidget;
class UCombatDeckEditCardSlotWidget;

UCLASS()
class DEVKIT_API UCombatDeckEditWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void BindToCombatDeck(UCombatDeckComponent* InCombatDeck);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void BindToOwningPlayerCombatDeck();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void RefreshDeckList();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void SelectCard(int32 CardIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool MoveCard(int32 FromIndex, int32 InsertIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool ToggleLinkOrientation(int32 CardIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool ToggleSelectedLinkOrientation();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void SetInteractionLocked(bool bLocked);

	UFUNCTION(BlueprintPure, Category = "Combat Deck|Edit")
	bool IsInteractionLocked() const { return bInteractionLocked; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck|Edit")
	int32 GetSelectedCardIndex() const { return SelectedCardIndex; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck|Edit")
	UCombatDeckComponent* GetBoundCombatDeck() const { return BoundCombatDeck; }

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnSelectedCardChanged(const FCombatCardInstance& SelectedCard, int32 CardIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnInteractionBlocked(const FCombatCardInstance& Card, int32 CardIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CardListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<URuneInfoCardWidget> RuneInfoCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit")
	TSubclassOf<UCombatDeckEditCardSlotWidget> CardSlotClass;

private:
	UPROPERTY()
	TObjectPtr<UCombatDeckComponent> BoundCombatDeck;

	int32 SelectedCardIndex = INDEX_NONE;
	bool bInteractionLocked = false;

	void UnbindFromCurrentDeck();
	void RefreshSelectedCardInfo();

	UFUNCTION()
	void HandleDeckChanged(const TArray<FCombatCardInstance>& ActiveSequence);

	UFUNCTION()
	void HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result);

	UFUNCTION()
	void HandleRewardAddedToDeck(const FCombatCardInstance& Card);
};
