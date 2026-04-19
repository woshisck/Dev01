#include "LevelFlow/Nodes/LENode_Delay.h"
#include "Containers/Ticker.h"

ULENode_Delay::ULENode_Delay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void ULENode_Delay::ExecuteInput(const FName& PinName)
{
	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float) -> bool
		{
			TriggerOutput(TEXT("Out"), true);
			return false;
		}),
		Duration);
}

void ULENode_Delay::Cleanup()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	Super::Cleanup();
}
