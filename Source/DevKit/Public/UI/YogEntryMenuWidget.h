#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "YogEntryMenuWidget.generated.h"

class UButton;
class UBorder;
class UTexture2D;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FYogEntryMenuAction);

UCLASS(Blueprintable)
class DEVKIT_API UYogEntryMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Entry Menu")
	FYogEntryMenuAction OnStartRequested;

	UPROPERTY(BlueprintAssignable, Category = "Entry Menu")
	FYogEntryMenuAction OnContinueRequested;

	UPROPERTY(BlueprintAssignable, Category = "Entry Menu")
	FYogEntryMenuAction OnOptionsRequested;

	UPROPERTY(BlueprintAssignable, Category = "Entry Menu")
	FYogEntryMenuAction OnQuitRequested;

	UFUNCTION(BlueprintCallable, Category = "Entry Menu")
	void SetBackgroundTexture(UTexture2D* InTexture);

	UFUNCTION(BlueprintCallable, Category = "Entry Menu")
	void FocusQuit();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnStart;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnContinue;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnOptions;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnQuit;

private:
	void BuildFallbackLayout();
	void CacheButtons();
	void MoveFocus(int32 Direction);
	void FocusButton(int32 NewIndex);
	void ActivateFocusedButton();
	void RefreshButtonVisuals();

	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleContinueClicked();

	UFUNCTION()
	void HandleOptionsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	TArray<TObjectPtr<UButton>> MenuButtons;

	UPROPERTY()
	TObjectPtr<UTexture2D> BackgroundTexture;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UBorder> BackgroundImage;

	int32 FocusedButtonIndex = 0;
	float LastAnalogNavigationTime = 0.f;
};
