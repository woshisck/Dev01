#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "SaveGame/YogSaveOperationQueue.h"
#include "SaveGame/YogWeaponSaveSupport.h"

namespace YogSaveSafetyTests
{
// No editor world, GameInstance initialization, real save slot, or disk I/O is used.
struct FFakeStorage
{
	TArray<FYogSaveOperation> Started;
	TArray<TSharedPtr<TPromise<bool>>> Completions;
	TFuture<bool> Start(const FYogSaveOperation& Operation)
	{
		Started.Add(Operation);
		TSharedPtr<TPromise<bool>> Promise = MakeShared<TPromise<bool>>();
		TFuture<bool> Future = Promise->GetFuture();
		Completions.Add(Promise);
		return Future;
	}
	void Complete(int32 Index, bool bSuccess) { Completions[Index]->SetValue(bSuccess); }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveQueueSlotSnapshotTest,
	"DevKit.SaveSafety.Queue.SnapshotsStayWithSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveQueueSlotSnapshotTest::RunTest(const FString& Parameters)
{
	YogSaveSafetyTests::FFakeStorage Storage;
	FYogSaveOperationQueue Queue([&Storage](const FYogSaveOperation& Op) { return Storage.Start(Op); });
	FString SelectedSlot = TEXT("Fake_A");
	TArray<uint8> LiveBytes{1};
	Queue.Write(SelectedSlot, LiveBytes);
	LiveBytes[0] = 2;
	Queue.Write(SelectedSlot, LiveBytes);
	SelectedSlot = TEXT("Fake_B");
	Queue.Write(SelectedSlot, {3});
	TestEqual(TEXT("Only one operation runs at a time"), Storage.Started.Num(), 1);
	TestEqual(TEXT("First in-flight snapshot is immutable"), Storage.Started[0].Bytes[0], uint8(1));
	Storage.Complete(0, true);
	Queue.Poll();
	TestEqual(TEXT("Queued A write keeps A after selection moves to B"), Storage.Started[1].SlotName, FString(TEXT("Fake_A")));
	TestEqual(TEXT("Queued A includes latest A state"), Storage.Started[1].Bytes[0], uint8(2));
	Storage.Complete(1, true);
	Queue.Poll();
	TestEqual(TEXT("B follows both A snapshots"), Storage.Started[2].SlotName, FString(TEXT("Fake_B")));
	Storage.Complete(2, true);
	TestTrue(TEXT("Flush drains all completed writes"), Queue.Flush());
	const TArray<FYogSaveOperationResult> Results = Queue.TakeResults();
	TestEqual(TEXT("Each completion identifies its actual slot"), Results[0].Operation.SlotName, FString(TEXT("Fake_A")));
	TestEqual(TEXT("Each completion contains its actual saved bytes"), Results[0].Operation.Bytes[0], uint8(1));
	TestFalse(TEXT("Nothing remains after flush"), Queue.IsBusy());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveQueueDeleteBarrierTest,
	"DevKit.SaveSafety.Queue.DeleteWaitsForInFlightWrite",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveQueueDeleteBarrierTest::RunTest(const FString& Parameters)
{
	YogSaveSafetyTests::FFakeStorage Storage;
	FYogSaveOperationQueue Queue([&Storage](const FYogSaveOperation& Op) { return Storage.Start(Op); });
	Queue.Write(TEXT("Fake_A"), {1});
	Queue.Write(TEXT("Fake_A"), {2});
	Queue.Delete(TEXT("Fake_A"));
	TestEqual(TEXT("Delete cannot race the in-flight writer"), Storage.Started.Num(), 1);
	Storage.Complete(0, true);
	Queue.Poll();
	TestEqual(TEXT("Pending stale write is replaced by delete"), Storage.Started.Num(), 2);
	TestTrue(TEXT("Delete is the next storage operation"), Storage.Started[1].Kind == EYogSaveOperation::Delete);
	Storage.Complete(1, true);
	TestTrue(TEXT("Delete finishes before returning to UI"), Queue.Flush());
	TestFalse(TEXT("No stale write can recreate the deleted slot"), Queue.IsBusy());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveQueueFailureRetryTest,
	"DevKit.SaveSafety.Queue.FailureRetainsSnapshotForRetry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveQueueFailureRetryTest::RunTest(const FString& Parameters)
{
	YogSaveSafetyTests::FFakeStorage Storage;
	FYogSaveOperationQueue Queue([&Storage](const FYogSaveOperation& Op) { return Storage.Start(Op); });
	Queue.Write(TEXT("Fake_A"), {42});
	Storage.Complete(0, false);
	TestFalse(TEXT("Failure is returned by flush"), Queue.Flush());
	TestTrue(TEXT("Failure remains visible after completion"), Queue.HasFailures());
	TArray<uint8> Unsaved;
	TestTrue(TEXT("Failed slot can recover its in-memory snapshot"), Queue.GetUnsavedBytes(TEXT("Fake_A"), Unsaved));
	TestEqual(TEXT("Unsaved state was not dropped"), Unsaved[0], uint8(42));
	const TArray<FYogSaveOperationResult> FailedResults = Queue.TakeResults();
	TestFalse(TEXT("Completion does not falsely report success"), FailedResults[0].bSuccess);
	Queue.RetryFailures();
	TestEqual(TEXT("Retry uses the original slot"), Storage.Started[1].SlotName, FString(TEXT("Fake_A")));
	Storage.Complete(1, true);
	TestTrue(TEXT("Successful retry clears failure"), Queue.Flush());
	TestFalse(TEXT("Failure cleared only after successful storage"), Queue.HasFailures());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveQueueDeleteThenNewGameTest,
	"DevKit.SaveSafety.Queue.NewGameDoesNotCrossDeleteBarrier",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveQueueDeleteThenNewGameTest::RunTest(const FString& Parameters)
{
	YogSaveSafetyTests::FFakeStorage Storage;
	FYogSaveOperationQueue Queue([&Storage](const FYogSaveOperation& Op) { return Storage.Start(Op); });
	Queue.Write(TEXT("Fake_A"), {1});
	Queue.Delete(TEXT("Fake_A"));
	Queue.Write(TEXT("Fake_A"), {9});
	Storage.Complete(0, true);
	Queue.Poll();
	TestTrue(TEXT("Explicit delete remains between old and new games"), Storage.Started[1].Kind == EYogSaveOperation::Delete);
	Storage.Complete(1, true);
	Queue.Poll();
	TestEqual(TEXT("Only the explicit new game recreates the slot"), Storage.Started[2].Bytes[0], uint8(9));
	Storage.Complete(2, true);
	TestTrue(TEXT("Shutdown-style flush drains the final operation"), Queue.Flush());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSaveNullLayerTest,
	"DevKit.SaveSafety.Weapon.NullLayerIsOptional",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSaveNullLayerTest::RunTest(const FString& Parameters)
{
	FWeaponInstanceData Data;
	Data.WeaponLayerClassPath = TEXT("stale");
	YogWeaponSaveSupport::CaptureLayer(nullptr, Data);
	TestNull(TEXT("No layer is a valid saved weapon state"), Data.WeaponLayer.Get());
	TestTrue(TEXT("No layer clears the old class path"), Data.WeaponLayerClassPath.IsEmpty());
	TestNull(TEXT("No layer restores without requiring a Blueprint"), YogWeaponSaveSupport::ResolveLayer(Data, nullptr).Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveQueueShutdownRetryRecoversTest,
	"DevKit.SaveSafety.Queue.ShutdownRetryRecoversOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveQueueShutdownRetryRecoversTest::RunTest(const FString& Parameters)
{
	int32 Attempts = 0;
	FYogSaveOperationQueue Queue([&Attempts](const FYogSaveOperation& Op)
	{
		TPromise<bool> Promise;
		TFuture<bool> Future = Promise.GetFuture();
		Promise.SetValue(++Attempts == 2);
		return Future;
	});
	Queue.Write(TEXT("Fake_Shutdown"), {7});
	TestTrue(TEXT("Shutdown retries a transient failure successfully"), Queue.DrainForShutdown());
	TestEqual(TEXT("Initial write plus one final retry"), Attempts, 2);
	TestFalse(TEXT("Successful retry clears retained failure"), Queue.HasFailures());
	TestTrue(TEXT("Second exit hook remains successful"), Queue.DrainForShutdown());
	TestEqual(TEXT("Second exit hook cannot retry again"), Attempts, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveQueueShutdownRetryBoundedTest,
	"DevKit.SaveSafety.Queue.ShutdownPersistentFailureIsBounded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveQueueShutdownRetryBoundedTest::RunTest(const FString& Parameters)
{
	int32 Attempts = 0;
	FYogSaveOperationQueue Queue([&Attempts](const FYogSaveOperation& Op)
	{
		++Attempts;
		TPromise<bool> Promise;
		TFuture<bool> Future = Promise.GetFuture();
		Promise.SetValue(false);
		return Future;
	});
	Queue.Write(TEXT("Fake_Shutdown"), {7});
	TestFalse(TEXT("Persistent failure remains explicit"), Queue.DrainForShutdown());
	TestEqual(TEXT("Persistent failures are attempted only twice"), Attempts, 2);
	TestTrue(TEXT("Unsaved request remains marked failed"), Queue.HasFailures());
	TestFalse(TEXT("Queue is drained rather than endlessly retrying"), Queue.IsBusy());
	TestFalse(TEXT("Second exit hook returns the same failure"), Queue.DrainForShutdown());
	TestEqual(TEXT("Both hooks share the same single retry budget"), Attempts, 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSlotResetIdentityGuardTest,
	"DevKit.SaveSafety.Slot.ResetCannotCrossSlotOrObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveSlotResetIdentityGuardTest::RunTest(const FString& Parameters)
{
	FYogSaveSlotState State;
	int32 SaveA = 0;
	int32 SaveB = 0;
	TestTrue(TEXT("Outer reset owns the slot mutation guard"), State.TryBeginMutation());
	TestFalse(TEXT("Selection from a notification cannot reenter reset"), State.TryBeginMutation());
	TestTrue(TEXT("Requested slot and object may be reset"), FYogSaveSlotState::IsSameSelection(0, &SaveA, 0, &SaveA));
	TestFalse(TEXT("A request cannot clear another current slot"), FYogSaveSlotState::IsSameSelection(0, &SaveA, 1, &SaveB));
	TestFalse(TEXT("Even the same slot cannot reset a substituted object"), FYogSaveSlotState::IsSameSelection(0, &SaveA, 0, &SaveB));
	TestFalse(TEXT("Failed selection cannot reset anything"), FYogSaveSlotState::IsSameSelection(0, nullptr, 0, nullptr));
	State.EndMutation();
	TestTrue(TEXT("A later non-reentrant user operation is allowed"), State.TryBeginMutation());
	State.EndMutation();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSlotPreviewRevisionTest,
	"DevKit.SaveSafety.Slot.StalePreviewIsDiscarded",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSaveSlotPreviewRevisionTest::RunTest(const FString& Parameters)
{
	FYogSaveSlotState State;
	const uint64 OldA = State.GetPreviewRevision(0);
	const uint64 OldB = State.GetPreviewRevision(1);
	State.AdvancePreviewRevision(0); // A write was accepted after its preview started.
	TestFalse(TEXT("A stale preview cannot overwrite the post-write UI"), State.IsPreviewCurrent(0, OldA));
	TestTrue(TEXT("A changed slot does not invalidate another slot's preview"), State.IsPreviewCurrent(1, OldB));
	const uint64 BeforeDelete = State.GetPreviewRevision(0);
	State.AdvancePreviewRevision(0);
	TestFalse(TEXT("A result started before delete must be discarded"), State.IsPreviewCurrent(0, BeforeDelete));
	const uint64 BeforeReset = State.GetPreviewRevision(0);
	State.AdvancePreviewRevision(0);
	TestFalse(TEXT("A result started before reset must be discarded"), State.IsPreviewCurrent(0, BeforeReset));
	TestTrue(TEXT("The latest request can update the UI"), State.IsPreviewCurrent(0, State.GetPreviewRevision(0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSaveLayerClassPathTest,
	"DevKit.SaveSafety.Weapon.ClassPathAndLegacyFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSaveLayerClassPathTest::RunTest(const FString& Parameters)
{
	FWeaponInstanceData Data;
	UClass* AnimationClass = UYogAnimInstance::StaticClass();
	YogWeaponSaveSupport::CaptureLayer(AnimationClass, Data);
	TestEqual(TEXT("Capture stores the actual animation class"), Data.WeaponLayer.Get(), AnimationClass);
	TestEqual(TEXT("Path identifies the animation class, not its meta-class"), Data.WeaponLayerClassPath, AnimationClass->GetPathName());
	TestEqual(TEXT("Saved class path round-trips"), YogWeaponSaveSupport::ResolveLayer(Data, nullptr).Get(), AnimationClass);
	Data.WeaponLayer = nullptr;
	Data.WeaponLayerClassPath = UClass::StaticClass()->GetPathName();
	TestEqual(TEXT("Legacy erroneous meta-class path recovers the weapon default"),
		YogWeaponSaveSupport::ResolveLayer(Data, AnimationClass).Get(), AnimationClass);
	return true;
}

#endif
