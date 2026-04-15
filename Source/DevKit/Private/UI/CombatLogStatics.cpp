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
