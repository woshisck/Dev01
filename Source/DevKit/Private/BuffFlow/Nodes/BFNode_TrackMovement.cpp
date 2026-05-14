#include "BuffFlow/Nodes/BFNode_TrackMovement.h"
#include "Types/FlowDataPinResults.h"
#include "Engine/World.h"
#include "TimerManager.h"

UBFNode_TrackMovement::UBFNode_TrackMovement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnTrigger")), FFlowPin(TEXT("OnStationary")), FFlowPin(TEXT("Stopped")) };

	DistancePerTrigger.Value = 100.f;
}

void UBFNode_TrackMovement::ExecuteBuffFlowInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (PinName == TEXT("In"))
	{
		if (TickTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TickTimerHandle);
		}

		TrackedActor = ResolveTarget(Target);
		if (!TrackedActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_TrackMovement] Target 解析为 null"));
			return;
		}

		LastPosition = TrackedActor->GetActorLocation();
		AccumulatedDistance = 0.f;
		StationaryTimer = 0.f;

		World->GetTimerManager().SetTimer(
			TickTimerHandle,
			this,
			&UBFNode_TrackMovement::OnTick,
			FMath::Max(TickInterval, 0.05f),
			true,
			FMath::Max(TickInterval, 0.05f)
		);
	}
	else if (PinName == TEXT("Stop"))
	{
		if (TickTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TickTimerHandle);
			TickTimerHandle.Invalidate();
		}
		TriggerOutput(TEXT("Stopped"), true);
	}
}

void UBFNode_TrackMovement::OnTick()
{
	if (!TrackedActor.IsValid())
	{
		Cleanup();
		return;
	}

	FFlowDataPinResult_Float DistResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_TrackMovement, DistancePerTrigger));
	const float ResolvedDistance = (DistResult.Result == EFlowDataPinResolveResult::Success)
		? DistResult.Value : DistancePerTrigger.Value;
	const float Threshold = FMath::Max(ResolvedDistance, 1.f);

	const FVector CurrentPos = TrackedActor->GetActorLocation();
	const float Delta = FVector::Dist(CurrentPos, LastPosition);
	LastPosition = CurrentPos;
	AccumulatedDistance += Delta;

	if (AccumulatedDistance >= Threshold)
	{
		AccumulatedDistance = FMath::Fmod(AccumulatedDistance, Threshold);
		TriggerOutput(TEXT("OnTrigger"), false);
	}

	if (Delta < 1.f)
	{
		StationaryTimer += FMath::Max(TickInterval, 0.05f);
		if (StationaryTimeout > 0.f && StationaryTimer >= StationaryTimeout)
		{
			TriggerOutput(TEXT("OnStationary"), true);
		}
	}
	else
	{
		StationaryTimer = 0.f;
	}
}

void UBFNode_TrackMovement::Cleanup()
{
	if (UWorld* World = GetWorld())
	{
		if (TickTimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TickTimerHandle);
			TickTimerHandle.Invalidate();
		}
	}
	TrackedActor.Reset();
	AccumulatedDistance = 0.f;
	StationaryTimer = 0.f;

	Super::Cleanup();
}
