#include "SaveGame/YogSaveOperationQueue.h"

FYogSaveOperationQueue::FYogSaveOperationQueue(FExecutor InExecutor)
	: Executor(MoveTemp(InExecutor))
{
}

FYogSaveOperationQueue::~FYogSaveOperationQueue()
{
	Flush();
}

void FYogSaveOperationQueue::Write(FString SlotName, TArray<uint8> Bytes)
{
	// Coalesce only not-yet-started writes; never replace the in-flight snapshot.
	for (int32 Index = Pending.Num() - 1; Index >= 0; --Index)
	{
		if (Pending[Index].SlotName == SlotName)
		{
			if (Pending[Index].Kind == EYogSaveOperation::Write)
			{
				Pending[Index].Bytes = MoveTemp(Bytes);
				return;
			}
			break; // A delete is an ordering barrier for a subsequent new game.
		}
	}
	Pending.Add({MoveTemp(SlotName), EYogSaveOperation::Write, MoveTemp(Bytes)});
	StartNext();
}

void FYogSaveOperationQueue::Delete(FString SlotName)
{
	Pending.RemoveAll([&SlotName](const FYogSaveOperation& Op) { return Op.SlotName == SlotName; });
	FailedOperations.Remove(SlotName);
	Pending.Add({MoveTemp(SlotName), EYogSaveOperation::Delete, {}});
	StartNext();
}

bool FYogSaveOperationQueue::IsBusy() const
{
	return Active.IsSet() || !Pending.IsEmpty();
}

void FYogSaveOperationQueue::StartNext()
{
	if (Active.IsSet() || Pending.IsEmpty()) return;
	Active = MoveTemp(Pending[0]);
	Pending.RemoveAt(0);
	ActiveFuture = Executor(Active.GetValue());
}

void FYogSaveOperationQueue::FinishActive()
{
	const bool bSuccess = ActiveFuture.IsValid() && ActiveFuture.Get();
	FYogSaveOperation Finished = MoveTemp(Active.GetValue());
	Active.Reset();
	if (bSuccess)
	{
		FailedOperations.Remove(Finished.SlotName);
	}
	else
	{
		FailedOperations.Add(Finished.SlotName, Finished);
	}
	Results.Add({MoveTemp(Finished), bSuccess});
}

void FYogSaveOperationQueue::Poll()
{
	while (Active.IsSet() && (!ActiveFuture.IsValid() || ActiveFuture.IsReady()))
	{
		FinishActive();
		StartNext();
	}
}

bool FYogSaveOperationQueue::Flush()
{
	StartNext();
	while (Active.IsSet())
	{
		if (ActiveFuture.IsValid()) ActiveFuture.Wait();
		FinishActive();
		StartNext();
	}
	return !HasFailures();
}

bool FYogSaveOperationQueue::DrainForShutdown()
{
	if (bShutdownDrainAttempted) return !HasFailures();
	bShutdownDrainAttempted = true;
	if (Flush()) return true;
	RetryFailures();
	return Flush(); // No loop: persistent storage failures get one final attempt only.
}

bool FYogSaveOperationQueue::GetUnsavedBytes(const FString& SlotName, TArray<uint8>& OutBytes) const
{
	for (int32 Index = Pending.Num() - 1; Index >= 0; --Index)
	{
		if (Pending[Index].SlotName == SlotName)
		{
			if (Pending[Index].Kind != EYogSaveOperation::Write) return false;
			OutBytes = Pending[Index].Bytes;
			return true;
		}
	}
	if (Active.IsSet() && Active->SlotName == SlotName)
	{
		if (Active->Kind != EYogSaveOperation::Write) return false;
		OutBytes = Active->Bytes;
		return true;
	}
	if (const FYogSaveOperation* Failed = FailedOperations.Find(SlotName))
	{
		if (Failed->Kind != EYogSaveOperation::Write) return false;
		OutBytes = Failed->Bytes;
		return true;
	}
	return false;
}

void FYogSaveOperationQueue::RetryFailures()
{
	if (IsBusy()) return;
	for (const auto& Pair : FailedOperations) Pending.Add(Pair.Value);
	StartNext();
}

TArray<FYogSaveOperationResult> FYogSaveOperationQueue::TakeResults()
{
	TArray<FYogSaveOperationResult> Out = MoveTemp(Results);
	Results.Reset();
	return Out;
}
