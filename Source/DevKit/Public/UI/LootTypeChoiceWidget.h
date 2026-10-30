#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "GameModes/LevelFlowTypes.h"
#include "LootTypeChoiceWidget.generated.h"

class APlayerCharacterBase;
class ARewardPickup;
class UButton;
class ULootTypeChoiceWidget;
class UPanelWidget;

/** Routes a dynamically created button's argument-less OnClicked back to its choice index. */
UCLASS()
class ULootTypeChoiceClickProxy : public UObject
{
	GENERATED_BODY()

public:
	void Init(ULootTypeChoiceWidget* InOwner, int32 InIndex);

	UFUNCTION()
	void HandleClicked();

private:
	TWeakObjectPtr<ULootTypeChoiceWidget> Owner;
	int32 ChoiceIndex = INDEX_NONE;
};

/**
 * "Choose your reward" popup shown by a reward pickup before any loot is granted.
 * One button per FLootTypeChoice on the pickup.
 *
 * WBP setup:
 *   1. Create a Widget Blueprint with parent LootTypeChoiceWidget.
 *   2. Optionally add a panel widget named ChoiceContainer (HorizontalBox / VerticalBox / WrapBox).
 *      Buttons are created into it at runtime. Without it the widget builds a plain fallback layout.
 *   3. Register it in the UI Registry DA under ScreenId LootTypeChoice.
 */
UCLASS(Blueprintable)
class DEVKIT_API ULootTypeChoiceWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	void Setup(ARewardPickup* InPickup, APlayerCharacterBase* InPlayer);

	/** Picks entry Index: closes the popup, then asks the pickup to grant it. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void ConfirmChoice(int32 Index);

	/** Closes the popup and resets the pickup so the player can interact with it again. */
	UFUNCTION(BlueprintCallable, Category = "Loot")
	void CancelChoice();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ChoiceContainer;

private:
	void BuildFallbackLayout();
	void RebuildChoiceButtons();
	void FocusButton(int32 NewIndex);
	void ActivateFocusedButton();
	void CloseScreen();

	TWeakObjectPtr<ARewardPickup> SourcePickup;
	TWeakObjectPtr<APlayerCharacterBase> OwningPlayer;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> ChoiceButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ULootTypeChoiceClickProxy>> ClickProxies;

	int32 FocusedButtonIndex = 0;
};
