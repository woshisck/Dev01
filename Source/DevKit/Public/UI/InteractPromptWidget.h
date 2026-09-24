#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputTypeEnum.h"
#include "InteractPromptWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * Hold-to-interact prompt: the interact button glyph with the radial hold material behind it.
 *
 * Shown and hidden by UInteractPromptComponent, which owns the widget component this lives in.
 * The ring stays on screen for as long as the prompt does and reads as an empty track at 0, so
 * the button always communicates that it can be held.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Drives the radial fill. 0 leaves the ring empty rather than hiding it. */
	UFUNCTION(BlueprintCallable, Category = "Interact Prompt")
	void SetHoldProgress(float Normalized);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ButtonIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> HoldProgressRing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	TSoftObjectPtr<UTexture2D> KeyboardIcon =
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Button/keyBoard/E_Key_Light.E_Key_Light")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	TSoftObjectPtr<UTexture2D> GamepadIcon =
		TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/Button/Xbox/XboxSeriesX_A.XboxSeriesX_A")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt", meta = (ClampMin = "8.0"))
	float IconSize = 32.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt", meta = (ClampMin = "8.0"))
	float RingSize = 64.f;

private:
	void BuildFallbackLayout();
	void RefreshIcon(ECommonInputType NewInputType = ECommonInputType::MouseAndKeyboard);
};
