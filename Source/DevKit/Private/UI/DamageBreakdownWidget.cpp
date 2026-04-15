#include "UI/DamageBreakdownWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Pawn.h"

// ============================================================
//  生命周期
// ============================================================

void UDamageBreakdownWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 订阅玩家 ASC 的伤害明细委托
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UAbilitySystemComponent* RawASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			if (UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(RawASC))
			{
				ASC->OnDamageBreakdown.AddDynamic(this, &UDamageBreakdownWidget::HandleDamageEntry);
			}
		}
	}

	RefreshSummary();
}

void UDamageBreakdownWidget::NativeDestruct()
{
	if (APawn* Pawn = GetOwningPlayerPawn())
	{
		if (UAbilitySystemComponent* RawASC =
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pawn))
		{
			if (UYogAbilitySystemComponent* ASC = Cast<UYogAbilitySystemComponent>(RawASC))
			{
				ASC->OnDamageBreakdown.RemoveDynamic(this, &UDamageBreakdownWidget::HandleDamageEntry);
			}
		}
	}

	Super::NativeDestruct();
}

// ============================================================
//  委托回调
// ============================================================

void UDamageBreakdownWidget::HandleDamageEntry(FDamageBreakdown Entry)
{
	// 记录发生时间（Widget 侧打时间戳）
	if (UWorld* World = GetWorld())
		Entry.GameTime = World->GetTimeSeconds();

	// 加入历史，超出上限时移除最旧的
	RecentEntries.Add(Entry);
	while (RecentEntries.Num() > FMath::Max(1, MaxHistoryEntries))
		RecentEntries.RemoveAt(0);

	// 按类别累计会话统计
	const FString TypeStr = Entry.DamageType.ToString();
	if (TypeStr.StartsWith("Rune"))
	{
		SessionRune += Entry.FinalDamage;
		HitRune++;
	}
	else if (Entry.DamageType == FName("Bleed"))
	{
		SessionBleed += Entry.FinalDamage;
		HitBleed++;
	}
	else if (Entry.bIsCrit)
	{
		SessionCrit += Entry.FinalDamage;
		HitCrit++;
	}
	else
	{
		SessionNormal += Entry.FinalDamage;
		HitNormal++;
	}

	// 增量追加：只有通过过滤器才加行，不重建整个列表
	if (PassesFilter(Entry))
		AddLogRow(Entry);

	RefreshSummary();
}

// ============================================================
//  重置
// ============================================================

void UDamageBreakdownWidget::ResetSession()
{
	RecentEntries.Empty();
	SessionNormal = SessionCrit = SessionRune = SessionBleed = 0.f;
	HitNormal = HitCrit = HitRune = HitBleed = 0;

	if (LogScrollBox)
		LogScrollBox->ClearChildren();

	RefreshSummary();
}

// ============================================================
//  过滤器
// ============================================================

void UDamageBreakdownWidget::SetFilter(ECombatLogFilter NewFilter)
{
	if (ActiveFilter == NewFilter)
		return;

	ActiveFilter = NewFilter;
	RebuildLog();
}

bool UDamageBreakdownWidget::PassesFilter(const FDamageBreakdown& Entry) const
{
	switch (ActiveFilter)
	{
	case ECombatLogFilter::All:
		return true;
	case ECombatLogFilter::Normal:
		return !Entry.bIsCrit
			&& Entry.DamageType != FName("Bleed")
			&& !Entry.DamageType.ToString().StartsWith("Rune");
	case ECombatLogFilter::Crit:
		return Entry.bIsCrit;
	case ECombatLogFilter::Rune:
		return Entry.DamageType.ToString().StartsWith("Rune");
	case ECombatLogFilter::Bleed:
		return Entry.DamageType == FName("Bleed");
	default:
		return true;
	}
}

// ============================================================
//  日志构建
// ============================================================

void UDamageBreakdownWidget::RebuildLog()
{
	if (!LogScrollBox)
		return;

	LogScrollBox->ClearChildren();

	for (const FDamageBreakdown& E : RecentEntries)
	{
		if (PassesFilter(E))
			AddLogRow(E);
	}
}

