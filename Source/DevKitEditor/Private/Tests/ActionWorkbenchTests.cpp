#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "EditorValidatorSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Tools/ActionWorkbench/DevKitActionWorkbenchModel.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace DevKitActionWorkbenchTests
{
	UStateConflictDataAsset* NewAsset()
	{
		UPackage* Package = CreatePackage(*FString::Printf(TEXT("/Temp/DevKitActionWorkbenchTests/%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
		return NewObject<UStateConflictDataAsset>(Package, NAME_None, RF_Transactional);
	}

	FGameplayTag Tag(const TCHAR* Name) { return FGameplayTag::RequestGameplayTag(FName(Name), false); }
	FGameplayTag Attack() { return Tag(TEXT("Character.State.Skill.Attack")); }
	FGameplayTag Dash() { return Tag(TEXT("Character.State.Movement.Dash")); }
	FGameplayTag Skill() { return Tag(TEXT("Character.State.Skill.WeaponSkill")); }

	FStateConflictRule& AddRule(UStateConflictDataAsset* Asset, FGameplayTag Active)
	{
		FStateConflictRule& Rule = Asset->Rules.AddDefaulted_GetRef();
		Rule.ActiveTag = Active;
		return Rule;
	}

	int32 ErrorCount(const TArray<FDevKitActionWorkbenchIssue>& Issues)
	{
		return Issues.FilterByPredicate([](const FDevKitActionWorkbenchIssue& Issue) { return Issue.bError; }).Num();
	}

	bool HasPlayContext()
	{
		if (!GEditor) return false;
		if (GEditor->PlayWorld || GEditor->bIsSimulatingInEditor) return true;
		for (const FWorldContext& Context : GEditor->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::PIE) return true;
		}
		return false;
	}

	/** No InitializeWorld, GameInstance, BeginPlay, map loading, or PIE lifecycle notifications. */
	struct FScopedPIEContext
	{
		TStrongObjectPtr<UWorld> World;
		explicit FScopedPIEContext(UEditorEngine& InEditor)
			: World(NewObject<UWorld>(GetTransientPackage(), NAME_None, RF_Transient)), Editor(InEditor)
		{
			World->WorldType = EWorldType::PIE;
			Editor.CreateNewWorldContext(EWorldType::PIE).SetCurrentWorld(World.Get());
		}
		~FScopedPIEContext() { Editor.DestroyWorldContext(World.Get()); }
		UEditorEngine& Editor;
	};

	/** One unique mount/package under Saved. Retained for asynchronous editor consumers and QA evidence. */
	struct FScopedDiskPackage
	{
		FString Root;
		FString Directory;
		FString Filename;
		TStrongObjectPtr<UStateConflictDataAsset> Asset;
		bool bCreatedDirectory = false;
		bool bMounted = false;
		bool bOwnsFile = false;

		bool Initialize()
		{
			const FString Id = FGuid::NewGuid().ToString(EGuidFormats::Digits);
			Root = TEXT("/DevKitActionWorkbenchTest_") + Id + TEXT("/");
			Directory = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation/ActionWorkbench"), Id)) + TEXT("/");
			Filename = Directory + TEXT("Rules.uasset");
			if (IFileManager::Get().DirectoryExists(*Directory) || IFileManager::Get().FileExists(*Filename)) return false;
			if (!IFileManager::Get().MakeDirectory(*Directory, true)) return false;
			bCreatedDirectory = true;
			FPackageName::RegisterMountPoint(Root, Directory);
			bMounted = true;
			UPackage* Package = CreatePackage(*(Root + TEXT("Rules")));
			Asset.Reset(NewObject<UStateConflictDataAsset>(Package, TEXT("Rules"), RF_Public | RF_Standalone | RF_Transactional));
			return Asset.IsValid();
		}

		bool Save()
		{
			if (!bMounted || !Asset.IsValid() || (!bOwnsFile && IFileManager::Get().FileExists(*Filename))) return false;
			// This fixture tests disk concurrency, not content/localization validators. Do not queue a
			// deferred whole-editor validation for a private test mount. The production save path is unchanged.
			const FScopedDisableValidateOnSave DisableFixtureContentValidation;
			FSavePackageArgs Args;
			Args.TopLevelFlags = RF_Public | RF_Standalone;
			Args.SaveFlags = SAVE_NoError;
			Args.bSlowTask = false;
			if (!UPackage::SavePackage(Asset->GetOutermost(), Asset.Get(), *Filename, Args)) return false;
			bOwnsFile = true;
			return true;
		}

		~FScopedDiskPackage()
		{
			if (Asset.IsValid())
			{
				Asset->GetOutermost()->SetDirtyFlag(false);
				Asset->ClearFlags(RF_Standalone);
			}
			if (bOwnsFile)
			{
				FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Filename, false);
			}
			// AssetSearch/DirectoryWatcher process saved packages asynchronously. Keep this GUID-only
			// evidence file and its mount until editor exit instead of racing those background readers.
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchDraftIsolationTest,
	"DevKit.ActionWorkbench.DraftIsolationAndDirection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchDraftIsolationTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewAsset());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	if (!TestTrue(TEXT("Project action tag dictionary is loaded"), Attack().IsValid() && Dash().IsValid())) return false;
	const FString Before = FDevKitActionWorkbenchModel::Snapshot(Source.Get());
	TestTrue(TEXT("Load creates detached draft"), Model.Load(Source.Get(), Message));
	TestTrue(TEXT("Separate object"), Model.GetDraft() != Source.Get());
	TestTrue(TEXT("Draft never becomes a content asset"), Model.GetDraft()->HasAnyFlags(RF_Transient));
	TestFalse(TEXT("Open has no edits"), Model.IsDirty());
	TestTrue(TEXT("Set block"), Model.SetBlocked(Attack(), Dash(), true, Message));
	TestEqual(TEXT("Block edits current state's row"), Model.GetDraft()->Rules[0].ActiveTag, Dash());
	TestTrue(TEXT("Current state blocks request identity"), Model.GetDraft()->Rules[0].BlockTags.HasTagExact(Attack()));
	TestTrue(TEXT("Set cancel"), Model.SetCancel(Attack(), Dash(), true, Message));
	TestEqual(TEXT("Cancel edits requesting state's row"), Model.GetDraft()->Rules[1].ActiveTag, Attack());
	TestTrue(TEXT("Request state cancels current identity"), Model.GetDraft()->Rules[1].CancelTags.HasTagExact(Dash()));
	const FDevKitActionWorkbenchCell Forward = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Attack(), Dash());
	const FDevKitActionWorkbenchCell Reverse = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Dash(), Attack());
	TestTrue(TEXT("Forward has block and post-activation cancel, not automatic override"), Forward.bBlocked && Forward.bCancels);
	TestFalse(TEXT("Reverse block is independent"), Reverse.bBlocked);
	TestFalse(TEXT("Reverse cancel is independent"), Reverse.bCancels);
	TestTrue(TEXT("Draft reports changes"), Model.IsDirty());
	TestEqual(TEXT("Source did not change"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), Before);
	TestFalse(TEXT("Opening/editing draft did not dirty source package"), Source->GetOutermost()->IsDirty());
	TestTrue(TEXT("Explicit reset discards draft"), Model.Reset(Message));
	TestFalse(TEXT("Reset draft is clean"), Model.IsDirty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchParentMatchTest,
	"DevKit.ActionWorkbench.ExactTriggersAndInheritedTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchParentMatchTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewAsset());
	const FGameplayTag SkillParent = Tag(TEXT("Character.State.Skill"));
	AddRule(Source.Get(), Dash()).BlockTags.AddTag(SkillParent);
	AddRule(Source.Get(), SkillParent).CancelTags.AddTag(Dash());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	TestTrue(TEXT("Load"), Model.Load(Source.Get(), Message));
	const FDevKitActionWorkbenchCell Cell = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Attack(), Dash());
	TestTrue(TEXT("Target child matches parent restriction"), Cell.bBlocked && Cell.bInheritedBlock);
	TestFalse(TEXT("ActiveTag parent is not an exact child trigger"), Cell.bHasRequestRule);
	TestFalse(TEXT("Parent active rule is not silently inherited"), Cell.bCancels);
	const FString Before = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	TestFalse(TEXT("Cannot remove shared parent via a child cell"), Model.SetBlocked(Attack(), Dash(), false, Message));
	TestEqual(TEXT("Refused edit keeps whole draft unchanged"), FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), Before);
	TestTrue(TEXT("Add explicit child without removing parent"), Model.SetBlocked(Attack(), Dash(), true, Message));
	const FString Both = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	TestFalse(TEXT("Exact child plus parent still cannot promise unblock"), Model.SetBlocked(Attack(), Dash(), false, Message));
	TestEqual(TEXT("Refused edit is atomic"), FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), Both);
	TestTrue(TEXT("Other sibling remains covered"), FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Skill(), Dash()).bBlocked);
	const FGameplayTag DashChild = Tag(TEXT("Character.State.Movement.Dash.Combo1"));
	const auto ChildCurrent = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Attack(), DashChild);
	TestFalse(TEXT("Current trigger is also exact, not inherited from dash parent"), ChildCurrent.bHasCurrentRule || ChildCurrent.bBlocked);
	AddRule(Model.GetDraft(), Attack()).CancelTags.AddTag(SkillParent);
	const auto CancelParent = FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Attack(), Skill());
	TestTrue(TEXT("Cancel target uses the same child to parent match"), CancelParent.bCancels && CancelParent.bInheritedCancel);
	const FString BeforeCancelRemoval = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	TestFalse(TEXT("Cannot remove shared parent cancellation using child cell"), Model.SetCancel(Attack(), Skill(), false, Message));
	TestEqual(TEXT("Inherited cancellation refusal is atomic"), FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), BeforeCancelRemoval);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchValidationTest,
	"DevKit.ActionWorkbench.ValidationFailsClosed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchValidationTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewAsset());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	TestTrue(TEXT("Load"), Model.Load(Source.Get(), Message));
	AddRule(Model.GetDraft(), Attack());
	AddRule(Model.GetDraft(), Attack()).Priority = 100;
	AddRule(Model.GetDraft(), FGameplayTag());
	TestTrue(TEXT("Duplicate and invalid ActiveTags are errors"), ErrorCount(Model.Validate()) >= 2);
	TestTrue(TEXT("Duplicate pair visibly ambiguous"), FDevKitActionWorkbenchModel::EvaluateCell(Model.GetDraft(), Attack(), Dash()).bAmbiguous);
	const FString Before = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	TestFalse(TEXT("Duplicate cannot be silently edited"), Model.SetCancel(Attack(), Dash(), true, Message));
	TestEqual(TEXT("Duplicate edit has no side effects"), FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), Before);
	TestFalse(TEXT("Invalid draft cannot apply"), Model.ApplyDraft(Message));
	TestEqual(TEXT("Failed apply preserves source"), Source->Rules.Num(), 0);
	TestFalse(TEXT("Failed apply preserves source dirty flag"), Source->GetOutermost()->IsDirty());
	TestTrue(TEXT("Null source fails closed"), ErrorCount(FDevKitActionWorkbenchModel::ValidateAsset(nullptr)) > 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchSourceConflictTest,
	"DevKit.ActionWorkbench.SourceConflictAndDirtyProtection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchSourceConflictTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewAsset());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	TestTrue(TEXT("Load"), Model.Load(Source.Get(), Message));
	Model.SetBlocked(Attack(), Dash(), true, Message);
	AddRule(Source.Get(), Skill()).BlockTags.AddTag(Dash()); // Another asset editor, without touching the draft.
	const FString ChangedSource = FDevKitActionWorkbenchModel::Snapshot(Source.Get());
	TestTrue(TEXT("Detect source edit"), Model.HasSourceChanged());
	TestFalse(TEXT("Cannot overwrite changed source"), Model.ApplyDraft(Message));
	TestEqual(TEXT("Other author's edit remains"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), ChangedSource);
	TestFalse(TEXT("Refused apply does not dirty package"), Source->GetOutermost()->IsDirty());
	TestTrue(TEXT("Explicit reload imports current in-memory source"), Model.Reset(Message));
	TestFalse(TEXT("Reload baseline has no stale data"), Model.HasSourceChanged());
	Source->GetOutermost()->SetDirtyFlag(true);
	TestTrue(TEXT("Existing dirty source may be inspected"), Model.Load(Source.Get(), Message));
	Model.SetBlocked(Attack(), Dash(), true, Message);
	TestFalse(TEXT("Pre-existing unsaved source cannot be overwritten"), Model.ApplyDraft(Message));
	TestEqual(TEXT("Dirty source remains unchanged"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), ChangedSource);
	Source->GetOutermost()->SetDirtyFlag(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchSnapshotTest,
	"DevKit.ActionWorkbench.DeterministicSnapshotAndCatalog",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchSnapshotTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	TStrongObjectPtr<UStateConflictDataAsset> A(NewAsset());
	TStrongObjectPtr<UStateConflictDataAsset> B(NewAsset());
	FStateConflictRule& RuleA = AddRule(A.Get(), Dash());
	RuleA.BlockTags.AddTag(Attack());
	RuleA.BlockTags.AddTag(Skill());
	FStateConflictRule& RuleB = AddRule(B.Get(), Dash());
	RuleB.BlockTags.AddTag(Skill());
	RuleB.BlockTags.AddTag(Attack());
	FGameplayTagContainer States;
	States.AddTag(Dash());
	const FGameplayTag Movement = Tag(TEXT("Block.Movement"));
	const FGameplayTag AI = Tag(TEXT("Block.AI"));
	A->BlockCategoryMap.Add(Movement, States);
	A->BlockCategoryMap.Add(AI, States);
	B->BlockCategoryMap.Add(AI, States);
	B->BlockCategoryMap.Add(Movement, States);
	TestEqual(TEXT("Map and target insertion order do not affect snapshot"), FDevKitActionWorkbenchModel::Snapshot(A.Get()), FDevKitActionWorkbenchModel::Snapshot(B.Get()));
	AddRule(A.Get(), Attack());
	AddRule(B.Get(), Attack());
	B->Rules.Swap(0, 1);
	TestNotEqual(TEXT("Rule order remains part of baseline"), FDevKitActionWorkbenchModel::Snapshot(A.Get()), FDevKitActionWorkbenchModel::Snapshot(B.Get()));
	const TArray<FDevKitActionWorkbenchAction> Actions = FDevKitActionWorkbenchModel::GetActions(A.Get());
	TestEqual(TEXT("Four registered builtin actions, without duplicate dash"), Actions.Num(), 4);
	TSet<FGameplayTag> Unique;
	for (const auto& Action : Actions) Unique.Add(Action.Id);
	TestEqual(TEXT("Catalog identities are unique"), Unique.Num(), Actions.Num());
	const FGameplayTag TargetOnly = Tag(TEXT("Character.State.Equipment.SwitchWeapon"));
	A->Rules[0].CancelTags.AddTag(TargetOnly);
	TestTrue(TEXT("Target-only identities are not omitted from inventory"),
		FDevKitActionWorkbenchModel::GetActions(A.Get()).ContainsByPredicate([TargetOnly](const auto& Action) { return Action.Id == TargetOnly; }));
	const FDevKitActionWorkbenchCell Empty = FDevKitActionWorkbenchModel::EvaluateCell(B.Get(), Skill(), Attack());
	TestFalse(TEXT("No target is not an invented allow rule"), Empty.bBlocked || Empty.bCancels);
	TestFalse(TEXT("Explicit unsupported pair is explained"), Empty.Explanation.IsEmpty());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchApplyUndoTest,
	"DevKit.ActionWorkbench.ApplyIsExplicitAndUndoable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchApplyUndoTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	if (!GEditor || HasPlayContext())
	{
		AddError(TEXT("This editor transaction test must run outside PIE/Simulate. No undo was performed."));
		return false;
	}
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewAsset());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	if (!TestTrue(TEXT("Load"), Model.Load(Source.Get(), Message))) return false;
	if (!TestTrue(TEXT("Create own draft edit transaction"), Model.SetBlocked(Attack(), Dash(), true, Message))) return false;
	const FString Original = FDevKitActionWorkbenchModel::Snapshot(Source.Get());
	if (!TestTrue(TEXT("Explicit apply succeeds on test-only asset"), Model.ApplyDraft(Message))) return false;
	TestTrue(TEXT("Applied but unsaved is separate from draft dirty"), Model.HasPendingApply());
	TestTrue(TEXT("Apply marks only test source dirty"), Source->GetOutermost()->IsDirty());
	TestTrue(TEXT("Applied source contains requested block"), FDevKitActionWorkbenchModel::EvaluateCell(Source.Get(), Attack(), Dash()).bBlocked);
	TestFalse(TEXT("Applied draft no longer differs"), Model.IsDirty());
	TestFalse(TEXT("Second apply requires save or undo handling"), Model.ApplyDraft(Message));
	TestFalse(TEXT("Memory-only test asset cannot claim successful save"), Model.AcknowledgeSavedSource(Message));
	GEditor->UndoTransaction(); // Only reached after this test successfully created its own apply transaction.
	TestEqual(TEXT("UE undo restores source"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), Original);
	TestTrue(TEXT("Undo is an external source change, requiring reload"), Model.HasSourceChanged());
	TestFalse(TEXT("Stale draft cannot quietly reapply over undo"), Model.ApplyDraft(Message));
	Source->GetOutermost()->SetDirtyFlag(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchPIEGuardTest,
	"DevKit.ActionWorkbench.PIETransitionRejectsWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchPIEGuardTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	if (!GEditor || HasPlayContext())
	{
		AddError(TEXT("PIE guard test requires an idle editor. It does not start or stop a real play session."));
		return false;
	}
	TStrongObjectPtr<UStateConflictDataAsset> Source(NewAsset());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	if (!TestTrue(TEXT("Load isolated source"), Model.Load(Source.Get(), Message))) return false;
	if (!TestTrue(TEXT("Prepare draft before simulated play transition"), Model.SetBlocked(Attack(), Dash(), true, Message))) return false;
	const FString SourceBefore = FDevKitActionWorkbenchModel::Snapshot(Source.Get());
	const FString DraftBefore = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	const int32 ContextCount = GEditor->GetWorldContexts().Num();
	{
		FScopedPIEContext Context(*GEditor);
		TestNull(TEXT("Transition test does not replace editor PlayWorld"), GEditor->PlayWorld.Get());
		TestNull(TEXT("No game instance and no save subsystem can start"), Context.World->GetGameInstance());
		TestFalse(TEXT("World never begins gameplay"), Context.World->HasBegunPlay());
		TestFalse(TEXT("PIE context rejects block draft edits"), Model.SetBlocked(Skill(), Dash(), true, Message));
		TestFalse(TEXT("PIE context rejects cancellation draft edits"), Model.SetCancel(Skill(), Attack(), true, Message));
		TestFalse(TEXT("PIE context rejects applying previously prepared draft"), Model.ApplyDraft(Message));
		TestEqual(TEXT("Draft preserved"), FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), DraftBefore);
		TestEqual(TEXT("Source preserved"), FDevKitActionWorkbenchModel::Snapshot(Source.Get()), SourceBefore);
		TestFalse(TEXT("Source remains clean"), Source->GetOutermost()->IsDirty());
	}
	TestEqual(TEXT("RAII restores exact world-context count"), GEditor->GetWorldContexts().Num(), ContextCount);
	TestFalse(TEXT("No PIE context remains"), HasPlayContext());
	TestTrue(TEXT("Editing resumes outside simulated transition"), Model.SetCancel(Skill(), Attack(), true, Message));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitActionWorkbenchDiskGuardTest,
	"DevKit.ActionWorkbench.IsolatedDiskSaveAndConflictGuards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitActionWorkbenchDiskGuardTest::RunTest(const FString& Parameters)
{
	using namespace DevKitActionWorkbenchTests;
	if (!GEditor || HasPlayContext())
	{
		AddError(TEXT("Isolated disk guard test requires an idle editor; no real asset was opened or saved."));
		return false;
	}
	FScopedDiskPackage Disk;
	if (!TestTrue(TEXT("Create a unique Saved-only mount; never overwrite existing path"), Disk.Initialize())) return false;
	AddInfo(TEXT("Isolated fixture retained for inspection; mount lasts until editor exit: ") + Disk.Filename);
	AddRule(Disk.Asset.Get(), Dash()).BlockTags.AddTag(Skill());
	if (!TestTrue(TEXT("Initial isolated package save"), Disk.Save())) return false;
	TestFalse(TEXT("Save clears source dirty state"), Disk.Asset->GetOutermost()->IsDirty());
	FDevKitActionWorkbenchModel Model;
	FText Message;
	if (!TestTrue(TEXT("Load disk-backed test asset"), Model.Load(Disk.Asset.Get(), Message))) return false;
	if (!TestTrue(TEXT("Prepare isolated draft"), Model.SetBlocked(Attack(), Dash(), true, Message))) return false;
	const FString Original = FDevKitActionWorkbenchModel::Snapshot(Disk.Asset.Get());
	if (!TestTrue(TEXT("Mark only owned test file read-only"), FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Disk.Filename, true))) return false;
	TestFalse(TEXT("Read-only file prevents apply"), Model.ApplyDraft(Message));
	TestEqual(TEXT("Read-only refusal preserves asset"), FDevKitActionWorkbenchModel::Snapshot(Disk.Asset.Get()), Original);
	TestFalse(TEXT("Read-only refusal preserves dirty flag"), Disk.Asset->GetOutermost()->IsDirty());
	if (!TestTrue(TEXT("Restore owned file writable"), FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Disk.Filename, false))) return false;
	if (!TestTrue(TEXT("Explicit apply"), Model.ApplyDraft(Message))) return false;
	TestTrue(TEXT("Apply is pending until a save occurs"), Model.HasPendingApply());
	if (!TestTrue(TEXT("Explicit save of only isolated source"), Disk.Save())) return false;
	TestTrue(TEXT("Acknowledge a real successful save"), Model.AcknowledgeSavedSource(Message));
	TestFalse(TEXT("Saved apply no longer pending"), Model.HasPendingApply());
	TestFalse(TEXT("Successful save updates disk baseline"), Model.HasSourceChanged());
	if (!TestTrue(TEXT("Prepare next draft"), Model.SetCancel(Attack(), Dash(), true, Message))) return false;
	const FString DraftBeforeDiskChange = FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft());
	const FString InMemorySource = FDevKitActionWorkbenchModel::Snapshot(Disk.Asset.Get());
	const int32 PriorityBefore = Disk.Asset->Rules[0].Priority;
	Disk.Asset->Rules[0].Priority = PriorityBefore + 1;
	if (!TestTrue(TEXT("Simulate a different valid version on the owned file"), Disk.Save())) return false;
	Disk.Asset->Rules[0].Priority = PriorityBefore; // Deliberately leave the loaded UObject stale, as with an external sync.
	const FString ExternalHash = LexToString(FMD5Hash::HashFile(*Disk.Filename));
	TestEqual(TEXT("In-memory source alone still matches its baseline"), FDevKitActionWorkbenchModel::Snapshot(Disk.Asset.Get()), InMemorySource);
	TestTrue(TEXT("Disk fingerprint detects the external version"), Model.HasSourceChanged());
	TestFalse(TEXT("Cannot overwrite external disk version"), Model.ApplyDraft(Message));
	TestFalse(TEXT("Reset cannot bless a stale loaded UObject"), Model.Reset(Message));
	TestFalse(TEXT("Direct Load of same UObject also cannot bypass protection"), Model.Load(Disk.Asset.Get(), Message));
	TestEqual(TEXT("Draft is retained after all refusals"), FDevKitActionWorkbenchModel::Snapshot(Model.GetDraft()), DraftBeforeDiskChange);
	TestEqual(TEXT("Source remains untouched after all refusals"), FDevKitActionWorkbenchModel::Snapshot(Disk.Asset.Get()), InMemorySource);
	TestEqual(TEXT("External disk version is never overwritten"), LexToString(FMD5Hash::HashFile(*Disk.Filename)), ExternalHash);
	TestFalse(TEXT("Conflict refusal never dirties source"), Disk.Asset->GetOutermost()->IsDirty());
	return true;
}

#endif
