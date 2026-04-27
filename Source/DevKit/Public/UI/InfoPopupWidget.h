#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InfoPopupWidget.generated.h"

class UCommonRichTextBlock;
class UImage;
class UBackgroundBlur;
class UButton;
class ULevelInfoPopupDA;

UCLASS()
class DEVKIT_API UInfoPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Show(const ULevelInfoPopupDA* DA);

	UFUNCTION(BlueprintCallable)
	void RequestClose();

	FSimpleMulticastDelegate OnClosed;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCommonRichTextBlock> BodyText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonRichTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBackgroundBlur> BlurPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> BtnClose;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	static constexpr float FadeDuration = 0.25f;

	float FadeDirection = 0.f;
	float FadeAlpha = 0.f;

	FTimerHandle AutoCloseTimer;

	UFUNCTION()
	void OnBtnCloseClicked();

	void DoClose();
};
