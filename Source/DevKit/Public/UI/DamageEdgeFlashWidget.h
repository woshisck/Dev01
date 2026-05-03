#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageEdgeFlashWidget.generated.h"

UCLASS()
class DEVKIT_API UDamageEdgeFlashWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Damage Feedback")
	void PlayEdgeFlash(FLinearColor InColor, float InMaxAlpha, float InDuration, float InEdgeWidthRatio);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	FLinearColor FlashColor = FLinearColor::Red;
	float MaxAlpha = 0.f;
	float Duration = 0.f;
	float Elapsed = 0.f;
	float EdgeWidthRatio = 0.1f;
	bool bActive = false;
};
