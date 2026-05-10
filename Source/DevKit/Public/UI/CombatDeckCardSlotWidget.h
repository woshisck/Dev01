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

	UFUNCTION(BlueprintCallable, Category = "Combat Deck|Animation")
	void PlayUseFlipAnimation();

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

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
	FLinearColor NextCardFrameColor = FLinearColor(0.78f, 0.82f, 0.90f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style")
	FLinearColor NormalCardFrameColor = FLinearColor(0.050f, 0.055f, 0.065f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style")
	FLinearColor EmptyCardFrameColor = FLinearColor(0.020f, 0.022f, 0.026f, 0.46f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style")
	FLinearColor LockedCardFrameColor = FLinearColor(0.94f, 0.68f, 0.20f, 0.92f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Style", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float LockedCardOpacity = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Animation", meta = (ClampMin = "0.01"))
	float UseFlipDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Animation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float UseFlipMinScaleX = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat Deck|Animation", meta = (ClampMin = "1.0"))
	float UseFlipPeakScaleY = 1.06f;

private:
	static FText GetCardDisplayName(const FCombatCardInstance& Card);
	static FText GetActionText(ECardRequiredAction RequiredAction);
	static FText GetTypeText(ECombatCardType CardType);
	static FText GetStateText(const FCombatCardInstance& Card, bool bIsNextCard);
	static void SetTextIfSupported(UWidget* Widget, const FText& Text);

	void ApplyUseFlipTransform(float NormalizedAlpha);
	void ResetUseFlipTransform();

	float UseFlipElapsed = 0.0f;
	bool bUseFlipAnimating = false;
};
