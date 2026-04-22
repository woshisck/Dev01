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
    bool bActuallyIncremented = false;
    if (AYogCharacterBase* Owner = GetBuffOwner())
    {
        if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
        {
            const int32 PhaseBefore = BGC->GetCurrentPhase();
            BGC->IncrementPhase();
            bActuallyIncremented = (BGC->GetCurrentPhase() > PhaseBefore);
        }
    }
    // 只有真正升阶时才触发 Out，避免满阶时后续节点（如热度清零）误执行
    if (bActuallyIncremented)
        TriggerOutput(TEXT("Out"), true);
}
