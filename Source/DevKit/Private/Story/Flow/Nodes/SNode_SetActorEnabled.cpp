#include "Story/Flow/Nodes/SNode_SetActorEnabled.h"

#include "EngineUtils.h"

USNode_SetActorEnabled::USNode_SetActorEnabled(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Level");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetActorEnabled::ExecuteInput(const FName& PinName)
{
	if (TargetActorName.IsNone() && TargetActorTag.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetActorEnabled] TargetActorName and TargetActorTag are both None — skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		const bool bNameMatch = !TargetActorName.IsNone() && Actor->GetFName() == TargetActorName;
		const bool bTagMatch  = !TargetActorTag.IsNone()  && Actor->Tags.Contains(TargetActorTag);
		if (!bNameMatch && !bTagMatch)
		{
			continue;
		}

		Actor->SetActorHiddenInGame(!bEnabled);
		Actor->SetActorEnableCollision(bEnabled);
		Actor->SetActorTickEnabled(bEnabled);
		++Count;
	}

	if (Count == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetActorEnabled] No actor matched. Name=%s Tag=%s Enabled=%d"),
			*TargetActorName.ToString(), *TargetActorTag.ToString(), bEnabled ? 1 : 0);
	}

	TriggerOutput(TEXT("Out"), true);
}
