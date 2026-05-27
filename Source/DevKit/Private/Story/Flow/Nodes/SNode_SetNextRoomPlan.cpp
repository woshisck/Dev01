#include "Story/Flow/Nodes/SNode_SetNextRoomPlan.h"

#include "System/YogGameInstanceBase.h"

USNode_SetNextRoomPlan::USNode_SetNextRoomPlan(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Level");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetNextRoomPlan::ExecuteInput(const FName& PinName)
{
	UYogGameInstanceBase* GI = GetWorld()
		? Cast<UYogGameInstanceBase>(GetWorld()->GetGameInstance())
		: nullptr;

	ApplyNextRoomPlan(GI);
	TriggerOutput(TEXT("Out"), true);
}

bool USNode_SetNextRoomPlan::ApplyNextRoomPlan(UYogGameInstanceBase* GameInstance) const
{
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetNextRoomPlan] GameInstance not found."));
		return false;
	}

	if (bClearPlan)
	{
		GameInstance->ClearPendingStoryNextRoomPlan();
		return true;
	}

	if (!Plan.HasAnyOverride())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetNextRoomPlan] Plan has no override data."));
		return false;
	}

	GameInstance->SetPendingStoryNextRoomPlan(Plan);
	return true;
}
