#include "BuffFlow/Nodes/BFNode_OnHeatReachedZero.h"
#include "Component/BackpackGridComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_OnHeatReachedZero::UBFNode_OnHeatReachedZero(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")) };
    OutputPins = { FFlowPin(TEXT("OnReachedZero")), FFlowPin(TEXT("OnAboveZero")) };
}

void UBFNode_OnHeatReachedZero::ExecuteInput(const FName& PinName)
{
    if (AYogCharacterBase* Owner = GetBuffOwner())
    {
        if (UBackpackGridComponent* BGC = Owner->FindComponentByClass<UBackpackGridComponent>())
        {
            BoundBGC = BGC;
            BGC->OnHeatReachedZero.AddUObject(this, &UBFNode_OnHeatReachedZero::HandleHeatReachedZero);
            BGC->OnHeatAboveZero.AddUObject(this, &UBFNode_OnHeatReachedZero::HandleHeatAboveZero);
        }
    }
}

void UBFNode_OnHeatReachedZero::HandleHeatReachedZero()
{
    TriggerOutput(TEXT("OnReachedZero"), false);
}

void UBFNode_OnHeatReachedZero::HandleHeatAboveZero()
{
    TriggerOutput(TEXT("OnAboveZero"), false);
}

void UBFNode_OnHeatReachedZero::Cleanup()
{
    if (BoundBGC.IsValid())
    {
        BoundBGC->OnHeatReachedZero.RemoveAll(this);
        BoundBGC->OnHeatAboveZero.RemoveAll(this);
        BoundBGC.Reset();
    }
    Super::Cleanup();
}
