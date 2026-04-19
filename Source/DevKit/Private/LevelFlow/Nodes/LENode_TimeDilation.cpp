#include "LevelFlow/Nodes/LENode_TimeDilation.h"
#include "Kismet/GameplayStatics.h"
#include "Containers/Ticker.h"

ULENode_TimeDilation::ULENode_TimeDilation(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("LevelEvent");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void ULENode_TimeDilation::ExecuteInput(const FName& PinName)
{
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), DilationScale);

	TickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float) -> bool
		{
			UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
			TriggerOutput(TEXT("Out"), true);
			return false;
		}),
		Duration);
}

void ULENode_TimeDilation::Cleanup()
{
	if (TickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickerHandle);
		TickerHandle.Reset();
	}
	UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
	Super::Cleanup();
}
