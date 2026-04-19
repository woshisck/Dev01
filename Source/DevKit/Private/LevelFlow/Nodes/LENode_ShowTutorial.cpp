#include "LevelFlow/Nodes/LENode_ShowTutorial.h"
#include "Tutorial/TutorialManager.h"
#include "Kismet/GameplayStatics.h"

ULENode_ShowTutorial::ULENode_ShowTutorial(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void ULENode_ShowTutorial::ExecuteInput(const FName& PinName)
{
	if (UTutorialManager* TM = GetTutorialManager())
	{
		APlayerController* PC = GetPlayerController();
		TM->ShowByEventID(EventID, PC);
	}
	TriggerOutput(TEXT("Out"), true);
}
