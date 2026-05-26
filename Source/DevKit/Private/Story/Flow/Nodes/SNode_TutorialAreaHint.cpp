#include "Story/Flow/Nodes/SNode_TutorialAreaHint.h"

#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleTypes.h"

USNode_TutorialAreaHint::USNode_TutorialAreaHint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|UI");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_TutorialAreaHint::ExecuteInput(const FName& PinName)
{
	if (UStoryEngineSubsystem* Engine = GetStoryEngine())
	{
		FStoryAction Action;
		Action.Type = EStoryActionType::ShowInfoHint;
		Action.HintText = ResolveInputAwareText(
			HintText,
			bUseInputTextVariants,
			KeyboardMouseHintText,
			GamepadHintText);
		Action.HintDuration = Duration;
		Engine->ExecuteStoryAction(Action, MakeStoryEventContext());
	}
	TriggerOutput(TEXT("Out"), true);
}
