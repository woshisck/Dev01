#include "UI/CombatLogStatics.h"

// 静态成员定义
TArray<FDamageBreakdown> UCombatLogStatics::Entries;
int32 UCombatLogStatics::Version = 0;

void UCombatLogStatics::PushEntry(const FDamageBreakdown& Entry)
{
	Entries.Add(Entry);

	// 超出上限时移除最旧的一条
	if (Entries.Num() > MaxEntries)
		Entries.RemoveAt(0);

	++Version;
}

int32 UCombatLogStatics::GetVersion()
{
	return Version;
}

const TArray<FDamageBreakdown>& UCombatLogStatics::GetAllEntries()
{
	return Entries;
}

int32 UCombatLogStatics::GetEntryCount()
{
	return Entries.Num();
}

void UCombatLogStatics::ClearEntries()
{
	Entries.Empty();
	++Version;
}

FString UCombatLogStatics::GetEntryText(const FDamageBreakdown& Entry)
{
	const int32 Sec   = FMath::FloorToInt(Entry.GameTime);
	const FString Time  = FString::Printf(TEXT("[%02d:%02d]"), Sec / 60, Sec % 60);
	const FString Arrow = FString::Printf(TEXT(" %s→%s  "), *Entry.SourceName, *Entry.TargetName);
	const FString Type  = Entry.DamageType.ToString();

	if (Type.StartsWith("Rune") || Entry.DamageType == FName("Bleed"))
		return Time + Arrow + FString::Printf(TEXT("[%s] → %.1f"), *Entry.ActionName.ToString(), Entry.FinalDamage);

	const FString Crit = Entry.bIsCrit ? TEXT(" ★CRIT") : TEXT("");
	return Time + Arrow + FString::Printf(TEXT("[%s]  %.0f×%.2f×%.2f%s = %.1f"),
		*Entry.ActionName.ToString(),
		Entry.BaseAttack, Entry.ActionMultiplier, Entry.DmgTakenMult,
		*Crit, Entry.FinalDamage);
}

FLinearColor UCombatLogStatics::GetEntryColor(const FDamageBreakdown& Entry)
{
	if (Entry.DamageType == FName("Bleed"))             return FLinearColor(1.f, 0.2f, 0.2f);
	if (Entry.DamageType.ToString().StartsWith("Rune")) return FLinearColor(0.8f, 0.4f, 1.f);
	if (Entry.bIsCrit)                                  return FLinearColor(1.f, 0.9f, 0.1f);
	return FLinearColor(1.f, 1.f, 1.f);
}

bool UCombatLogStatics::PassesFilter(const FDamageBreakdown& Entry, ECombatLogFilter Filter)
{
	switch (Filter)
	{
	case ECombatLogFilter::All:    return true;
	case ECombatLogFilter::Normal: return !Entry.bIsCrit && Entry.DamageType != FName("Bleed") && !Entry.DamageType.ToString().StartsWith("Rune");
	case ECombatLogFilter::Crit:   return Entry.bIsCrit;
	case ECombatLogFilter::Rune:   return Entry.DamageType.ToString().StartsWith("Rune");
	case ECombatLogFilter::Bleed:  return Entry.DamageType == FName("Bleed");
	default:                       return true;
	}
}

FString UCombatLogStatics::GetFormattedLog(ECombatLogFilter Filter)
{
	FString Result;

	for (const FDamageBreakdown& E : Entries)
	{
		// 过滤
		bool bPass = false;
		switch (Filter)
		{
		case ECombatLogFilter::All:    bPass = true; break;
		case ECombatLogFilter::Normal: bPass = !E.bIsCrit && E.DamageType != FName("Bleed") && !E.DamageType.ToString().StartsWith("Rune"); break;
		case ECombatLogFilter::Crit:   bPass = E.bIsCrit; break;
		case ECombatLogFilter::Rune:   bPass = E.DamageType.ToString().StartsWith("Rune"); break;
		case ECombatLogFilter::Bleed:  bPass = (E.DamageType == FName("Bleed")); break;
		}
		if (!bPass) continue;

		// 时间戳
		const int32 Sec = FMath::FloorToInt(E.GameTime);
		const FString Time = FString::Printf(TEXT("[%02d:%02d]"), Sec / 60, Sec % 60);

		// 来源 → 目标
		const FString Arrow = FString::Printf(TEXT(" %s→%s  "), *E.SourceName, *E.TargetName);

		// 伤害内容
		FString Body;
		const FString TypeStr = E.DamageType.ToString();
		if (TypeStr.StartsWith("Rune") || E.DamageType == FName("Bleed"))
		{
			Body = FString::Printf(TEXT("[%s] → %.1f"), *E.ActionName.ToString(), E.FinalDamage);
		}
		else
		{
			const FString Crit = E.bIsCrit ? TEXT(" ★CRIT") : TEXT("");
			Body = FString::Printf(TEXT("[%s]  %.0f×%.2f×%.2f%s = %.1f"),
				*E.ActionName.ToString(),
				E.BaseAttack, E.ActionMultiplier, E.DmgTakenMult,
				*Crit, E.FinalDamage);
		}

		Result += Time + Arrow + Body + TEXT("\n");
	}

	if (Result.IsEmpty())
		Result = TEXT("（暂无记录）");

	return Result;
}

FString UCombatLogStatics::GetFormattedSummary()
{
	float Normal = 0, Crit = 0, Rune = 0, Bleed = 0;
	int32 HN = 0, HC = 0, HR = 0, HB = 0;

	for (const FDamageBreakdown& E : Entries)
	{
		const FString T = E.DamageType.ToString();
		if      (T.StartsWith("Rune"))           { Rune  += E.FinalDamage; HR++; }
		else if (E.DamageType == FName("Bleed")) { Bleed += E.FinalDamage; HB++; }
		else if (E.bIsCrit)                      { Crit  += E.FinalDamage; HC++; }
		else                                     { Normal+= E.FinalDamage; HN++; }
	}

	return FString::Printf(
		TEXT("──── 本次战斗 ────\n")
		TEXT("普通攻击  %3d次   %.1f\n")
		TEXT("暴击      %3d次   %.1f\n")
		TEXT("符文      %3d次   %.1f\n")
		TEXT("流血      %3d次   %.1f\n")
		TEXT("──────────────────\n")
		TEXT("合计             %.1f"),
		HN, Normal, HC, Crit, HR, Rune, HB, Bleed,
		Normal + Crit + Rune + Bleed);
}
