#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CommonInputTypeEnum.h"
#include "InteractPromptWidget.generated.h"

class UBorder;
class UYogCommonRichTextBlock;

UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UInteractPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Interact Prompt")
	void SetPromptLabel(const FText& InLabel);

	static FText MakePromptMarkup(const FText& Label);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> PromptBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UYogCommonRichTextBlock> PromptText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	FText PromptLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	int32 PromptFontSize = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	FLinearColor PromptTextColor = FLinearColor(0.94f, 0.92f, 0.84f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	FLinearColor PromptFillColor = FLinearColor(0.02f, 0.025f, 0.03f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interact Prompt")
	FLinearColor PromptBorderColor = FLinearColor(0.75f, 0.66f, 0.42f, 0.85f);

private:
	void BuildFallbackLayout();
	void RefreshPrompt(ECommonInputType NewInputType = ECommonInputType::MouseAndKeyboard);
	void EnsureInputDecorator();
};
