#pragma once

#include "CoreMinimal.h"
#include "CommonActivatableWidget.h"
#include "SaveGame/YogSettingsSave.h"
#include "YogGraphicsSettingsWidgetBase.generated.h"

class UButton;
class UCheckBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FYogGraphicsSettingsWidgetAction);

UCLASS(Blueprintable)
class DEVKIT_API UYogGraphicsSettingsWidgetBase : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Settings|Graphics")
	FYogGraphicsSettingsWidgetAction OnBackRequested;

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	static TArray<FName> GetRequiredDesignerWidgetNames();

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	static FName GetDefaultFocusWidgetName();

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void RefreshFromSavedSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingSettings(const FYogGraphicsSettings& InSettings);

	UFUNCTION(BlueprintPure, Category = "Settings|Graphics")
	FYogGraphicsSettings GetPendingSettings() const { return PendingSettings; }

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void ApplyProfile(EYogPerformanceProfile Profile);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void ApplyTargetTier(EYogPerformanceTargetTier TargetTier);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	bool ApplyPendingSettings(bool bSaveToDisk = true);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingResolutionScale(float ResolutionScalePercent);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingFrameRateLimit(int32 FrameRateLimit);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingLumenLite(bool bUseLumenLite);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingBatchProxies(bool bPreferBatchedGeometryProxies);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingModelQuality(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingShadowQuality(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingReflectionQuality(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingTextureQuality(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingMaterialQuality(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingDynamicLightQuality(int32 Quality);

	UFUNCTION(BlueprintCallable, Category = "Settings|Graphics")
	void SetPendingMaterialLightQuality(int32 Quality);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnTierEpic;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnTierHigh;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnTierMid;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnTierLow;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnFrame30;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnFrame40;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnFrame60;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnFrame120;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnFrameUnlimited;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnApplyCustom;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UButton> BtnBack;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> ResolutionScaleSlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UCheckBox> LumenLiteCheckBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UCheckBox> BatchProxiesCheckBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> ModelQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> ShadowQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> ReflectionQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> TextureQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> MaterialQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> DynamicLightQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<USlider> MaterialLightQualitySlider;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> CurrentProfileText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> ResolutionScaleText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> FrameRateText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> ModelQualityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> ShadowQualityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> ReflectionQualityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> TextureQualityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> MaterialQualityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> DynamicLightQualityText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional), Category = "Settings|Graphics")
	TObjectPtr<UTextBlock> MaterialLightQualityText;

private:
	void BuildFallbackLayout();
	void BindControls();
	void SyncControlsFromPendingSettings();
	void MarkPendingAsCustom();

	UFUNCTION()
	void HandleTierEpicClicked();

	UFUNCTION()
	void HandleTierHighClicked();

	UFUNCTION()
	void HandleTierMidClicked();

	UFUNCTION()
	void HandleTierLowClicked();

	UFUNCTION()
	void HandleFrame30Clicked();

	UFUNCTION()
	void HandleFrame40Clicked();

	UFUNCTION()
	void HandleFrame60Clicked();

	UFUNCTION()
	void HandleFrame120Clicked();

	UFUNCTION()
	void HandleFrameUnlimitedClicked();

	UFUNCTION()
	void HandleApplyCustomClicked();

	UFUNCTION()
	void HandleBackClicked();

	UFUNCTION()
	void HandleResolutionScaleChanged(float NewValue);

	UFUNCTION()
	void HandleLumenLiteChanged(bool bIsChecked);

	UFUNCTION()
	void HandleBatchProxiesChanged(bool bIsChecked);

	UFUNCTION()
	void HandleModelQualityChanged(float NewValue);

	UFUNCTION()
	void HandleShadowQualityChanged(float NewValue);

	UFUNCTION()
	void HandleReflectionQualityChanged(float NewValue);

	UFUNCTION()
	void HandleTextureQualityChanged(float NewValue);

	UFUNCTION()
	void HandleMaterialQualityChanged(float NewValue);

	UFUNCTION()
	void HandleDynamicLightQualityChanged(float NewValue);

	UFUNCTION()
	void HandleMaterialLightQualityChanged(float NewValue);

	UPROPERTY()
	FYogGraphicsSettings PendingSettings;

	bool bBindingControls = false;
};
