#pragma once

#include "CoreMinimal.h"
#include "Async/Future.h"

enum class EYogSaveOperation : uint8
{
	Write,
	Delete
};

// No UObject crosses the I/O boundary. Bytes are captured when the request is made.
struct FYogSaveOperation
{
	FString SlotName;
	EYogSaveOperation Kind = EYogSaveOperation::Write;
	TArray<uint8> Bytes;
};

struct FYogSaveOperationResult
{
	FYogSaveOperation Operation;
	bool bSuccess = false;
};

// Game-thread slot control, separated from UObject/UI code for deterministic tests.
class FYogSaveSlotState
{
public:
	bool TryBeginMutation()
	{
		if (bMutating) return false;
		bMutating = true;
		return true;
	}
	void EndMutation() { bMutating = false; }
	uint64 GetPreviewRevision(int32 SlotIndex) const { return PreviewRevisions.FindRef(SlotIndex); }
	void AdvancePreviewRevision(int32 SlotIndex) { ++PreviewRevisions.FindOrAdd(SlotIndex); }
	bool IsPreviewCurrent(int32 SlotIndex, uint64 Revision) const
	{
		return GetPreviewRevision(SlotIndex) == Revision;
	}
	static bool IsSameSelection(int32 RequestedSlot, const void* RequestedObject,
		int32 CurrentSlot, const void* CurrentObject)
	{
		return RequestedObject && RequestedSlot == CurrentSlot && RequestedObject == CurrentObject;
	}

private:
	bool bMutating = false;
	TMap<int32, uint64> PreviewRevisions;
};

// Game-thread owned; only the executor touches storage. Injectable for disk-free tests.
class DEVKIT_API FYogSaveOperationQueue
{
public:
	using FExecutor = TFunction<TFuture<bool>(const FYogSaveOperation&)>;
	explicit FYogSaveOperationQueue(FExecutor InExecutor);
	~FYogSaveOperationQueue();
	void Write(FString SlotName, TArray<uint8> Bytes);
	void Delete(FString SlotName);
	void Poll();
	bool Flush();
	// Idempotent: drain accepted work, then retry remaining failures at most once.
	bool DrainForShutdown();
	bool IsBusy() const;
	bool HasFailures() const { return !FailedOperations.IsEmpty(); }
	bool GetUnsavedBytes(const FString& SlotName, TArray<uint8>& OutBytes) const;
	void RetryFailures();
	TArray<FYogSaveOperationResult> TakeResults();

private:
	void StartNext();
	void FinishActive();
	FExecutor Executor;
	TArray<FYogSaveOperation> Pending;
	TOptional<FYogSaveOperation> Active;
	TFuture<bool> ActiveFuture;
	TMap<FString, FYogSaveOperation> FailedOperations;
	TArray<FYogSaveOperationResult> Results;
	bool bShutdownDrainAttempted = false;
};
