#include "UI/CombatLogWidget.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"

// ============================================================
//  Tick：轮询版本号，变化时重建
// ============================================================

void UCombatLogWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	const int32 CurrentVersion = UCombatLogStatics::GetVersion();
	if (CurrentVersion == CachedVersion)
		return;

	CachedVersion = CurrentVersion;
	RebuildLog();
	RefreshSummary();
}

// ============================================================
//  过滤器
// ============================================================

void UCombatLogWidget::SetFilter(ECombatLogFilter NewFilter)
{
	if (ActiveFilter == NewFilter)
		return;
	ActiveFilter = NewFilter;
	RebuildLog();
}

bool UCombatLogWidget::PassesFilter(const FDamageBreakdown& Entry) const
{
	switch (ActiveFilter)
	{
	case ECombatLogFilter::All:    return true;
	case ECombatLogFilter::Normal: return !Entry.bIsCrit
									&& Entry.DamageType != FName("Bleed")
									&& !Entry.DamageType.ToString().StartsWith("Rune");
	case ECombatLogFilter::Crit:   return Entry.bIsCrit;
	case ECombatLogFilter::Rune:   return Entry.DamageType.ToString().StartsWith("Rune");
	case ECombatLogFilter::Bleed:  return Entry.DamageType == FName("Bleed");
	default:                       return true;
	}
}

// ============================================================
//  重置
// ============================================================

void UCombatLogWidget::ResetLog()
{
	UCombatLogStatics::ClearEntries();
	SessionNormal = SessionCrit = SessionRune = SessionBleed = 0.f;
	HitNormal = HitCrit = HitRune = HitBleed = 0;
	if (LogScrollBox) LogScrollBox->ClearChildren();
	RefreshSummary();
}

// ============================================================
//  日志构建
// ============================================================

void UCombatLogWidget::RebuildLog()
{
	if (!LogScrollBox)
		return;

	SessionNormal = SessionCrit = SessionRune = SessionBleed = 0.f;
	HitNormal = HitCrit = HitRune = HitBleed = 0;
	LogScrollBox->ClearChildren();

	for (const FDamageBreakdown& E : UCombatLogStatics::GetAllEntries())
	{
		const FString TypeStr = E.DamageType.ToString();
		if      (TypeStr.StartsWith("Rune"))          { SessionRune   += E.FinalDamage; HitRune++;   }
		else if (E.DamageType == FName("Bleed"))      { SessionBleed  += E.FinalDamage; HitBleed++;  }
		else if (E.bIsCrit)                           { SessionCrit   += E.FinalDamage; HitCrit++;   }
		else                                          { SessionNormal += E.FinalDamage; HitNormal++; }

		if (PassesFilter(E))
			AddLogRow(E);
	}
}

void UCombatLogWidget::AddLogRow(const FDamageBreakdown& Entry)
{
	if (!LogScrollBox) return;

	const float OffsetEnd    = LogScrollBox->GetScrollOffsetOfEnd();
	const float OffsetNow    = LogScrollBox->GetScrollOffset();
	const bool  bWasAtBottom = (OffsetEnd - OffsetNow) < 4.f;

	UTextBlock* Row = NewObject<UTextBlock>(LogScrollBox);
	Row->SetText(FText::FromString(FormatEntry(Entry)));
	Row->SetColorAndOpacity(FSlateColor(GetEntryColor(Entry)));

	FSlateFontInfo FontInfo = Row->GetFont();
	FontInfo.Size = LogFontSize;
	Row->SetFont(FontInfo);

	LogScrollBox->AddChild(Row);
	if (bWasAtBottom) LogScrollBox->ScrollToEnd();
}

FString UCombatLogWidget::FormatEntry(const FDamageBreakdown& Entry) const
{
	const int32   Sec     = FMath::FloorToInt(Entry.GameTime);
	const FString Time    = FString::Printf(TEXT("[%02d:%02d]"), Sec / 60, Sec % 60);
	const FString Arrow   = FString::Printf(TEXT(" %s → %s  "), *Entry.SourceName, *Entry.TargetName);
	const FString TypeStr = Entry.DamageType.ToString();

	if (TypeStr.StartsWith("Rune") || Entry.DamageType == FName("Bleed"))
	{
		return Time + Arrow +
			FString::Printf(TEXT("[%s] → %.1f"), *Entry.ActionName.ToString(), Entry.FinalDamage);
	}
	const FString Crit = Entry.bIsCrit ? TEXT(" ★CRIT") : TEXT("");
	return Time + Arrow +
		FString::Printf(TEXT("[%s]  %.0f × %.2f × %.2f%s = %.1f"),
			*Entry.ActionName.ToString(),
			Entry.BaseAttack, Entry.ActionMultiplier, Entry.DmgTakenMult,
			*Crit, Entry.FinalDamage);
}

FLinearColor UCombatLogWidget::GetEntryColor(const FDamageBreakdown& Entry) const
{
	if (Entry.DamageType == FName("Bleed"))             return FLinearColor(1.f, 0.2f, 0.2f);
	if (Entry.DamageType.ToString().StartsWith("Rune")) return FLinearColor(0.8f, 0.4f, 1.f);
	if (Entry.bIsCrit)                                  return FLinearColor(1.f, 0.9f, 0.1f);
	return FLinearColor(1.f, 1.f, 1.f);
}

// ============================================================
//  统计汇总
// ============================================================

void UCombatLogWidget::RefreshSummary()
{
	if (!SummaryText) return;

	const float Total = SessionNormal + SessionCrit + SessionRune + SessionBleed;
	SummaryText->SetText(FText::FromString(FString::Printf(
		TEXT("──── 本次战斗 ────\n")
		TEXT("普通攻击  %3d次   %.1f\n")
		TEXT("暴击      %3d次   %.1f\n")
		TEXT("符文      %3d次   %.1f\n")
		TEXT("流血      %3d次   %.1f\n")
		TEXT("──────────────────\n")
		TEXT("合计             %.1f"),
		HitNormal, SessionNormal, HitCrit, SessionCrit,
		HitRune, SessionRune, HitBleed, SessionBleed, Total)));
}
