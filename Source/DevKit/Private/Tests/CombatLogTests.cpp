#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UI/CombatLogStatics.h"

namespace
{
	FString JoinCombatLogSegments(const TArray<FCombatLogTextSegment>& Segments)
	{
		FString Result;
		for (const FCombatLogTextSegment& Segment : Segments)
		{
			Result += Segment.Text;
		}
		return Result;
	}

	FDamageBreakdown MakeCombatLogEntry(
		const FName DamageType,
		const FString& SourceName,
		const FString& TargetName,
		const float GameTime,
		const float FinalDamage)
	{
		FDamageBreakdown Entry;
		Entry.DamageType = DamageType;
		Entry.SourceName = SourceName;
		Entry.TargetName = TargetName;
		Entry.GameTime = GameTime;
		Entry.FinalDamage = FinalDamage;
		Entry.ActionName = FName(TEXT("轻击1"));
		Entry.BaseAttack = 45.f;
		Entry.ActionMultiplier = 1.8f;
		Entry.DmgTakenMult = 1.f;
		return Entry;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLogStorageVersionAndLimitTest,
	"DevKit.UI.CombatLog.StorageVersionAndLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLogStorageVersionAndLimitTest::RunTest(const FString& Parameters)
{
	UCombatLogStatics::ClearEntries();
	const int32 VersionAfterClear = UCombatLogStatics::GetVersion();

	for (int32 Index = 0; Index < UCombatLogStatics::MaxEntries + 1; ++Index)
	{
		FDamageBreakdown Entry = MakeCombatLogEntry(
			FName(TEXT("Attack")),
			TEXT("BP_PlayerCharacter_C_0"),
			FString::Printf(TEXT("BP_Enemy_Rat_C_%d"), Index),
			static_cast<float>(Index),
			static_cast<float>(Index + 1));
		UCombatLogStatics::PushEntry(Entry);
	}

	TestEqual(TEXT("Combat log keeps the configured maximum number of entries"),
		UCombatLogStatics::GetEntryCount(),
		UCombatLogStatics::MaxEntries);
	TestEqual(TEXT("PushEntry increments the version once per entry"),
		UCombatLogStatics::GetVersion(),
		VersionAfterClear + UCombatLogStatics::MaxEntries + 1);

	const TArray<FDamageBreakdown>& Entries = UCombatLogStatics::GetAllEntries();
	TestTrue(TEXT("Oldest entry is evicted after exceeding MaxEntries"),
		Entries.Num() > 0 && FMath::IsNearlyEqual(Entries[0].FinalDamage, 2.f));

	UCombatLogStatics::ClearEntries();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLogNameBeautifyTest,
	"DevKit.UI.CombatLog.NameBeautify",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLogNameBeautifyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Blueprint generated names are cleaned for display"),
		UCombatLogStatics::GetDisplayActorName(TEXT("BP_Enemy_Rat_C_12")),
		FString(TEXT("Enemy Rat")));
	TestEqual(TEXT("Already readable Chinese names are preserved"),
		UCombatLogStatics::GetDisplayActorName(TEXT("近战兵")),
		FString(TEXT("近战兵")));
	TestEqual(TEXT("Empty actor names use a safe placeholder"),
		UCombatLogStatics::GetDisplayActorName(TEXT("")),
		FString(TEXT("未知目标")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLogAdvancedFilterTest,
	"DevKit.UI.CombatLog.AdvancedFilter",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLogAdvancedFilterTest::RunTest(const FString& Parameters)
{
	const FDamageBreakdown RecentPlayerHit = MakeCombatLogEntry(
		FName(TEXT("Attack")),
		TEXT("BP_PlayerCharacter_C_0"),
		TEXT("BP_Enemy_Rat_C_7"),
		50.f,
		81.f);
	const FDamageBreakdown OldPlayerHit = MakeCombatLogEntry(
		FName(TEXT("Attack")),
		TEXT("BP_PlayerCharacter_C_0"),
		TEXT("BP_Enemy_Rat_C_7"),
		10.f,
		12.f);
	const FDamageBreakdown RecentRuneHit = MakeCombatLogEntry(
		FName(TEXT("Rune_Moonlight_Base")),
		TEXT("BP_PlayerCharacter_C_0"),
		TEXT("BP_Enemy_Rat_C_7"),
		49.f,
		32.f);

	TestTrue(TEXT("Advanced filter accepts matching source, target, type and time window"),
		UCombatLogStatics::PassesAdvancedFilter(RecentPlayerHit, ECombatLogFilter::Normal,
			TEXT("PlayerCharacter"), TEXT("Enemy Rat"), 50.f, 30.f));
	TestFalse(TEXT("Advanced filter rejects entries outside the time window"),
		UCombatLogStatics::PassesAdvancedFilter(OldPlayerHit, ECombatLogFilter::Normal,
			TEXT("PlayerCharacter"), TEXT("Enemy Rat"), 50.f, 30.f));
	TestFalse(TEXT("Advanced filter still respects the type filter"),
		UCombatLogStatics::PassesAdvancedFilter(RecentRuneHit, ECombatLogFilter::Normal,
			TEXT("PlayerCharacter"), TEXT("Enemy Rat"), 50.f, 30.f));
	TestTrue(TEXT("Zero time window means all history"),
		UCombatLogStatics::PassesAdvancedFilter(OldPlayerHit, ECombatLogFilter::Normal,
			TEXT("全部"), TEXT("全部"), 50.f, 0.f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLogReadableSegmentsTest,
	"DevKit.UI.CombatLog.ReadableSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLogReadableSegmentsTest::RunTest(const FString& Parameters)
{
	FDamageBreakdown Entry = MakeCombatLogEntry(
		FName(TEXT("Attack")),
		TEXT("BP_PlayerCharacter_C_0"),
		TEXT("BP_Enemy_Rat_C_7"),
		591.32f,
		81.f);
	Entry.bHasTargetVitals = true;
	Entry.TargetHealthBefore = 392.f;
	Entry.TargetHealthAfter = 311.f;

	const TArray<FCombatLogTextSegment> ReadableSegments = UCombatLogStatics::GetEntryTextSegments(Entry, false);
	const FString ReadableText = JoinCombatLogSegments(ReadableSegments);
	TestTrue(TEXT("Readable log uses Dota-style time"),
		ReadableText.Contains(TEXT("[09:51.32]")));
	TestTrue(TEXT("Readable log describes source, target and damage as a sentence"),
		ReadableText.Contains(TEXT("PlayerCharacter击中了Enemy Rat，造成81点伤害")));
	TestTrue(TEXT("Readable log displays target health delta when available"),
		ReadableText.Contains(TEXT("(392->311)")));
	TestFalse(TEXT("Readable log hides internal debug formula by default"),
		ReadableText.Contains(TEXT("45x1.80x1.00")));

	const bool bHasDamageColor = ReadableSegments.ContainsByPredicate([](const FCombatLogTextSegment& Segment)
	{
		return Segment.Text.Contains(TEXT("81")) && Segment.Color.R > 0.9f && Segment.Color.G < 0.3f;
	});
	TestTrue(TEXT("Damage amount segment is red"), bHasDamageColor);

	const bool bHasHealthColor = ReadableSegments.ContainsByPredicate([](const FCombatLogTextSegment& Segment)
	{
		return Segment.Text.Contains(TEXT("392->311")) && Segment.Color.G > 0.8f;
	});
	TestTrue(TEXT("Health delta segment is green"), bHasHealthColor);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatLogDebugSegmentsTest,
	"DevKit.UI.CombatLog.DebugSegments",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatLogDebugSegmentsTest::RunTest(const FString& Parameters)
{
	FDamageBreakdown Entry = MakeCombatLogEntry(
		FName(TEXT("Card_Link")),
		TEXT("玩家"),
		TEXT("BP_Enemy_Rat_C_7"),
		12.f,
		50.f);
	Entry.bTriggeredLink = true;
	Entry.bTriggeredMatchedFlow = true;
	Entry.CardDisplayName = FName(TEXT("月光刃"));

	const FString DebugText = JoinCombatLogSegments(UCombatLogStatics::GetEntryTextSegments(Entry, true));

	TestTrue(TEXT("Debug log includes internal DamageType"),
		DebugText.Contains(TEXT("DamageType=Card_Link")));
	TestTrue(TEXT("Debug log includes the formula"),
		DebugText.Contains(TEXT("45x1.80x1.00")));
	TestTrue(TEXT("Debug log includes card trigger flags"),
		DebugText.Contains(TEXT("Matched")));
	TestTrue(TEXT("Debug log includes link trigger flags"),
		DebugText.Contains(TEXT("Link")));

	return true;
}

#endif
