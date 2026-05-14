#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "YogGameOverWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class UWidget;

UCLASS(Blueprintable)
class DEVKIT_API UYogGameOverWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "GameOver")
	void Setup(bool bInCanRevive);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SubtitleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> ButtonBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnRevive;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnRetry;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnReturnToTitle;

private:
	void BuildFallbackLayout();
	void CacheButtons();
	void FocusButton(int32 NewIndex);
	void ActivateFocusedButton();
	void RefreshButtonVisuals();
	void CloseManagedScreen();

	UFUNCTION()
	void HandleReviveClicked();

	UFUNCTION()
	void HandleReviveHovered();

	UFUNCTION()
	void HandleRetryClicked();

	UFUNCTION()
	void HandleRetryHovered();

	UFUNCTION()
	void HandleReturnToTitleClicked();

	UFUNCTION()
	void HandleReturnToTitleHovered();

	TArray<TObjectPtr<UButton>> MenuButtons;
	bool bCanRevive = false;
	int32 FocusedButtonIndex = 0;
	float LastAnalogNavigationTime = 0.f;
};
