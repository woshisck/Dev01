#include "BuffFlow/Nodes/BFNode_CheckDistance.h"
#include "Types/FlowDataPinResults.h"

UBFNode_CheckDistance::UBFNode_CheckDistance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Utility");
#endif
	InputPins  = { FFlowPin(TEXT("Save")), FFlowPin(TEXT("Check")) };
	OutputPins = { FFlowPin(TEXT("Saved")), FFlowPin(TEXT("Far")), FFlowPin(TEXT("Near")) };

	RequiredDistance.Value = 800.f;
}

void UBFNode_CheckDistance::ExecuteBuffFlowInput(const FName& PinName)
{
	if (PinName == TEXT("Save"))
	{
		TrackedActor = ResolveTarget(Target);
		if (!TrackedActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_CheckDistance] Save: Target 解析为 null"));
			return;
		}

		SavedOrigin = TrackedActor->GetActorLocation();
		TriggerOutput(TEXT("Saved"), false);
	}
	else if (PinName == TEXT("Check"))
	{
		if (!TrackedActor.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BFNode_CheckDistance] Check: TrackedActor 无效"));
			TriggerOutput(TEXT("Near"), true);
			return;
		}

		FFlowDataPinResult_Float DistResult = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_CheckDistance, RequiredDistance));
		const float ResolvedThreshold = (DistResult.Result == EFlowDataPinResolveResult::Success)
			? DistResult.Value : RequiredDistance.Value;
		const float Threshold = FMath::Max(ResolvedThreshold, 0.f);

		const float CurrentDist = FVector::Dist(TrackedActor->GetActorLocation(), SavedOrigin);

		if (CurrentDist >= Threshold)
		{
			TriggerOutput(TEXT("Far"), true);
		}
		else
		{
			TriggerOutput(TEXT("Near"), true);
		}
	}
}

void UBFNode_CheckDistance::Cleanup()
{
	TrackedActor.Reset();
	SavedOrigin = FVector::ZeroVector;

	Super::Cleanup();
}
