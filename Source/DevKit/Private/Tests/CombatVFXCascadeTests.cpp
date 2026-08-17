#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/CombatVFXCascade.h"
#include "GameplayTagContainer.h"

namespace
{
	struct FCascadeTestRow
	{
		FCombatVFXMatchRule Rule;
		int32 Payload = 0;
	};

	const FCombatVFXMatchRule& CombatVFXCascadeTests_GetRule(const FCascadeTestRow& Row)
	{
		return Row.Rule;
	}

	FCombatVFXCascadeResult CombatVFXCascadeTests_Resolve(
		const TArray<FCascadeTestRow>& Rows,
		const FGameplayTagContainer& ContextTags,
		int32 MaxAdditiveLayers = CombatVFXCascade::DefaultMaxAdditiveLayers)
	{
		return CombatVFXCascade::Resolve(Rows, &CombatVFXCascadeTests_GetRule, ContextTags, MaxAdditiveLayers);
	}

	FGameplayTag CombatVFXCascadeTests_Tag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), /*ErrorIfNotFound=*/false);
	}

	FCascadeTestRow CombatVFXCascadeTests_MakeRow(const FGameplayTag& MatchTag, int32 Priority, bool bIsAdditive, int32 Payload)
	{
		FCascadeTestRow Row;
		if (MatchTag.IsValid())
		{
			Row.Rule.MatchTags.AddTag(MatchTag);
		}
		Row.Rule.Priority = Priority;
		Row.Rule.bIsAdditive = bIsAdditive;
		Row.Payload = Payload;
		return Row;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadeBasePriorityTest,
	"DevKit.Combat.VFXCascade.HighestPriorityBaseWinsWhole",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadeBasePriorityTest::RunTest(const FString& Parameters)
{
	const FGameplayTag FireTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Fire"));
	const FGameplayTag PoisonTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Poison"));
	TestTrue(TEXT("Buff.Fire is a registered project tag"), FireTag.IsValid());
	TestTrue(TEXT("Buff.Poison is a registered project tag"), PoisonTag.IsValid());
	if (!FireTag.IsValid() || !PoisonTag.IsValid())
	{
		return false;
	}

	TArray<FCascadeTestRow> Rows;
	Rows.Add(CombatVFXCascadeTests_MakeRow(PoisonTag, /*Priority=*/10, /*bIsAdditive=*/false, /*Payload=*/1));
	Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, /*Priority=*/20, /*bIsAdditive=*/false, /*Payload=*/2));

	FGameplayTagContainer Context;
	Context.AddTag(FireTag);
	Context.AddTag(PoisonTag);

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, Context);

	TestTrue(TEXT("A base row was selected"), Result.HasBase());
	TestEqual(TEXT("Higher priority row wins the base slot"), Result.BaseIndex, 1);
	TestEqual(TEXT("Competing non-additive rows do not become additive layers"), Result.AdditiveIndices.Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadeNoMatchTest,
	"DevKit.Combat.VFXCascade.NoMatchYieldsEmptyResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadeNoMatchTest::RunTest(const FString& Parameters)
{
	const FGameplayTag FireTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Fire"));
	const FGameplayTag PoisonTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Poison"));
	if (!FireTag.IsValid() || !PoisonTag.IsValid())
	{
		AddError(TEXT("Buff.Fire / Buff.Poison must be registered project tags"));
		return false;
	}

	TArray<FCascadeTestRow> Rows;
	Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, 10, false, 1));

	FGameplayTagContainer Context;
	Context.AddTag(PoisonTag);

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, Context);

	TestTrue(TEXT("Result is empty when nothing matches"), Result.IsEmpty());
	TestEqual(TEXT("Base index is INDEX_NONE"), Result.BaseIndex, static_cast<int32>(INDEX_NONE));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadeAdditiveLayeringTest,
	"DevKit.Combat.VFXCascade.AdditiveRowsStackOnBase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadeAdditiveLayeringTest::RunTest(const FString& Parameters)
{
	const FGameplayTag FireTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Fire"));
	const FGameplayTag PoisonTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Poison"));
	if (!FireTag.IsValid() || !PoisonTag.IsValid())
	{
		AddError(TEXT("Buff.Fire / Buff.Poison must be registered project tags"));
		return false;
	}

	TArray<FCascadeTestRow> Rows;
	Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, /*Priority=*/20, /*bIsAdditive=*/false, /*Payload=*/1));
	Rows.Add(CombatVFXCascadeTests_MakeRow(PoisonTag, /*Priority=*/5, /*bIsAdditive=*/true, /*Payload=*/2));
	Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, /*Priority=*/9, /*bIsAdditive=*/true, /*Payload=*/3));

	FGameplayTagContainer Context;
	Context.AddTag(FireTag);
	Context.AddTag(PoisonTag);

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, Context);

	TestEqual(TEXT("Non-additive fire row is the base"), Result.BaseIndex, 0);
	TestEqual(TEXT("Both additive rows are returned"), Result.AdditiveIndices.Num(), 2);
	if (Result.AdditiveIndices.Num() == 2)
	{
		TestEqual(TEXT("Additive layers are ordered highest priority first"), Result.AdditiveIndices[0], 2);
		TestEqual(TEXT("Lower priority additive comes second"), Result.AdditiveIndices[1], 1);
	}
	TestEqual(TEXT("Nothing overflowed under the default cap"), Result.AdditiveOverflowCount, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadeAdditiveOnlyTest,
	"DevKit.Combat.VFXCascade.AdditiveOnlyMatchStillResolves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadeAdditiveOnlyTest::RunTest(const FString& Parameters)
{
	const FGameplayTag PoisonTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Poison"));
	if (!PoisonTag.IsValid())
	{
		AddError(TEXT("Buff.Poison must be a registered project tag"));
		return false;
	}

	TArray<FCascadeTestRow> Rows;
	Rows.Add(CombatVFXCascadeTests_MakeRow(PoisonTag, 5, /*bIsAdditive=*/true, 1));

	FGameplayTagContainer Context;
	Context.AddTag(PoisonTag);

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, Context);

	TestFalse(TEXT("No base row exists to select"), Result.HasBase());
	TestFalse(TEXT("Result is not considered empty"), Result.IsEmpty());
	TestEqual(TEXT("Additive layer is still returned without a base"), Result.AdditiveIndices.Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadeUniversalRowTest,
	"DevKit.Combat.VFXCascade.EmptyMatchTagsActAsUniversalDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadeUniversalRowTest::RunTest(const FString& Parameters)
{
	TArray<FCascadeTestRow> Rows;
	Rows.Add(CombatVFXCascadeTests_MakeRow(FGameplayTag(), /*Priority=*/0, /*bIsAdditive=*/false, /*Payload=*/1));

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, FGameplayTagContainer());

	TestEqual(TEXT("A row with no match tags matches an empty context"), Result.BaseIndex, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadePriorityTieTest,
	"DevKit.Combat.VFXCascade.PriorityTieKeepsAuthoringOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadePriorityTieTest::RunTest(const FString& Parameters)
{
	const FGameplayTag FireTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Fire"));
	if (!FireTag.IsValid())
	{
		AddError(TEXT("Buff.Fire must be a registered project tag"));
		return false;
	}

	TArray<FCascadeTestRow> Rows;
	Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, /*Priority=*/10, false, 1));
	Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, /*Priority=*/10, false, 2));

	FGameplayTagContainer Context;
	Context.AddTag(FireTag);

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, Context);

	TestEqual(TEXT("First authored row wins an equal-priority tie"), Result.BaseIndex, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCombatVFXCascadeAdditiveCapTest,
	"DevKit.Combat.VFXCascade.AdditiveCapReportsOverflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatVFXCascadeAdditiveCapTest::RunTest(const FString& Parameters)
{
	const FGameplayTag FireTag = CombatVFXCascadeTests_Tag(TEXT("Buff.Fire"));
	if (!FireTag.IsValid())
	{
		AddError(TEXT("Buff.Fire must be a registered project tag"));
		return false;
	}

	TArray<FCascadeTestRow> Rows;
	for (int32 i = 0; i < 5; ++i)
	{
		Rows.Add(CombatVFXCascadeTests_MakeRow(FireTag, /*Priority=*/i, /*bIsAdditive=*/true, i));
	}

	FGameplayTagContainer Context;
	Context.AddTag(FireTag);

	const FCombatVFXCascadeResult Result = CombatVFXCascadeTests_Resolve(Rows, Context, /*MaxAdditiveLayers=*/2);

	TestEqual(TEXT("Additive layers are truncated to the cap"), Result.AdditiveIndices.Num(), 2);
	TestEqual(TEXT("Dropped layers are reported"), Result.AdditiveOverflowCount, 3);
	if (Result.AdditiveIndices.Num() == 2)
	{
		TestEqual(TEXT("Highest priority additive survives truncation"), Result.AdditiveIndices[0], 4);
		TestEqual(TEXT("Second highest priority additive survives truncation"), Result.AdditiveIndices[1], 3);
	}

	return true;
}

#endif
