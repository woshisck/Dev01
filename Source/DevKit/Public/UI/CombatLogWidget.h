#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "UI/CombatLogStatics.h"
#include "CombatLogWidget.generated.h"

class UScrollBox;
class UTextBlock;
class UWrapBox;
class UComboBoxString;
class UCheckBox;
class USlider;
class UButton;
class UCombatLogWidget;

// 按钮点击代理：持有过滤枚举，供 UButton.OnClicked 动态绑定 ──────────────
UCLASS()
class DEVKIT_API UCombatFilterProxy : public UObject
{
	GENERATED_BODY()
public:
	ECombatLogFilter            Filter = ECombatLogFilter::All;
	TWeakObjectPtr<UCombatLogWidget> Owner;

	UFUNCTION()
	void OnClicked();
};

/**
 * 战斗日志显示基类（UUserWidget）
 *
 * 数据来自 UCombatLogStatics 静态桥（NativeTick 轮询版本号）。
 *
 * Blueprint 使用步骤：
 *   将 EUW_CombatLog 的父类改为 CombatLogWidget，
 *   Designer 中放置：
 *     [WrapBox]   "FilterButtonBox"  ← 过滤按钮自动生成，无需手动摆
 *     [ScrollBox] "LogScrollBox"
 *     [TextBlock] "SummaryText"
 *   重置按钮 OnClicked → ResetLog()
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UCombatLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── 自动绑定控件 ────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UWrapBox> FilterButtonBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> LogScrollBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	// ── 配置 ────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 LogFontSize = 11;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 FilterFontSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 SummaryFontSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MinLogFontSize = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxLogFontSize = 16;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float ControlBarHeight = 104.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float SummaryHeight = 126.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float DefaultTimeWindowSeconds = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float MaxTimeWindowSeconds = 60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bDefaultDebugMode = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FMargin LogRowPadding = FMargin(2.f, 2.f, 2.f, 2.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FLinearColor LogRowBackgroundColor = FLinearColor(0.03f, 0.05f, 0.06f, 0.35f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FLinearColor PanelBackgroundColor = FLinearColor(0.015f, 0.018f, 0.022f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FLinearColor ControlTextColor = FLinearColor(0.78f, 0.84f, 0.86f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	FLinearColor SummaryTextColor = FLinearColor(0.64f, 0.72f, 0.73f, 1.f);

	// ── 蓝图可调用 ──────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void SetFilter(ECombatLogFilter NewFilter);

	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void ResetLog();

	UFUNCTION(BlueprintPure, Category = "CombatLog")
	ECombatLogFilter GetActiveFilter() const { return ActiveFilter; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void ConfigureWidgetLayout();
	void BuildFilterButtons();
	void BuildFilterButtonsLegacy();
	void RefreshFilterButtonVisuals();
	void RefreshFilterOptions();
	void RefreshTimeWindowText();
	void RebuildLog();
	void AddLogRow(const FDamageBreakdown& Entry);
	void RefreshSummary();
	void RefreshSummaryLegacy();
	void ApplySummaryTextStyle();
	int32 GetEffectiveLogFontSize() const;
	int32 GetEffectiveFilterFontSize() const;
	int32 GetEffectiveSummaryFontSize() const;
	FString      FormatEntry(const FDamageBreakdown& Entry) const;
	FLinearColor GetEntryColor(const FDamageBreakdown& Entry) const;
	bool         PassesFilter(const FDamageBreakdown& Entry) const;
	float        GetLatestEntryTime() const;

	UFUNCTION()
	void OnSourceFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnTargetFilterChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	UFUNCTION()
	void OnDebugModeChanged(bool bIsChecked);

	UFUNCTION()
	void OnTimeWindowSliderChanged(float Value);

	ECombatLogFilter ActiveFilter  = ECombatLogFilter::All;
	int32            CachedVersion = -1;
	FString          ActiveSourceFilter = TEXT("全部");
	FString          ActiveTargetFilter = TEXT("全部");
	float            ActiveTimeWindowSeconds = 30.f;
	bool             bDebugMode = false;
	bool             bRefreshingFilterOptions = false;

	TArray<TObjectPtr<UCombatFilterProxy>> FilterProxies;
	TObjectPtr<UComboBoxString> SourceComboBox;
	TObjectPtr<UComboBoxString> TargetComboBox;
	TObjectPtr<UCheckBox> DebugModeCheckBox;
	TObjectPtr<USlider> TimeWindowSlider;
	TObjectPtr<UTextBlock> TimeWindowText;
	TMap<ECombatLogFilter, TObjectPtr<UButton>> FilterButtons;
	TMap<ECombatLogFilter, TObjectPtr<UTextBlock>> FilterButtonLabels;

	// 传统伤害统计
	float SessionNormal = 0.f, SessionCrit  = 0.f;
	float SessionRune   = 0.f, SessionBleed = 0.f;
	int32 HitNormal     = 0,   HitCrit      = 0;
	int32 HitRune       = 0,   HitBleed     = 0;

	// 512版本：卡牌统计
	int32 HitCardConsume   = 0, HitCardHit     = 0, HitCardMatched  = 0;
	int32 HitCardLink      = 0, HitCardFinisher = 0, HitCardShuffle = 0;
	float SessionCardHit   = 0.f, SessionCardLink = 0.f, SessionCardFinisher = 0.f;
};
