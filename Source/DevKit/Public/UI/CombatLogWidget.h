#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/CombatLogStatics.h"
#include "CombatLogWidget.generated.h"

class UScrollBox;
class UTextBlock;

/**
 * 战斗日志显示基类（UUserWidget）
 *
 * 数据来自 UCombatLogStatics 静态桥（NativeTick 轮询版本号），
 * 无需委托订阅，天然兼容 Editor Utility Widget。
 *
 * 蓝图使用步骤：
 *   将 EUW_CombatLog 的父类改为 CombatLogWidget，
 *   Designer 中放置：
 *     [ScrollBox]  "LogScrollBox"   ← Editor Utility Scroll Box
 *     [TextBlock]  "SummaryText"
 *   过滤按钮 OnClicked → SetFilter(ECombatLogFilter)
 *   重置按钮 OnClicked → ResetLog()
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UCombatLogWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── 自动绑定控件 ────────────────────────────────────────────
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> LogScrollBox;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	// ── 配置 ────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 LogFontSize = 11;

	// ── 蓝图可调用 ──────────────────────────────────────────────
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void SetFilter(ECombatLogFilter NewFilter);

	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void ResetLog();

	UFUNCTION(BlueprintPure, Category = "CombatLog")
	ECombatLogFilter GetActiveFilter() const { return ActiveFilter; }

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void RebuildLog();
	void AddLogRow(const FDamageBreakdown& Entry);
	void RefreshSummary();
	FString      FormatEntry(const FDamageBreakdown& Entry) const;
	FLinearColor GetEntryColor(const FDamageBreakdown& Entry) const;
	bool         PassesFilter(const FDamageBreakdown& Entry) const;

	ECombatLogFilter ActiveFilter  = ECombatLogFilter::All;
	int32            CachedVersion = -1;

	float SessionNormal = 0.f, SessionCrit  = 0.f;
	float SessionRune   = 0.f, SessionBleed = 0.f;
	int32 HitNormal     = 0,   HitCrit      = 0;
	int32 HitRune       = 0,   HitBleed     = 0;
};
