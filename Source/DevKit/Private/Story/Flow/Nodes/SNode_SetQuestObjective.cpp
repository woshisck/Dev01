#include "Story/Flow/Nodes/SNode_SetQuestObjective.h"

#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleTypes.h"

USNode_SetQuestObjective::USNode_SetQuestObjective(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Quest");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetQuestObjective::ExecuteInput(const FName& PinName)
{
	if (!QuestTaskId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetQuestObjective] QuestTaskId is not valid — skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	if (UStoryEngineSubsystem* Engine = GetStoryEngine())
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::SetQuestTask;
		Action.QuestTaskId = QuestTaskId;
		Action.QuestTaskText = DisplayText;
		Action.QuestSourceTag = SourceTag;
		Action.RelatedFlagTag = RelatedFlagTag;
		Engine->ExecuteStoryAction(Action, FStoryEventContext{});
	}

	TriggerOutput(TEXT("Out"), true);
}
