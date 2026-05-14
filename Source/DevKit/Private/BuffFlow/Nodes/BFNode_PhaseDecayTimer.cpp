#include "BuffFlow/Nodes/BFNode_PhaseDecayTimer.h"

UBFNode_PhaseDecayTimer::UBFNode_PhaseDecayTimer(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
#if WITH_EDITOR
    Category = TEXT("BuffFlow|Phase");
#endif
    InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Cancel")) };
    OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_PhaseDecayTimer::ExecuteBuffFlowInput(const FName& PinName)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (PinName == TEXT("In"))
    {
        // 已在运行则静默忽略（原 C++ 行为：if !IsTimerActive then SetTimer）
        if (!World->GetTimerManager().IsTimerActive(TimerHandle))
        {
            World->GetTimerManager().SetTimer(
                TimerHandle, this, &UBFNode_PhaseDecayTimer::OnTimerComplete, Duration, false);
        }
    }
    else if (PinName == TEXT("Cancel"))
    {
        World->GetTimerManager().ClearTimer(TimerHandle);
    }
}

void UBFNode_PhaseDecayTimer::OnTimerComplete()
{
    TriggerOutput(TEXT("Out"), false);
}

void UBFNode_PhaseDecayTimer::Cleanup()
{
    if (UWorld* World = GetWorld())
        World->GetTimerManager().ClearTimer(TimerHandle);

    Super::Cleanup();
}
