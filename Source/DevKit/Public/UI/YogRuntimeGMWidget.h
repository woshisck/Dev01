#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "YogRuntimeGMWidget.generated.h"

class UButton;
class USpinBox;
class UTextBlock;
class UWidget;
class UYogRuntimeGMSubsystem;

UCLASS()
class DEVKIT_API UYogRuntimeGMWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	void InitializeRuntimeGM(UYogRuntimeGMSubsystem* InSubsystem);

	UFUNCTION(BlueprintCallable, Category = "Runtime GM")
	void RefreshFromSubsystem();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

private:
	void BuildFallbackWidget();

	UFUNCTION()
	void HandleGiveWeaponClicked();

	UFUNCTION()
	void HandleSpawnEnemyClicked();

	UFUNCTION()
	void HandleResetClicked();

	UFUNCTION()
	void HandleCloseClicked();

	UFUNCTION()
	void HandleSpawnCountChanged(float Value);

	UFUNCTION()
	void HandleSpawnRadiusChanged(float Value);

	UPROPERTY()
	TObjectPtr<UYogRuntimeGMSubsystem> RuntimeGMSubsystem;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> GiveWeaponButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> SpawnEnemyButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResetButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpawnCountSpinBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USpinBox> SpawnRadiusSpinBox;
};
