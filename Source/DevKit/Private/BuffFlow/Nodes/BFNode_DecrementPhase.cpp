#include "BuffFlow/Nodes/BFNode_DecrementPhase.h"
#include "Component/BackpackGridComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_DecrementPhase::UBFNode_DecrementPhase(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_DecrementPhase::ExecuteBuffFlowInput(const FName& PinName)
{
    if (AYogCharacterBase* Owner = GetBuffOwner())
    {
        if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
        {
            if (BGC->GetCurrentPhase() > 0)
            {
                BGC->DecrementPhase();
            }
        }
    }
    TriggerOutput(TEXT("Out"), true);
}
