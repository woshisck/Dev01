#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/CombatDeckComponent.h"
#include "CombatDeckEditCardSlotWidget.generated.h"

class UButton;
class UImage;
class UWidget;
class UDragDropOperation;
class UCombatDeckEditWidget;

UCLASS()
class DEVKIT_API UCombatDeckEditCardSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void SetCard(UCombatDeckEditWidget* InOwnerWidget, const FCombatCardInstance& InCard, int32 InDeckIndex, bool bInSelected);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Edit")
	void ClearCard();

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnCardChanged(const FCombatCardInstance& InCard, int32 InDeckIndex, bool bInSelected);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnInteractionBlocked(const FCombatCardInstance& InCard, int32 InDeckIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnDragStarted(const FCombatCardInstance& InCard, int32 InDeckIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Combat Deck|Edit")
	void BP_OnDragHovered(int32 InsertIndex);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnAddedToFocusPath(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDragOver(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TypeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DirectionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SelectedMark;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SelectButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReverseButton;

private:
	UPROPERTY()
	TObjectPtr<UCombatDeckEditWidget> OwnerWidget;

	FCombatCardInstance Card;
	int32 DeckIndex = INDEX_NONE;
	bool bSelected = false;
	double MouseDownTimeSeconds = 0.0;
	bool bCapturedDefaultVisualState = false;
	FLinearColor DefaultColorAndOpacity = FLinearColor::White;
	float DefaultRenderOpacity = 1.0f;
	FWidgetTransform DefaultRenderTransform;
	FTimerHandle BlockedFeedbackTimerHandle;

	UFUNCTION()
	void HandleSelectClicked();

	UFUNCTION()
	void HandleReverseClicked();

	void SelectThisCard();
	bool IsInteractionLocked() const;
	void TriggerBlockedFeedback();
	void StartDragVisual();
	void ResetVisualState();
	void CaptureDefaultVisualState();
	void ApplySelectionVisual();
	FReply HandleCardMouseButtonDown(const FPointerEvent& InMouseEvent);
	FReply TryHandleReverseInput(const FKey& Key);
	bool IsPointerOverReverseButton(const FPointerEvent& InMouseEvent) const;
	int32 CalculateDropInsertIndex(const FGeometry& InGeometry, const FPointerEvent& ScreenEvent) const;

	static FText GetCardDisplayName(const FCombatCardInstance& InCard);
	static FText GetCardTypeText(ECombatCardType CardType);
	static FText GetDirectionText(const FCombatCardInstance& InCard);
	static void SetTextIfSupported(UWidget* Widget, const FText& Text);
};
