#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/StateConflictDataAsset.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/StrongObjectPtr.h"

// This accessor exercises the synchronous reentrant boundary without modifying a
// production ability CDO, starting the game instance, or loading any save/assets.
struct FLegacyStateConflictTestAccessor
{
	static void HoldDrain(UYogAbilitySystemComponent& ASC, TFunctionRef<void()> Callback)
	{
		TGuardValue<bool> Guard(ASC.bProcessingConflict, true);
		Callback();
	}
	static void Drain(UYogAbilitySystemComponent& ASC)
	{
		ASC.ProcessStateConflict(FGameplayTag(), false);
	}
	static void RepeatEvent(UYogAbilitySystemComponent& ASC, FGameplayTag Tag, bool bExists)
	{
		ASC.ProcessStateConflict(Tag, bExists);
	}
	static int32 PendingCount(const UYogAbilitySystemComponent& ASC)
	{
		return ASC.PendingLegacyStateConflictEvents.Num();
	}
	static int32 ContributionCount(const UYogAbilitySystemComponent& ASC)
	{
		return ASC.LegacyStateConflictBlockContributions.Num();
	}
	static bool HasCategoryState(const UYogAbilitySystemComponent& ASC, FGameplayTag Category, FGameplayTag State)
	{
		const FGameplayTagContainer* States = ASC.BlockCategoryMap.Find(Category);
		return States && States->HasTagExact(State);
	}
};

