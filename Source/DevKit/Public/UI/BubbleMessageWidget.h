#pragma once

#include "CoreMinimal.h"
#include "CommonUserWidget.h"
#include "BubbleMessageWidget.generated.h"

class UTextBlock;

/**
 * One line of bubble text plus an optional speaker name.
 *
 * Used by both anchor modes: UBubbleMessageComponent hosts it in a screen-space widget component
 * above a speaker, and AYogHUD hosts it directly in the viewport for the screen-corner path.
 * The widget itself is passive — it never decides when it is shown or where it sits.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBubbleMessageWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	/** An empty SpeakerName collapses the name block rather than leaving a blank row. */
	UFUNCTION(BlueprintCallable, Category = "Bubble")
	void SetLine(const FText& SpeakerName, const FText& Body);

	UFUNCTION(BlueprintCallable, Category = "Bubble")
	void SetFadeAlpha(float Alpha);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> BodyText;

private:
	/** Builds a usable text-only layout so the bubble still reads before a WBP is authored. */
	void BuildFallbackLayout();
};
