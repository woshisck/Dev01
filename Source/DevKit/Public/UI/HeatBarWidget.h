#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeatBarWidget.generated.h"

class UProgressBar;
class UImage;
class UBackpackGridComponent;

/**
 * Heat bar widget.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UHeatBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Bound background image in the widget blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> HeatBarBG;

    /** Bound progress bar in the widget blueprint. */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HeatBar;

    UFUNCTION(BlueprintNativeEvent)
    void HandleHeatBarUpdate(float NormalizedHeat, int32 NewPhase);

    virtual void HandleHeatBarUpdate_Implementation(float NormalizedHeat, int32 NewPhase);

    /** Latest phase received from the component. */
    UPROPERTY(BlueprintReadOnly, Category = "Heat Bar")
    int32 CurrentPhase = 0;

    /** Phase value before the most recent update. */
    UPROPERTY(BlueprintReadOnly, Category = "Heat Bar")
    int32 PreviousPhase = 0;

    /** True only when the latest update changed phase. */
    UPROPERTY(BlueprintReadOnly, Category = "Heat Bar")
    bool bPhaseChanged = false;

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;
    UBackpackGridComponent* GetBackpack() const;

    UFUNCTION()
    void OnHeatBarUpdateReceived(float NormalizedHeat, int32 NewPhase);

    void RefreshDisplay(float NormalizedHeat, int32 Phase);
};
