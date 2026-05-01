#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Component/CombatDeckComponent.h"
#include "CombatDeckCardSlotWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;
class UWidget;

UCLASS()
class DEVKIT_API UCombatDeckCardSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void SetCard(const FCombatCardInstance& InCard, bool bIsNextCard);

	UFUNCTION(BlueprintCallable, Category = "Combat Deck")
	void ClearSlot();

protected:
	virtual void NativePreConstruct() override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> CardFrame;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> CardIcon;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CardNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> ActionText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> TypeText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWidget> StateText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style")
	FLinearColor NextCardFrameColor = FLinearColor(0.95f, 0.72f, 0.22f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style")
	FLinearColor NormalCardFrameColor = FLinearColor(0.12f, 0.16f, 0.22f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style")
	FLinearColor EmptyCardFrameColor = FLinearColor(0.04f, 0.05f, 0.07f, 0.45f);

private:
	static FText GetCardDisplayName(const FCombatCardInstance& Card);
	static FText GetActionText(ECardRequiredAction RequiredAction);
	static FText GetTypeText(ECombatCardType CardType);
	static void SetTextIfSupported(UWidget* Widget, const FText& Text);
};
