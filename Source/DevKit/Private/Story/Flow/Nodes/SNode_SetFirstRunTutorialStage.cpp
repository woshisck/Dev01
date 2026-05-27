#include "Story/Flow/Nodes/SNode_SetFirstRunTutorialStage.h"

USNode_SetFirstRunTutorialStage::USNode_SetFirstRunTutorialStage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|FirstRunTutorial");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_SetFirstRunTutorialStage::ExecuteInput(const FName& PinName)
{
	ApplyStage(GetWorld() ? GetWorld()->GetGameInstance() : nullptr);
	TriggerOutput(TEXT("Out"), true);
}

bool USNode_SetFirstRunTutorialStage::ApplyStage(UGameInstance* GameInstance) const
{
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_SetFirstRunTutorialStage] GameInstance not found."));
		return false;
	}

	if (UFirstRunTutorialDirectorSubsystem* Director = GameInstance->GetSubsystem<UFirstRunTutorialDirectorSubsystem>())
	{
		Director->SetStage(Stage);
		return true;
	}

	UE_LOG(LogTemp, Warning, TEXT("[SNode_SetFirstRunTutorialStage] Director subsystem not found."));
	return false;
}
