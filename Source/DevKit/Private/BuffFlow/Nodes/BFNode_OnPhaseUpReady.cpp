#include "BuffFlow/Nodes/BFNode_OnPhaseUpReady.h"
#include "Component/BackpackGridComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_OnPhaseUpReady::UBFNode_OnPhaseUpReady(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("OnPhaseUp")) };
}

void UBFNode_OnPhaseUpReady::ExecuteInput(const FName& PinName)
{
    if (AYogCharacterBase* Owner = GetBuffOwner())
    {
        if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
        {
            BoundBGC = BGC;
            BGC->OnPhaseUpReady.AddUObject(this, &UBFNode_OnPhaseUpReady::HandlePhaseUpReady);
        }
    }
}

void UBFNode_OnPhaseUpReady::HandlePhaseUpReady()
{
    TriggerOutput(TEXT("OnPhaseUp"), false);
}

void UBFNode_OnPhaseUpReady::Cleanup()
{
    if (BoundBGC.IsValid())
    {
        BoundBGC->OnPhaseUpReady.RemoveAll(this);
        BoundBGC.Reset();
    }
    Super::Cleanup();
}
