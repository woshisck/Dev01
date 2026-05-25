#include "Story/Flow/Nodes/SNode_ShowTutorialPopup.h"

#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleTypes.h"

USNode_ShowTutorialPopup::USNode_ShowTutorialPopup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|UI");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_ShowTutorialPopup::ExecuteInput(const FName& PinName)
{
	if (UStoryEngineSubsystem* Engine = GetStoryEngine())
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::ShowTutorialPopup;
		Action.TutorialEventId = TutorialEventId;
		Action.TutorialPages = InlinePages;
		Action.bPauseGame = bPauseGame;
		Engine->ExecuteStoryAction(Action, FStoryEventContext{});
	}
	TriggerOutput(TEXT("Out"), true);
}
