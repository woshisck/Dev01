#include "Story/Flow/Nodes/SNode_ShowHint.h"

#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleTypes.h"

USNode_ShowHint::USNode_ShowHint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|UI");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_ShowHint::ExecuteInput(const FName& PinName)
{
	if (UStoryEngineSubsystem* Engine = GetStoryEngine())
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::ShowInfoHint;
		Action.HintText = HintText;
		Action.HintDuration = Duration;
		Engine->ExecuteStoryAction(Action, FStoryEventContext{});
	}
	TriggerOutput(TEXT("Out"), true);
}
