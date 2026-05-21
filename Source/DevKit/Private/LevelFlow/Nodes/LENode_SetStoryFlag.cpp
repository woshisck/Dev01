#include "LevelFlow/Nodes/LENode_SetStoryFlag.h"
#include "Story/StoryEngineSubsystem.h"
#include "Engine/GameInstance.h"

ULENode_SetStoryFlag::ULENode_SetStoryFlag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Story");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void ULENode_SetStoryFlag::ExecuteInput(const FName& PinName)
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UStoryEngineSubsystem* StoryEngine = GI ? GI->GetSubsystem<UStoryEngineSubsystem>() : nullptr;

	if (StoryEngine && FlagTag.IsValid())
	{
		StoryEngine->SetStoryFlag(FlagTag, Scope, bValue);
	}

	TriggerOutput(TEXT("Out"), true);
}
