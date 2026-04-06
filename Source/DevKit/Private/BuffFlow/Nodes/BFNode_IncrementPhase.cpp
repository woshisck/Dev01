#include "BuffFlow/Nodes/BFNode_IncrementPhase.h"
#include "Component/BackpackGridComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_IncrementPhase::UBFNode_IncrementPhase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_IncrementPhase::ExecuteInput(const FName& PinName)
{
    if (AYogCharacterBase* Owner = GetBuffOwner())
    {
        if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
        {
            BGC->IncrementPhase();
        }
    }
    TriggerOutput(TEXT("Out"), true);
}
