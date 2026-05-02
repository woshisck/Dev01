#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/CombatDeckComponent.h"
#include "CombatDeckEditWidget.generated.h"

class UVerticalBox;
class UBorder;
class URuneInfoCardWidget;
class UCombatDeckEditCardSlotWidget;
class UDragDropOperation;

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
	bool SelectAdjacentCard(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool HandleDeckDirectionalInput(int32 Direction);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool HandleDeckSelectPressed();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool HandleDeckSelectReleased();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void TickDeckGamepadInput(float DeltaTime);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool ToggleDetailPreview();

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void SetDetailPreviewVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "Combat Deck|Edit")
	bool IsDetailPreviewVisible() const { return bDetailPreviewVisible; }

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool MoveCard(int32 FromIndex, int32 InsertIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void BeginDragPreview(int32 SourceIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void UpdateDragPreview(int32 InsertIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	bool CommitDragPreview(int32 InsertIndex);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void EndDragPreview();

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

	UFUNCTION(BlueprintPure, Category = "Combat Deck|Edit")
	bool IsDragPreviewActive() const { return bDragPreviewActive; }

	UFUNCTION(BlueprintPure, Category = "Combat Deck|Edit")
	bool CanHandleDeckInput() const;

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnSelectedCardChanged(const FCombatCardInstance& SelectedCard, int32 CardIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnInteractionBlocked(const FCombatCardInstance& Card, int32 CardIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> CardListBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<URuneInfoCardWidget> RuneInfoCard;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit")
	TSubclassOf<UCombatDeckEditCardSlotWidget> CardSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit|Drag Preview")
	float DropIndicatorHeight = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit|Drag Preview")
	FLinearColor DropIndicatorColor = FLinearColor(1.0f, 0.74f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit|Gamepad")
	float GamepadDragHoldSeconds = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit|Gamepad")
	FVector2D GamepadFloatingDragOffset = FVector2D(18.0f, -24.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit|Gamepad")
	float GamepadFloatingDragOpacity = 0.92f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Edit|Gamepad")
	float GamepadFloatingDragScale = 1.06f;

private:
	UPROPERTY()
	TObjectPtr<UCombatDeckComponent> BoundCombatDeck;

	UPROPERTY()
	TObjectPtr<UCombatDeckEditCardSlotWidget> GamepadFloatingDragSlot;

	int32 SelectedCardIndex = INDEX_NONE;
	bool bInteractionLocked = false;
	bool bDragPreviewActive = false;
	int32 DragSourceIndex = INDEX_NONE;
	int32 DragPreviewInsertIndex = INDEX_NONE;
	bool bGamepadSelectHeld = false;
	bool bGamepadDragActive = false;
	float GamepadSelectHeldTime = 0.0f;
	int32 GamepadDragInsertIndex = INDEX_NONE;
	bool bDetailPreviewVisible = true;

	void UnbindFromCurrentDeck();
	void RefreshSelectedCardInfo();
	void ApplyDetailPreviewVisibility();
	void RefreshDeckListInternal(bool bUseDragPreview);
	void AddDropIndicatorToList();
	void AddInlineFloatingDragCardToList(const FCombatCardInstance& Card, int32 CardIndex);
	bool EnsureValidSelection();
	void BeginGamepadDrag();
	void ResetGamepadDragState();
	void ShowGamepadFloatingDragCard(const FCombatCardInstance& Card, int32 CardIndex);
	void UpdateGamepadFloatingDragCardPosition();
	void HideGamepadFloatingDragCard();
	int32 GetPreviewVisualInsertIndex(int32 InsertIndex) const;
	int32 GetDropInsertIndexFromListGeometry(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const;

	UFUNCTION()
	void HandleDeckChanged(const TArray<FCombatCardInstance>& ActiveSequence);

	UFUNCTION()
	void HandleCardConsumed(const FCombatCardInstance& Card, const FCombatCardResolveResult& Result);

	UFUNCTION()
	void HandleRewardAddedToDeck(const FCombatCardInstance& Card);
};
