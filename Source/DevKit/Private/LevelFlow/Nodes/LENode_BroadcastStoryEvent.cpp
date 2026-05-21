#include "LevelFlow/Nodes/LENode_BroadcastStoryEvent.h"
#include "Story/StoryEngineSubsystem.h"
#include "Story/StoryRuleTypes.h"
#include "Engine/GameInstance.h"

ULENode_BroadcastStoryEvent::ULENode_BroadcastStoryEvent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Story");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void ULENode_BroadcastStoryEvent::ExecuteInput(const FName& PinName)
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UStoryEngineSubsystem* StoryEngine = GI ? GI->GetSubsystem<UStoryEngineSubsystem>() : nullptr;

	if (StoryEngine && EventTag.IsValid())
	{
		FStoryEventContext Context;
		Context.EventTag   = EventTag;
		Context.ContextTag = ContextTag;
		Context.AreaTag    = AreaTag;
		Context.ItemTag    = ItemTag;
		StoryEngine->BroadcastStoryEventWithContext(Context);
	}

	TriggerOutput(TEXT("Out"), true);
}
