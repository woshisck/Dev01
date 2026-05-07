#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "UI/CombatLogStatics.h"
#include "CombatLogEditorUtilityWidget.generated.h"

class UButton;
class UCheckBox;
class UComboBoxString;
class UScrollBox;
class USlider;
class UTextBlock;
class UWrapBox;
class UCombatLogEditorUtilityWidget;
class SWidget;

UCLASS()
class DEVKITEDITOR_API UCombatLogEditorFilterProxy : public UObject
{
	GENERATED_BODY()

public:
	ECombatLogFilter Filter = ECombatLogFilter::All;
	TWeakObjectPtr<UCombatLogEditorUtilityWidget> Owner;

	UFUNCTION()
	void OnClicked();
};

UCLASS(Blueprintable, BlueprintType)
class DEVKITEDITOR_API UCombatLogEditorUtilityWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> FilterButtonBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> LogScrollBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Style")
	int32 LogFontSize = 11;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Style")
	int32 FilterFontSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Style")
	int32 SummaryFontSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Style")
	float ControlBarHeight = 104.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Style")
	float SummaryHeight = 126.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Filter")
	float DefaultTimeWindowSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Filter")
	float MaxTimeWindowSeconds = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CombatLog|Filter")
	bool bDefaultDebugMode = false;

	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void SetFilter(ECombatLogFilter NewFilter);

	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void ResetLog();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void EnsureLayout();
	void BuildControlBar();
	void RefreshFilterButtonVisuals();
	void RefreshFilterOptions();
	void RefreshTimeWindowText();
	void RebuildLog();
	void AddLogRow(const FDamageBreakdown& Entry);
	void RefreshSummary();
	bool PassesFilter(const FDamageBreakdown& Entry) const;
	float GetLatestEntryTime() const;
	int32 GetSafeLogFontSize() const;
	int32 GetSafeFilterFontSize() const;
	int32 GetSafeSummaryFontSize() const;

	UFUNCTION()
	void OnSourceFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnTargetFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnDebugModeChanged(bool bIsChecked);

	UFUNCTION()
	void OnTimeWindowSliderChanged(float Value);

	ECombatLogFilter ActiveFilter = ECombatLogFilter::All;
	int32 CachedVersion = -1;
	FString ActiveSourceFilter = TEXT("全部");
	FString ActiveTargetFilter = TEXT("全部");
	float ActiveTimeWindowSeconds = 30.f;
	bool bDebugMode = false;
	bool bRefreshingFilterOptions = false;

	TArray<TObjectPtr<UCombatLogEditorFilterProxy>> FilterProxies;
	TObjectPtr<UComboBoxString> SourceComboBox;
	TObjectPtr<UComboBoxString> TargetComboBox;
	TObjectPtr<UCheckBox> DebugModeCheckBox;
	TObjectPtr<USlider> TimeWindowSlider;
	TObjectPtr<UTextBlock> TimeWindowText;
	TMap<ECombatLogFilter, TObjectPtr<UButton>> FilterButtons;
	TMap<ECombatLogFilter, TObjectPtr<UTextBlock>> FilterButtonLabels;

	float SessionNormal = 0.f;
	float SessionCrit = 0.f;
	float SessionRune = 0.f;
	float SessionBleed = 0.f;
	float SessionCardHit = 0.f;
	float SessionCardLink = 0.f;
	float SessionCardFinisher = 0.f;

	int32 HitNormal = 0;
	int32 HitCrit = 0;
	int32 HitRune = 0;
	int32 HitBleed = 0;
	int32 HitCardConsume = 0;
	int32 HitCardHit = 0;
	int32 HitCardMatched = 0;
	int32 HitCardLink = 0;
	int32 HitCardFinisher = 0;
	int32 HitCardShuffle = 0;
};