void UDamageBreakdownWidget::AddLogRow(const FDamageBreakdown& Entry)
{
	if (!LogScrollBox)
		return;

	// 智能自动滚动：仅当用户已在底部时才跟随新条目
	// 阈值 4px，防止浮点误差导致底部判断失效
	const float OffsetEnd    = LogScrollBox->GetScrollOffsetOfEnd();
	const float OffsetNow    = LogScrollBox->GetScrollOffset();
	const bool  bWasAtBottom = (OffsetEnd - OffsetNow) < 4.f;

	UTextBlock* Row = NewObject<UTextBlock>(LogScrollBox);
	Row->SetText(FText::FromString(FormatEntry(Entry)));
	Row->SetColorAndOpacity(FSlateColor(GetEntryColor(Entry)));

	// 应用字体大小（保留控件自身的字体类型）
	FSlateFontInfo FontInfo = Row->GetFont();
	FontInfo.Size = LogFontSize;
	Row->SetFont(FontInfo);

	LogScrollBox->AddChild(Row);

	if (bWasAtBottom)
		LogScrollBox->ScrollToEnd();
}

FString UDamageBreakdownWidget::FormatEntry(const FDamageBreakdown& Entry) const
{
	// 时间戳 [MM:SS]
	const int32 TotalSec = FMath::FloorToInt(Entry.GameTime);
	const FString TimeStr = FString::Printf(TEXT("[%02d:%02d]"), TotalSec / 60, TotalSec % 60);

	// 来源 → 目标
	const FString Arrow = FString::Printf(TEXT(" %s → %s  "),
		*Entry.SourceName, *Entry.TargetName);

	const FString TypeStr = Entry.DamageType.ToString();

	if (TypeStr.StartsWith("Rune") || Entry.DamageType == FName("Bleed"))
	{
		// 流血 / 符文：只显示类型 + 最终伤害
		return TimeStr + Arrow +
			FString::Printf(TEXT("[%s] → %.1f"),
				*Entry.ActionName.ToString(), Entry.FinalDamage);
	}
	else
	{
		// 动作攻击：完整公式展示
		const FString CritStr = Entry.bIsCrit ? TEXT(" ★CRIT") : TEXT("");
		return TimeStr + Arrow +
			FString::Printf(TEXT("[%s]  %.0f × %.2f × %.2f%s = %.1f"),
				*Entry.ActionName.ToString(),
				Entry.BaseAttack,
				Entry.ActionMultiplier,
				Entry.DmgTakenMult,
				*CritStr,
				Entry.FinalDamage);
	}
}

FLinearColor UDamageBreakdownWidget::GetEntryColor(const FDamageBreakdown& Entry) const
{
	const FString TypeStr = Entry.DamageType.ToString();

	if (Entry.DamageType == FName("Bleed"))
		return FLinearColor(1.f, 0.2f, 0.2f);           // 红：流血

	if (TypeStr.StartsWith("Rune"))
		return FLinearColor(0.8f, 0.4f, 1.f);            // 紫：符文

	if (Entry.bIsCrit)
		return FLinearColor(1.f, 0.9f, 0.1f);            // 黄：暴击

	return FLinearColor(1.f, 1.f, 1.f);                  // 白：普通
}

// ============================================================
//  刷新统计汇总
// ============================================================

void UDamageBreakdownWidget::RefreshSummary()
{
	if (!SummaryText)
		return;

	const float Total = SessionNormal + SessionCrit + SessionRune + SessionBleed;

	const FString Summary = FString::Printf(
		TEXT("──── 本次战斗 ────\n")
		TEXT("普通攻击  %3d次   %.1f\n")
		TEXT("暴击      %3d次   %.1f\n")
		TEXT("符文      %3d次   %.1f\n")
		TEXT("流血      %3d次   %.1f\n")
		TEXT("──────────────────\n")
		TEXT("合计             %.1f"),
		HitNormal, SessionNormal,
		HitCrit,   SessionCrit,
		HitRune,   SessionRune,
		HitBleed,  SessionBleed,
		Total);

	SummaryText->SetText(FText::FromString(Summary));
}