namespace LegacyStateConflictTests
{
	FGameplayTag StateA() { return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack")); }
	FGameplayTag StateB() { return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill")); }
	FGameplayTag TargetA() { return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash")); }
	FGameplayTag TargetB() { return FGameplayTag::RequestGameplayTag(TEXT("Character.State.Equipment.SwitchWeapon")); }
	FGameplayTagContainer Tags(FGameplayTag Tag) { return FGameplayTagContainer(Tag); }

	struct FFixture
	{
		TStrongObjectPtr<UYogAbilitySystemComponent> ASC{NewObject<UYogAbilitySystemComponent>()};
		TStrongObjectPtr<UStateConflictDataAsset> Table{NewObject<UStateConflictDataAsset>()};
		FStateConflictRule& AddRule(FGameplayTag State, FGameplayTag Target)
		{
			FStateConflictRule& Rule = Table->Rules.AddDefaulted_GetRef();
			Rule.ActiveTag = State;
			Rule.BlockTags.AddTag(Target);
			return Rule;
		}
		void Install() { ASC->SetConflictTable(Table.Get()); }
	};
}

namespace LegacyStateConflictTests
{

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictContributionOwnershipTest,
	"DevKit.StateConflict.Lifecycle.ContributionsPreserveOtherOwners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictContributionOwnershipTest::RunTest(const FString& Parameters)
{
	FFixture F;
	F.AddRule(StateA(), TargetA());
	F.AddRule(StateB(), TargetA());
	F.Install();
	F.ASC->BlockAbilitiesWithTags(Tags(TargetA())); // A contribution outside StateConflict.
	F.ASC->AddLooseGameplayTag(StateA());
	F.ASC->AddLooseGameplayTag(StateB());
	F.ASC->RemoveLooseGameplayTag(StateA());
	TestTrue(TEXT("Other active state still blocks"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	F.ASC->RemoveLooseGameplayTag(StateB());
	TestTrue(TEXT("Unrelated external block survives both releases"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	F.ASC->UnBlockAbilitiesWithTags(Tags(TargetA()));
	TestFalse(TEXT("Every contribution released exactly once"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestEqual(TEXT("Ledger empty"), FLegacyStateConflictTestAccessor::ContributionCount(*F.ASC), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictReloadSnapshotTest,
	"DevKit.StateConflict.Lifecycle.ReloadReplacesOnlyAppliedSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictReloadSnapshotTest::RunTest(const FString& Parameters)
{
	FFixture F;
	F.AddRule(StateA(), TargetA());
	F.Install();
	F.ASC->AddLooseGameplayTag(StateA());
	TStrongObjectPtr<UStateConflictDataAsset> Replacement(NewObject<UStateConflictDataAsset>());
	FStateConflictRule& Rule = Replacement->Rules.AddDefaulted_GetRef();
	Rule.ActiveTag = StateA();
	Rule.BlockTags.AddTag(TargetB());
	F.ASC->SetConflictTable(Replacement.Get());
	TestFalse(TEXT("Old snapshot released during reload"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestTrue(TEXT("Currently held state picks up new blocks"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	F.ASC->InitConflictTable();
	F.ASC->InitConflictTable();
	F.ASC->RemoveLooseGameplayTag(StateA());
	TestFalse(TEXT("Repeated reload does not accumulate contributions"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictActiveCategoryRemapTest,
	"DevKit.StateConflict.Lifecycle.ActiveCategoryRemapKeepsInstalledSnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictActiveCategoryRemapTest::RunTest(const FString& Parameters)
{
	AddExpectedError(TEXT("Rejected category remap"), EAutomationExpectedErrorFlags::Contains, 6);
	for (const TCHAR* CategoryName : { TEXT("Block.Movement"), TEXT("Block.AI") })
	{
		FFixture F; // No World, actor, movement component, AI brain or production asset load.
		const FGameplayTag Category = FGameplayTag::RequestGameplayTag(FName(CategoryName));
		F.AddRule(StateA(), TargetA());
		F.Table->BlockCategoryMap.Add(Category, Tags(StateA()));
		F.Install();
		F.ASC->AddLooseGameplayTag(StateA());
		TStrongObjectPtr<UStateConflictDataAsset> Candidate(NewObject<UStateConflictDataAsset>());
		Candidate->BlockCategoryMap.Add(Category, Tags(StateB()));
		F.ASC->SetConflictTable(Candidate.Get());
		TestTrue(TEXT("Rejected setter restores accepted table identity"), F.ASC->ConflictTable == F.Table.Get());
		TestTrue(TEXT("Rejected setter keeps old GAS block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
		TestTrue(TEXT("Rejected setter keeps old exit mapping"), FLegacyStateConflictTestAccessor::HasCategoryState(*F.ASC, Category, StateA()));
		TestFalse(TEXT("Rejected setter does not install new map"), FLegacyStateConflictTestAccessor::HasCategoryState(*F.ASC, Category, StateB()));

		F.ASC->ConflictTable = Candidate.Get();
		F.ASC->InitConflictTable();
		TestTrue(TEXT("Direct Init cannot bypass guard"), F.ASC->ConflictTable == F.Table.Get());

		F.Table->BlockCategoryMap[Category] = Tags(StateB());
		F.Table->Rules[0].BlockTags = Tags(TargetB());
		F.ASC->InitConflictTable();
		TestTrue(TEXT("In-place asset edit is compared to installed map"), FLegacyStateConflictTestAccessor::HasCategoryState(*F.ASC, Category, StateA()));
		TestTrue(TEXT("In-place rejection keeps actual old contribution"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
		TestFalse(TEXT("In-place rejection does not add candidate contribution"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
		F.ASC->RemoveLooseGameplayTag(StateA());
		TestFalse(TEXT("Original state exit still releases its old block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));

		F.ASC->InitConflictTable();
		TestTrue(TEXT("Inactive category remap is accepted"), FLegacyStateConflictTestAccessor::HasCategoryState(*F.ASC, Category, StateB()));
		TestFalse(TEXT("Accepted map no longer contains old state"), FLegacyStateConflictTestAccessor::HasCategoryState(*F.ASC, Category, StateA()));
		F.ASC->AddLooseGameplayTag(StateA());
		TestTrue(TEXT("Accepted inactive reload installs candidate rules"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
		F.ASC->RemoveLooseGameplayTag(StateA());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictNewCategoryStateGuardTest,
	"DevKit.StateConflict.Lifecycle.NewActiveCategoryStateRejectedButSameMapAllowed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictNewCategoryStateGuardTest::RunTest(const FString& Parameters)
{
	FFixture F;
	const FGameplayTag Category = FGameplayTag::RequestGameplayTag(TEXT("Block.Movement"));
	F.AddRule(StateA(), TargetA());
	F.Install();
	F.ASC->AddLooseGameplayTag(StateA());
	TStrongObjectPtr<UStateConflictDataAsset> Candidate(NewObject<UStateConflictDataAsset>());
	Candidate->BlockCategoryMap.Add(Category, Tags(StateA()));
	AddExpectedError(TEXT("Rejected category remap"), EAutomationExpectedErrorFlags::Contains, 1);
	F.ASC->SetConflictTable(Candidate.Get());
	TestTrue(TEXT("Newly mapped active state is also guarded"), F.ASC->ConflictTable == F.Table.Get());
	TestTrue(TEXT("Rejected new mapping preserves old block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestFalse(TEXT("New mapping not silently enabled"), FLegacyStateConflictTestAccessor::HasCategoryState(*F.ASC, Category, StateA()));
	F.ASC->RemoveLooseGameplayTag(StateA());
	F.ASC->SetConflictTable(Candidate.Get());
	F.ASC->AddLooseGameplayTag(StateA());
	TStrongObjectPtr<UStateConflictDataAsset> SameCategories(NewObject<UStateConflictDataAsset>());
	SameCategories->BlockCategoryMap = Candidate->BlockCategoryMap;
	FStateConflictRule& Rule = SameCategories->Rules.AddDefaulted_GetRef();
	Rule.ActiveTag = StateA(); Rule.BlockTags = Tags(TargetB());
	F.ASC->SetConflictTable(SameCategories.Get());
	TestTrue(TEXT("Unchanged category map can hot reload ordinary rules"), F.ASC->ConflictTable == SameCategories.Get());
	TestTrue(TEXT("Ordinary rule reload still takes effect"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	F.ASC->RemoveLooseGameplayTag(StateA());
	TestFalse(TEXT("Ordinary reload remains balanced"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictReleaseUsesAppliedTagsTest,
	"DevKit.StateConflict.Lifecycle.ReleaseUsesActualAppliedTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictReleaseUsesAppliedTagsTest::RunTest(const FString& Parameters)
{
	FFixture F;
	F.AddRule(StateA(), TargetA());
	F.Install();
	F.ASC->AddLooseGameplayTag(StateA());
	FLegacyStateConflictTestAccessor::RepeatEvent(*F.ASC, StateA(), true);
	F.Table->Rules[0].BlockTags = Tags(TargetB());
	F.ASC->RemoveLooseGameplayTag(StateA());
	FLegacyStateConflictTestAccessor::RepeatEvent(*F.ASC, StateA(), false);
	TestFalse(TEXT("Duplicate enter/exit leaves no original block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestFalse(TEXT("Unapplied edited target remains untouched"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictReentrantQueueTest,
	"DevKit.StateConflict.Lifecycle.ReentrantQueuePreservesTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictReentrantQueueTest::RunTest(const FString& Parameters)
{
	FFixture F;
	F.AddRule(StateA(), TargetA());
	F.AddRule(StateB(), TargetB());
	F.Install();
	F.ASC->AddLooseGameplayTag(StateA());
	FLegacyStateConflictTestAccessor::HoldDrain(*F.ASC, [&]()
	{
		F.ASC->RemoveLooseGameplayTag(StateA());
		F.ASC->AddLooseGameplayTag(StateB());
		F.ASC->RemoveLooseGameplayTag(StateB());
	});
	TestEqual(TEXT("All nested transitions queued, none dropped"), FLegacyStateConflictTestAccessor::PendingCount(*F.ASC), 3);
	FLegacyStateConflictTestAccessor::Drain(*F.ASC);
	TestFalse(TEXT("Nested removal releases pre-existing block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestFalse(TEXT("Queued enter followed by exit balances"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	TestEqual(TEXT("Drain completes"), FLegacyStateConflictTestAccessor::PendingCount(*F.ASC), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictReentrantReloadTest,
	"DevKit.StateConflict.Lifecycle.ReloadInvalidatesOldQueuedGeneration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictReentrantReloadTest::RunTest(const FString& Parameters)
{
	FFixture F;
	F.AddRule(StateA(), TargetA());
	F.Install();
	FLegacyStateConflictTestAccessor::HoldDrain(*F.ASC, [&]()
	{
		F.ASC->AddLooseGameplayTag(StateA());
		F.ASC->RemoveLooseGameplayTag(StateA());
		F.Table->Rules[0].BlockTags = Tags(TargetB());
		F.ASC->InitConflictTable();
		F.ASC->AddLooseGameplayTag(StateA());
	});
	FLegacyStateConflictTestAccessor::Drain(*F.ASC);
	TestFalse(TEXT("Old queued policy not resurrected"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestTrue(TEXT("Current generation enter applies new policy"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	F.ASC->RemoveLooseGameplayTag(StateA());
	TestFalse(TEXT("New generation release balances"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictInitialOwnedStateTest,
	"DevKit.StateConflict.Lifecycle.RebuildPreservesExactTriggerAndStackSemantics",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictInitialOwnedStateTest::RunTest(const FString& Parameters)
{
	FFixture F;
	F.AddRule(StateA(), TargetA());
	F.AddRule(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill")), TargetB());
	F.ASC->AddLooseGameplayTag(StateA(), 2);
	F.Install();
	TestTrue(TEXT("Existing explicit state is rebuilt"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestFalse(TEXT("Implicit parent does not invent a rule trigger"), F.ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	F.ASC->RemoveLooseGameplayTag(StateA());
	TestTrue(TEXT("Count 2 to 1 does not release state block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	F.ASC->RemoveLooseGameplayTag(StateA());
	TestFalse(TEXT("Count 1 to 0 releases state block"), F.ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStateConflictUnregisterTest,
	"DevKit.StateConflict.Lifecycle.UnregisterReclaimsOnlyOwnedContributions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStateConflictUnregisterTest::RunTest(const FString& Parameters)
{
	const UWorld::InitializationValues InitValues = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
		.CreateAISystem(false).ShouldSimulatePhysics(false).SetTransactional(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
		true, ERHIFeatureLevel::Num, &InitValues);
	if (!TestNotNull(TEXT("Isolated world"), World)) return false;
	AActor* Owner = World->SpawnActor<AActor>();
	if (!TestNotNull(TEXT("Isolated actor"), Owner))
	{
		World->DestroyWorld(false);
		return false;
	}
	UYogAbilitySystemComponent* ASC = NewObject<UYogAbilitySystemComponent>(Owner);
	Owner->AddInstanceComponent(ASC);
	ASC->RegisterComponent();
	ASC->InitAbilityActorInfo(Owner, Owner);
	TStrongObjectPtr<UStateConflictDataAsset> Table(NewObject<UStateConflictDataAsset>());
	FStateConflictRule& Rule = Table->Rules.AddDefaulted_GetRef();
	Rule.ActiveTag = StateA();
	Rule.BlockTags.AddTag(TargetA());
	ASC->SetConflictTable(Table.Get());
	ASC->BlockAbilitiesWithTags(Tags(TargetB()));
	ASC->AddLooseGameplayTag(StateA());
	ASC->UnregisterComponent();
	TestFalse(TEXT("Unregister releases own contribution"), ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	TestTrue(TEXT("Unregister does not clear unrelated block ledger"), ASC->AreAbilityTagsBlocked(Tags(TargetB())));
	TestEqual(TEXT("Own ledger empty after unregister"), FLegacyStateConflictTestAccessor::ContributionCount(*ASC), 0);
	FLegacyStateConflictTestAccessor::RepeatEvent(*ASC, StateA(), true);
	TestFalse(TEXT("Teardown callback cannot reacquire"), ASC->AreAbilityTagsBlocked(Tags(TargetA())));
	ASC->UnBlockAbilitiesWithTags(Tags(TargetB()));
	World->DestroyWorld(false);
	return true;
}
}

#endif
