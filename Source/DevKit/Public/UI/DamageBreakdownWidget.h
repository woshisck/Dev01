#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "UI/CombatLogStatics.h"   // ECombatLogFilter + FDamageBreakdown 静态桥
#include "DamageBreakdownWidget.generated.h"

class UTextBlock;
class UScrollBox;

/**
 * 伤害构成调试面板 —— Dota2 风格战斗日志
 *
 * 蓝图创建步骤：
 *   1. 新建 Widget Blueprint，父类选 DamageBreakdownWidget
 *   2. Designer 中放置（名称必须完全一致）：
 *      [CanvasPanel / VerticalBox]
 *        ├── [ScrollBox]  "LogScrollBox"  ← 战斗日志滚动区（推荐高度 300-400px）
 *        └── [TextBlock]  "SummaryText"   ← 会话统计汇总
 *   3. 可选：添加过滤按钮，OnClicked → 调用 SetFilter(ECombatLogFilter)
 *   4. 可选：重置按钮 → 调用 ResetSession()
 *   5. 将 Widget 添加到 Viewport（PlayerController BeginPlay 或 HUD）
 *
 * 日志行格式：
 *   [01:23] 玩家 → BP_Rat  [轻击2]  25 × 1.20 × 1.00 ★CRIT = 30.0
 *   [01:24] 玩家 → BP_Rat  [Bleed] → 12.0
 *   [01:25] 玩家 → BP_Rat  [符文_致命先機] → 45.0
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UDamageBreakdownWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// =========================================================
	// 自动绑定控件（名称必须与 Designer 里一致）
	// =========================================================

	/** 战斗日志滚动区，每条伤害独立一行、独立颜色 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> LogScrollBox;

	/** 会话统计：普通/暴击/符文/流血各自的次数与累计伤害，以及总合计 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SummaryText;

	// =========================================================
	// 配置
	// =========================================================

	/** 历史记录最多保留多少条（超出后移除最旧的） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 MaxHistoryEntries = 200;

	/** 日志行字体大小（建议 10-12） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	int32 LogFontSize = 11;

	// =========================================================
	// 蓝图可调用
	// =========================================================

	/** 清空所有会话统计和记录列表 */
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void ResetSession();

	/** 切换过滤器（供蓝图按钮 OnClicked 调用） */
	UFUNCTION(BlueprintCallable, Category = "CombatLog")
	void SetFilter(ECombatLogFilter NewFilter);

	/** 获取当前过滤器 */
	UFUNCTION(BlueprintPure, Category = "CombatLog")
	ECombatLogFilter GetActiveFilter() const { return ActiveFilter; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	// ── 委托回调 ─────────────────────────────────────────────
	UFUNCTION()
	void HandleDamageEntry(FDamageBreakdown Entry);

	// ── 日志构建 ─────────────────────────────────────────────
	/** 过滤器变更时清空 ScrollBox 并重建所有符合条件的行 */
	void RebuildLog();

	/** 增量添加单行到 ScrollBox（不清空现有行） */
	void AddLogRow(const FDamageBreakdown& Entry);

	/** 格式化单条伤害为显示文本 */
	FString FormatEntry(const FDamageBreakdown& Entry) const;

	/** 根据伤害类型返回行颜色 */
	FLinearColor GetEntryColor(const FDamageBreakdown& Entry) const;

	/** 判断该条目是否通过当前过滤器 */
	bool PassesFilter(const FDamageBreakdown& Entry) const;

	/** 仅刷新 SummaryText */
	void RefreshSummary();

	// ── 数据 ─────────────────────────────────────────────────
	TArray<FDamageBreakdown> RecentEntries;
	ECombatLogFilter ActiveFilter = ECombatLogFilter::All;

	// 会话累计（按伤害类别）
	float SessionNormal = 0.f;
	float SessionCrit   = 0.f;
	float SessionRune   = 0.f;
	float SessionBleed  = 0.f;

	int32 HitNormal = 0;
	int32 HitCrit   = 0;
	int32 HitRune   = 0;
	int32 HitBleed  = 0;
};
