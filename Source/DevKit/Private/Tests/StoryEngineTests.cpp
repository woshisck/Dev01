#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/GameInstance.h"
#include "Misc/AutomationTest.h"

#include "Story/StoryEngineSettings.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryEventRegistryDA.h"
#include "Story/StoryEventTypes.h"
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
		TEXT("Story.Event.FirstRun.FirstRewardCardEntered"),
		TEXT("Story.Event.FirstRun.FirstBackpackOpened"),
		TEXT("Story.Event.Hub.FirstEntered"),
		TEXT("Story.Event.Player.Died"),
		TEXT("Story.Event.Item.Obtained"),
		TEXT("Story.Event.Area.Entered"),
		TEXT("Story.Flag.MemoryTutorial.Completed"),
		TEXT("Story.Flag.FirstRun.Started"),
		TEXT("Story.Flag.FirstRune.Obtained"),
		TEXT("Story.Flag.FirstBackpack.Opened"),
		TEXT("Story.Flag.Hub.FirstEntered"),
		TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.heavy_card_obtained"),
		TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.weapon_skill_finisher_obtained"),
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEventRegistryRewardToDeckBroadcastOnlyTest,
	"DevKit.StoryEngine.RegistryRewardToDeckBroadcastOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEventRegistryRewardToDeckBroadcastOnlyTest::RunTest(const FString& Parameters)
{
	const UStoryEventRegistryDA* Registry = LoadObject<UStoryEventRegistryDA>(
		nullptr,
		TEXT("/Game/Data/Story/DA_StoryEventRegistry_Tutorial.DA_StoryEventRegistry_Tutorial"));
	TestNotNull(TEXT("Tutorial story event registry loads"), Registry);
	if (!Registry)
	{
		return false;
	}

	const FGameplayTag RewardToDeckEvent = StoryEngineTests::RequireTag(TEXT("Tutorial.RewardToDeck"));
	if (!TestTrue(TEXT("RewardToDeck event tag is configured"), RewardToDeckEvent.IsValid()))
	{
		return false;
	}

	const FStoryEventEntry* Entry = Registry->FindEntry(RewardToDeckEvent);
	TestNotNull(TEXT("RewardToDeck registry entry exists"), Entry);
	if (!Entry)
	{
		return false;
	}

	TestEqual(TEXT("RewardToDeck registry entry is broadcast-only"), Entry->ActionType, EStoryEventActionType::BroadcastOnly);
	TestTrue(TEXT("RewardToDeck registry entry no longer dispatches a tutorial ID"), Entry->TutorialEventID.IsNone());
	TestFalse(TEXT("RewardToDeck registry entry no longer pauses the game"), Entry->bPauseGame);

	const UStoryRuleSetDA* RuleSet = LoadObject<UStoryRuleSetDA>(
		nullptr,
		TEXT("/Game/Story/Rules/SR_FirstRun.SR_FirstRun"));
	TestNotNull(TEXT("First-run rule set loads"), RuleSet);
	if (!RuleSet)
	{
		return false;
	}

	const bool bHasRewardToDeckRule = RuleSet->Rules.ContainsByPredicate(
		[RewardToDeckEvent](const FStoryRule& Rule)
		{
			return Rule.TriggerEventTag == RewardToDeckEvent;
		});
	TestFalse(TEXT("RewardToDeck broadcast is not consumed by first-run story rules"), bHasRewardToDeckRule);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineFirstRuneRuleMarksWeaponSkillFinisherProgressTest,
	"DevKit.StoryEngine.FirstRuneRuleMarksWeaponSkillFinisherProgress",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineFirstRuneRuleMarksWeaponSkillFinisherProgressTest::RunTest(const FString& Parameters)
{
	const UStoryRuleSetDA* RuleSet = LoadObject<UStoryRuleSetDA>(
		nullptr,
		TEXT("/Game/Story/Rules/SR_FirstRun.SR_FirstRun"));
	TestNotNull(TEXT("First-run rule set loads"), RuleSet);
	if (!RuleSet)
	{
		return false;
	}

	const FGameplayTag FirstRuneEvent = StoryEngineTests::RequireTag(TEXT("Story.Event.FirstRun.FirstRuneObtained"));
	const FGameplayTag LegacyHeavyCardProgress = StoryEngineTests::RequireTag(
		TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.heavy_card_obtained"));
	const FGameplayTag WeaponSkillFinisherProgress = StoryEngineTests::RequireTag(
		TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.weapon_skill_finisher_obtained"));
	if (!TestTrue(TEXT("First rune event tag is configured"), FirstRuneEvent.IsValid()) ||
		!TestTrue(TEXT("legacy heavy card progress tag is configured"), LegacyHeavyCardProgress.IsValid()) ||
		!TestTrue(TEXT("WeaponSkill finisher progress tag is configured"), WeaponSkillFinisherProgress.IsValid()))
	{
		return false;
	}

	bool bMarksWeaponSkillFinisherProgress = false;
	for (const FStoryRule& Rule : RuleSet->Rules)
	{
		if (Rule.TriggerEventTag != FirstRuneEvent)
		{
			continue;
		}

		bMarksWeaponSkillFinisherProgress |= Rule.Actions.ContainsByPredicate(
			[LegacyHeavyCardProgress, WeaponSkillFinisherProgress](const FStoryAction& Action)
			{
				return Action.Type == EStoryActionType::SetFlag
					&& Action.FlagScope == EStoryFlagScope::Save
					&& Action.FlagTag == WeaponSkillFinisherProgress
					&& Action.FlagTag != LegacyHeavyCardProgress;
			});
	}

	TestTrue(TEXT("First rune rule marks first-run WeaponSkill finisher progress"), bMarksWeaponSkillFinisherProgress);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineFirstRewardCardEnteredShowsTutorialTest,
	"DevKit.StoryEngine.FirstRewardCardEnteredShowsTutorial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineFirstRewardCardEnteredShowsTutorialTest::RunTest(const FString& Parameters)
{
	const UStoryRuleSetDA* RuleSet = LoadObject<UStoryRuleSetDA>(
		nullptr,
		TEXT("/Game/Story/Rules/SR_FirstRun.SR_FirstRun"));
	TestNotNull(TEXT("First-run rule set loads"), RuleSet);
	if (!RuleSet)
	{
		return false;
	}

	const FGameplayTag RewardCardEnteredEvent = StoryEngineTests::RequireTag(TEXT("Story.Event.FirstRun.FirstRewardCardEntered"));
	const FGameplayTag FirstRuneFlag = StoryEngineTests::RequireTag(TEXT("Story.Flag.FirstRune.Obtained"));
	if (!TestTrue(TEXT("First reward card entered event tag is configured"), RewardCardEnteredEvent.IsValid()) ||
		!TestTrue(TEXT("First rune obtained flag is configured"), FirstRuneFlag.IsValid()))
	{
		return false;
	}

	const FStoryRule* MatchingRule = nullptr;
	for (const FStoryRule& Rule : RuleSet->Rules)
	{
		if (Rule.TriggerEventTag == RewardCardEnteredEvent)
		{
			MatchingRule = &Rule;
			break;
		}
	}

	TestNotNull(TEXT("First reward card entered rule exists"), MatchingRule);
	if (!MatchingRule)
	{
		return false;
	}

	TestEqual(TEXT("First reward card entered rule fires once per save"), MatchingRule->FirePolicy, EStoryRuleFirePolicy::OncePerSave);
	TestTrue(TEXT("First reward card entered rule is gated by first-rune save flag"),
		MatchingRule->Conditions.ContainsByPredicate(
			[FirstRuneFlag](const FStoryCondition& Condition)
			{
				return Condition.Type == EStoryConditionType::HasFlag
					&& Condition.bInvert
					&& Condition.FlagScope == EStoryFlagScope::Save
					&& Condition.FlagTag == FirstRuneFlag;
			}));
	TestTrue(TEXT("First reward card entered rule shows tutorial_first_rune"),
		MatchingRule->Actions.ContainsByPredicate(
			[](const FStoryAction& Action)
			{
				return Action.Type == EStoryActionType::ShowTutorialPopup
					&& Action.TutorialEventId == FName(TEXT("tutorial_first_rune"));
			}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineMoonlightRuleShowsTutorialTest,
	"DevKit.StoryEngine.MoonlightRuleShowsTutorial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineFirstBackpackOpenedShowsTutorialPopupTest,
	"DevKit.StoryEngine.FirstBackpackOpenedShowsTutorialPopup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineFirstBackpackOpenedShowsTutorialPopupTest::RunTest(const FString& Parameters)
{
	const UStoryRuleSetDA* RuleSet = LoadObject<UStoryRuleSetDA>(
		nullptr,
		TEXT("/Game/Story/Rules/SR_FirstRun.SR_FirstRun"));
	TestNotNull(TEXT("First-run rule set loads"), RuleSet);
	if (!RuleSet)
	{
		return false;
	}

	const FGameplayTag FirstBackpackEvent = StoryEngineTests::RequireTag(TEXT("Story.Event.FirstRun.FirstBackpackOpened"));
	if (!TestTrue(TEXT("First backpack event tag is configured"), FirstBackpackEvent.IsValid()))
	{
		return false;
	}

	bool bHasBackpackPopup = false;
	for (const FStoryRule& Rule : RuleSet->Rules)
	{
		if (Rule.TriggerEventTag != FirstBackpackEvent)
		{
			continue;
		}

		bHasBackpackPopup |= Rule.Actions.ContainsByPredicate(
			[](const FStoryAction& Action)
			{
				return Action.Type == EStoryActionType::ShowTutorialPopup
					&& Action.TutorialEventId == FName(TEXT("tutorial_backpack"));
			});
	}

	TestTrue(TEXT("First backpack opened rule must show tutorial_backpack popup"), bHasBackpackPopup);
	return true;
}

bool FStoryEngineMoonlightRuleShowsTutorialTest::RunTest(const FString& Parameters)
{
	const UStoryRuleSetDA* RuleSet = LoadObject<UStoryRuleSetDA>(
		nullptr,
		TEXT("/Game/Story/Rules/SR_FirstRun.SR_FirstRun"));
	TestNotNull(TEXT("First-run rule set loads"), RuleSet);
	if (!RuleSet)
	{
		return false;
	}

	const FGameplayTag MoonlightEvent = StoryEngineTests::RequireTag(TEXT("Story.Event.FirstRun.MoonlightObtained"));
	const FGameplayTag MoonlightProgress = StoryEngineTests::RequireTag(
		TEXT("Story.Encounter.Progress.EM_FirstRun_Tutorial.first_run.moonlight_obtained"));
	if (!TestTrue(TEXT("Moonlight event tag is configured"), MoonlightEvent.IsValid()) ||
		!TestTrue(TEXT("Moonlight progress tag is configured"), MoonlightProgress.IsValid()))
	{
		return false;
	}

	bool bMarksMoonlightProgress = false;
	bool bShowsMoonlightTutorial = false;
	for (const FStoryRule& Rule : RuleSet->Rules)
	{
		if (Rule.TriggerEventTag != MoonlightEvent)
		{
			continue;
		}

		bMarksMoonlightProgress |= Rule.Actions.ContainsByPredicate(
			[MoonlightProgress](const FStoryAction& Action)
			{
				return Action.Type == EStoryActionType::SetFlag
					&& Action.FlagScope == EStoryFlagScope::Save
					&& Action.FlagTag == MoonlightProgress;
			});
		bShowsMoonlightTutorial |= Rule.Actions.ContainsByPredicate(
			[](const FStoryAction& Action)
			{
				return Action.Type == EStoryActionType::ShowTutorialPopup
					&& Action.TutorialEventId == FName(TEXT("tutorial_card_link_moonlight"));
			});
	}

	TestTrue(TEXT("Moonlight rule marks first-run moonlight progress"), bMarksMoonlightProgress);
	TestTrue(TEXT("Moonlight rule shows the moonlight link-card tutorial"), bShowsMoonlightTutorial);
	return true;
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEngineQuestTaskStateQueriesTest,
	"DevKit.StoryEngine.QuestTaskStateQueries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEngineQuestTaskStateQueriesTest::RunTest(const FString& Parameters)
{
	const FGameplayTag MainQuest = StoryEngineTests::RequireTag(TEXT("Story.Quest.Main"));
	const FGameplayTag MemoryQuest = StoryEngineTests::RequireTag(TEXT("Story.Quest.MemoryTutorial"));
	const FGameplayTag CodexSource = StoryEngineTests::RequireTag(TEXT("Story.Source.Codex"));
	if (!TestTrue(TEXT("Story.Quest.Main tag is configured"), MainQuest.IsValid()) ||
		!TestTrue(TEXT("Story.Quest.MemoryTutorial tag is configured"), MemoryQuest.IsValid()) ||
		!TestTrue(TEXT("Story.Source.Codex tag is configured"), CodexSource.IsValid()))
	{
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEngineSubsystem* Engine = NewObject<UStoryEngineSubsystem>(GameInstance);
	TestNotNull(TEXT("Story engine exists"), Engine);

	Engine->SetQuestTask(MainQuest, FText::FromString(TEXT("Find the first rune")), CodexSource, FGameplayTag());
	Engine->SetQuestTask(MemoryQuest, FText::FromString(TEXT("Recover the memory fragment")), CodexSource, FGameplayTag());
	Engine->SetQuestTaskState(MemoryQuest, EStoryQuestTaskState::Completed);

	TArray<FStoryQuestTaskData> ActiveTasks = Engine->GetQuestTasksByState(EStoryQuestTaskState::Active);
	TArray<FStoryQuestTaskData> CompletedTasks = Engine->GetQuestTasksByState(EStoryQuestTaskState::Completed);

	TestEqual(TEXT("Only one quest remains active"), ActiveTasks.Num(), 1);
	if (ActiveTasks.Num() == 1)
	{
		TestEqual(TEXT("Main quest remains active"), ActiveTasks[0].TaskId, MainQuest);
	}

	TestEqual(TEXT("Only one quest is completed"), CompletedTasks.Num(), 1);
	if (CompletedTasks.Num() == 1)
	{
		TestEqual(TEXT("Memory quest is completed"), CompletedTasks[0].TaskId, MemoryQuest);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStoryEnginePublicActionConditionApisTest,
	"DevKit.StoryEngine.PublicActionConditionApis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStoryEnginePublicActionConditionApisTest::RunTest(const FString& Parameters)
{
	const FGameplayTag FlagTag = StoryEngineTests::RequireTag(TEXT("State.Combat"));
	const FGameplayTag EventTag = StoryEngineTests::RequireTag(TEXT("Story.Event.Area.Entered"));
	if (!TestTrue(TEXT("State.Combat tag is configured"), FlagTag.IsValid()) ||
		!TestTrue(TEXT("Story.Event.Area.Entered tag is configured"), EventTag.IsValid()))
	{
		return false;
	}

	UGameInstance* GameInstance = NewObject<UGameInstance>();
	UStoryEngineSubsystem* Engine = NewObject<UStoryEngineSubsystem>(GameInstance);
	TestNotNull(TEXT("Story engine exists"), Engine);

	FStoryEventContext Context = FStoryEventContext::Make(EventTag);

	FStoryAction SetFlagAction;
	SetFlagAction.Type = EStoryActionType::SetFlag;
	SetFlagAction.FlagScope = EStoryFlagScope::Run;
	SetFlagAction.FlagTag = FlagTag;

	Engine->ExecuteStoryAction(SetFlagAction, Context);

	FStoryCondition HasFlagCondition;
	HasFlagCondition.Type = EStoryConditionType::HasFlag;
	HasFlagCondition.FlagScope = EStoryFlagScope::Run;
	HasFlagCondition.FlagTag = FlagTag;

	TestTrue(TEXT("ExecuteStoryAction applies a SetFlag action"), Engine->HasStoryFlag(FlagTag, EStoryFlagScope::Run));
	TestTrue(TEXT("EvaluateStoryCondition uses the same condition semantics as rules"),
		Engine->EvaluateStoryCondition(HasFlagCondition, Context));

	HasFlagCondition.bInvert = true;
	TestFalse(TEXT("EvaluateStoryCondition honors inverted conditions"),
		Engine->EvaluateStoryCondition(HasFlagCondition, Context));

	return true;
}

#endif
