#include "Story/Flow/Nodes/SNode_RecordProgress.h"

#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleTypes.h"
#include "Story/Encounter/StoryEncounterRuntimeSubsystem.h"

USNode_RecordProgress::USNode_RecordProgress(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Progress");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_RecordProgress::ExecuteInput(const FName& PinName)
{
	const FGameplayTag ProgressTag = UStoryEncounterRuntimeSubsystem::MakeProgressTag(EncounterId, ProgressKey);
	if (!ProgressTag.IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[SNode_RecordProgress] MakeProgressTag failed for EncounterId=%s ProgressKey=%s — check tag registration."),
			*EncounterId.ToString(), *ProgressKey.ToString());
	}
	else if (UStoryEngineSubsystem* Engine = GetStoryEngine())
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::SetFlag;
		Action.FlagTag = ProgressTag;
		Action.FlagScope = EStoryFlagScope::Save;
		Engine->ExecuteStoryAction(Action, MakeStoryEventContext());
	}
	TriggerOutput(TEXT("Out"), true);
}
