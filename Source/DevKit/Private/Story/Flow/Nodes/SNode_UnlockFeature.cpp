#include "Story/Flow/Nodes/SNode_UnlockFeature.h"

#include "MetaProgression/YogMetaProgressionSubsystem.h"

USNode_UnlockFeature::USNode_UnlockFeature(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("StoryDirector|Progress");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void USNode_UnlockFeature::ExecuteInput(const FName& PinName)
{
	if (!FeatureTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_UnlockFeature] FeatureTag is not valid — skipped."));
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	if (UYogMetaProgressionSubsystem* Meta = GI ? GI->GetSubsystem<UYogMetaProgressionSubsystem>() : nullptr)
	{
		Meta->UnlockFeature(FeatureTag);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SNode_UnlockFeature] YogMetaProgressionSubsystem not found."));
	}

	TriggerOutput(TEXT("Out"), true);
}
