#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "SacrificeGraceOptionWidget.generated.h"

class UTextBlock;
class UButton;
class USacrificeGraceDA;
class APlayerCharacterBase;
class ASacrificeGracePickup;

/**
 * Retired compatibility widget.
 *
 * SacrificeGraceOption is no longer used in runtime UI. The class remains so old
 * assets can load, but accepting/canceling only closes the widget.
 */
UCLASS(Abstract, Blueprintable)
class DEVKIT_API USacrificeGraceOptionWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	/** Compatibility setup for old assets; player and pickup are intentionally ignored. */
	void Setup(USacrificeGraceDA* InDA, APlayerCharacterBase* InPlayer, ASacrificeGracePickup* InPickup);

	UFUNCTION(BlueprintCallable, Category = "SacrificeGrace")
	void CancelChoice();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** Legacy BP hook kept for asset compatibility. */
	UFUNCTION(BlueprintImplementableEvent, Category = "SacrificeGrace")
	void OnSetup(USacrificeGraceDA* DA);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnYes;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BtnNo;

private:
	UPROPERTY()
	TObjectPtr<USacrificeGraceDA> SacrificeDA;

	UFUNCTION()
	void OnYesClicked();

	UFUNCTION()
	void OnNoClicked();

	void FocusButton(int32 NewIndex);
	void ActivateFocusedButton();

	int32 FocusedButtonIndex = 0;
	float LastAnalogNavigationTime = 0.f;
};
