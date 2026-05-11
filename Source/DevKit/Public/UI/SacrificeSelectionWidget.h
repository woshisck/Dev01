#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "Component/CombatDeckComponent.h"
#include "Data/AltarDataAsset.h"
#include "SacrificeSelectionWidget.generated.h"

class APlayerCharacterBase;
class AAltarActor;
class UButton;
class UTextBlock;
class UVerticalBox;

UCLASS(Blueprintable)
class DEVKIT_API USacrificeSelectionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	static TSubclassOf<UTextBlock> GetMenuTextBlockClassForTests();

	void Setup(UAltarDataAsset* InData, APlayerCharacterBase* InPlayer, AAltarActor* InSourceAltar = nullptr);

	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void SelectSacrificeOption(int32 OptionIndex);

	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void SelectDeckCardForSacrifice(int32 CardIndex);

	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void ConfirmSacrifice();

	UFUNCTION(BlueprintCallable, Category = "Sacrifice")
	void CancelSacrifice();

	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnShowSacrificeOptions(const TArray<FAltarSacrificeEntry>& Options);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnShowCostConfirmation(const FAltarSacrificeEntry& SelectedEntry);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnShowDeckCardSelection(const TArray<FCombatCardInstance>& DeckCards);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnSacrificeFailed(const FText& Reason);

	UFUNCTION(BlueprintImplementableEvent, Category = "Sacrifice")
	void OnSacrificeFinished(bool bConfirmed);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> OptionBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> DeckCardBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> OptionButton2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton0;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton1;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton2;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton3;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton4;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton5;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton6;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeckButton7;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnConfirm;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnCancel;

	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;
	TWeakObjectPtr<AAltarActor> SourceAltar;
	TObjectPtr<UAltarDataAsset> AltarData;
	TArray<FAltarSacrificeEntry> CurrentOptions;
	int32 SelectedOptionIndex = INDEX_NONE;
	int32 SelectedDeckCardIndex = INDEX_NONE;
	int32 Phase = 0;

private:
	void BuildFallbackLayout();
	void RefreshNativeView();
	void RefreshOptionButtons();
	void RefreshDeckButtons();
	void SetButtonLabel(UButton* Button, const FText& Text) const;
	TArray<UButton*> GetOptionButtons() const;
	TArray<UButton*> GetDeckButtons() const;
	TArray<UButton*> GetFocusableButtons() const;
	TArray<FCombatCardInstance> GetDeckCards() const;
	bool PaySelectedCost();
	bool GrantSelectedRune();
	void FailSacrifice(const FText& Reason);
	void FocusButton(int32 NewIndex);
	void MoveFocus(int32 Direction);
	void ActivateFocusedButton();

	int32 FocusedButtonIndex = 0;
	float LastAnalogNavigationTime = 0.f;

	UFUNCTION()
	void OnOption0Clicked();
	UFUNCTION()
	void OnOption1Clicked();
	UFUNCTION()
	void OnOption2Clicked();
	UFUNCTION()
	void OnDeck0Clicked();
	UFUNCTION()
	void OnDeck1Clicked();
	UFUNCTION()
	void OnDeck2Clicked();
	UFUNCTION()
	void OnDeck3Clicked();
	UFUNCTION()
	void OnDeck4Clicked();
	UFUNCTION()
	void OnDeck5Clicked();
	UFUNCTION()
	void OnDeck6Clicked();
	UFUNCTION()
	void OnDeck7Clicked();
	UFUNCTION()
	void OnConfirmClicked();
	UFUNCTION()
	void OnCancelClicked();
};
