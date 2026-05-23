#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FinisherQTEWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UProgressBar;
class UTextBlock;
class UWidget;

UCLASS()
class DEVKIT_API UFinisherQTEWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Finisher QTE")
	void ShowPrompt(float WindowDuration);

	UFUNCTION(BlueprintCallable, Category = "Finisher QTE")
	void HidePrompt();

	UFUNCTION(BlueprintCallable, Category = "Finisher QTE")
	void MarkConfirmed();

	UFUNCTION(BlueprintPure, Category = "Finisher QTE")
	bool IsPromptActive() const { return bPromptActive; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PromptPanel;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> KeyText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> PromptText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> WindowProgressBar;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher QTE")
	FText KeyLabel = FText::FromString(TEXT("<input action=\"HeavyAttack\"/>"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher QTE")
	FText ActivePrompt = FText::FromString(TEXT("FINISHER"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Finisher QTE")
	FText ConfirmedPrompt = FText::FromString(TEXT("CONFIRMED"));

private:
	UPROPERTY()
	TObjectPtr<UCanvasPanel> RuntimeRoot;

	float StartRealTime = 0.f;
	float Duration = 0.f;
	bool bPromptActive = false;
	bool bConfirmed = false;

	void BuildRuntimeLayout(bool bForceRebuild = false);
	void RefreshVisuals(float RemainingAlpha);
	float GetCurrentRealTime() const;
	bool IsKeyPromptRichText() const;
	void SetTextIfSupported(UWidget* Widget, const FText& Text) const;
};
