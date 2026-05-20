#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#include "Story/StoryEngineSettings.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleSetDA.h"

namespace StoryEngineTests
{
FGameplayTag RequireTag(const TCHAR* TagName)
{
	return FGameplayTag::RequestGameplayTag(FName(TagName), false);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineGameplayTagsConfiguredTest,
	"DevKit.StoryEngine.GameplayTagsConfigured",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineGameplayTagsConfiguredTest::RunTest(const FString& Parameters)
{
	const TCHAR* RequiredTags[] = {
		TEXT("Level.Stage.WeaponPickup"),
		TEXT("Tutorial.WeaponPickup"),
		TEXT("Tutorial.CardConsume"),
		TEXT("Story.Event.MemoryTutorial.Started"),
		TEXT("Story.Event.MemoryTutorial.PlayerFailed"),
		TEXT("Story.Event.MemoryTutorial.Completed"),
		TEXT("Story.Event.FirstRun.Started"),
		TEXT("Story.Event.FirstRun.FirstRuneObtained"),
		TEXT("Story.Event.FirstRun.FirstBackpackOpened"),
		TEXT("Story.Event.Hub.FirstEntered"),
		TEXT("Story.Flag.MemoryTutorial.Completed"),
		TEXT("Story.Flag.FirstRune.Obtained"),
		TEXT("Story.Flag.FirstBackpack.Opened"),
		TEXT("Story.Flag.Hub.FirstEntered"),
		TEXT("Story.Quest.Main"),
		TEXT("Story.Quest.MemoryTutorial"),
		TEXT("Story.Source.Codex"),
		TEXT("Story.Source.System"),
	};

	bool bAllValid = true;
	for (const TCHAR* TagName : RequiredTags)
	{
		const FGameplayTag Tag = StoryEngineTests::RequireTag(TagName);
		bAllValid &= TestTrue(FString::Printf(TEXT("%s tag is configured"), TagName), Tag.IsValid());
	}

	return bAllValid;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineConfiguredRuleSetsLoadTest,
	"DevKit.StoryEngine.ConfiguredRuleSetsLoad",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineConfiguredRuleSetsLoadTest::RunTest(const FString& Parameters)
{
	const UStoryEngineSettings* Settings = GetDefault<UStoryEngineSettings>();
	if (!TestNotNull(TEXT("Story engine settings exist"), Settings))
	{
		return false;
	}

	TestEqual(TEXT("Three onboarding rule sets are configured"), Settings->RuleSets.Num(), 3);
	if (Settings->RuleSets.Num() != 3)
	{
		return false;
	}

	const FString ExpectedPaths[] = {
		TEXT("/Game/Story/Rules/SR_MemoryTutorial.SR_MemoryTutorial"),
		TEXT("/Game/Story/Rules/SR_FirstRun.SR_FirstRun"),
		TEXT("/Game/Story/Rules/SR_HubOnboarding.SR_HubOnboarding"),
	};

	bool bAllLoaded = true;
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(ExpectedPaths); ++Index)
	{
		TestEqual(
			FString::Printf(TEXT("Configured rule set path %d"), Index),
			Settings->RuleSets[Index].ToSoftObjectPath().ToString(),
			ExpectedPaths[Index]);

		UStoryRuleSetDA* RuleSet = Settings->RuleSets[Index].LoadSynchronous();
		bAllLoaded &= TestNotNull(FString::Printf(TEXT("Rule set %s loads"), *ExpectedPaths[Index]), RuleSet);
		if (RuleSet)
		{
			bAllLoaded &= TestTrue(FString::Printf(TEXT("%s contains rules"), *RuleSet->GetName()), RuleSet->Rules.Num() > 0);
		}
	}

	return bAllLoaded;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryRuleSetFiltersAndSortsRulesTest,
	"DevKit.StoryEngine.RuleSetFiltersAndSortsRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryRuleSetFiltersAndSortsRulesTest::RunTest(const FString& Parameters)
{
	const FGameplayTag EventTag = StoryEngineTests::RequireTag(TEXT("State.Combat"));
	const FGameplayTag OtherTag = StoryEngineTests::RequireTag(TEXT("State.Dialogue"));
	if (!TestTrue(TEXT("State.Combat tag is configured"), EventTag.IsValid()) ||
		!TestTrue(TEXT("State.Dialogue tag is configured"), OtherTag.IsValid()))
	{
		return false;
	}

	UStoryRuleSetDA* RuleSet = NewObject<UStoryRuleSetDA>();
	TestNotNull(TEXT("RuleSet exists"), RuleSet);

	FStoryRule LowPriorityRule;
	LowPriorityRule.RuleId = TEXT("LowPriority");
	LowPriorityRule.TriggerEventTag = EventTag;
	LowPriorityRule.Priority = 10;

	FStoryRule HighPriorityRule;
	HighPriorityRule.RuleId = TEXT("HighPriority");
	HighPriorityRule.TriggerEventTag = EventTag;
	HighPriorityRule.Priority = 100;

	FStoryRule OtherEventRule;
	OtherEventRule.RuleId = TEXT("OtherEvent");
	OtherEventRule.TriggerEventTag = OtherTag;
	OtherEventRule.Priority = 200;

	FStoryRule DisabledRule;
	DisabledRule.RuleId = TEXT("Disabled");
	DisabledRule.TriggerEventTag = EventTag;
	DisabledRule.Priority = 300;
	DisabledRule.bEnabled = false;

	RuleSet->Rules = { LowPriorityRule, HighPriorityRule, OtherEventRule, DisabledRule };

	TArray<const FStoryRule*> MatchingRules;
	RuleSet->GetRulesForEventSorted(EventTag, MatchingRules);

	TestEqual(TEXT("Only enabled rules for the requested event are returned"), MatchingRules.Num(), 2);
	if (MatchingRules.Num() != 2)
	{
		return false;
	}

	TestEqual(TEXT("Highest priority rule executes first"), MatchingRules[0]->RuleId, FName(TEXT("HighPriority")));
	TestEqual(TEXT("Lower priority rule executes second"), MatchingRules[1]->RuleId, FName(TEXT("LowPriority")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryRuleSetDisabledSetReturnsNoRulesTest,
	"DevKit.StoryEngine.DisabledRuleSetReturnsNoRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryRuleSetDisabledSetReturnsNoRulesTest::RunTest(const FString& Parameters)
{
	const FGameplayTag EventTag = StoryEngineTests::RequireTag(TEXT("State.Combat"));
	if (!TestTrue(TEXT("State.Combat tag is configured"), EventTag.IsValid()))
	{
		return false;
	}

	UStoryRuleSetDA* RuleSet = NewObject<UStoryRuleSetDA>();
	RuleSet->bEnabled = false;

	FStoryRule Rule;
	Rule.RuleId = TEXT("DisabledSetRule");
	Rule.TriggerEventTag = EventTag;
	RuleSet->Rules.Add(Rule);

	TArray<const FStoryRule*> MatchingRules;
	RuleSet->GetRulesForEventSorted(EventTag, MatchingRules);

	TestEqual(TEXT("Disabled rule set contributes no rules"), MatchingRules.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineRunAndSessionFlagsResetIndependentlyTest,
	"DevKit.StoryEngine.RunAndSessionFlagsResetIndependently",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineRunAndSessionFlagsResetIndependentlyTest::RunTest(const FString& Parameters)
{
	const FGameplayTag RunFlag = StoryEngineTests::RequireTag(TEXT("State.Combat"));
	const FGameplayTag SessionFlag = StoryEngineTests::RequireTag(TEXT("State.Dialogue"));
	if (!TestTrue(TEXT("State.Combat tag is configured"), RunFlag.IsValid()) ||
		!TestTrue(TEXT("State.Dialogue tag is configured"), SessionFlag.IsValid()))
	{
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEngineSubsystem* Engine = NewObject<UStoryEngineSubsystem>(GameInstance);
	TestNotNull(TEXT("Story engine exists"), Engine);

	Engine->SetStoryFlag(RunFlag, EStoryFlagScope::Run, true);
	Engine->SetStoryFlag(SessionFlag, EStoryFlagScope::Session, true);

	TestTrue(TEXT("Run flag is visible before reset"), Engine->HasStoryFlag(RunFlag, EStoryFlagScope::Run));
	TestTrue(TEXT("Session flag is visible before reset"), Engine->HasStoryFlag(SessionFlag, EStoryFlagScope::Session));

	Engine->ResetRunState();

	TestFalse(TEXT("Run flag is cleared by run reset"), Engine->HasStoryFlag(RunFlag, EStoryFlagScope::Run));
	TestTrue(TEXT("Session flag survives run reset"), Engine->HasStoryFlag(SessionFlag, EStoryFlagScope::Session));

	Engine->ResetSessionState();

	TestFalse(TEXT("Session flag is cleared by session reset"), Engine->HasStoryFlag(SessionFlag, EStoryFlagScope::Session));

	return true;
}

#endif
