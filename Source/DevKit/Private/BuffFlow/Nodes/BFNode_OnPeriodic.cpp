#include "BuffFlow/Nodes/BFNode_OnPeriodic.h"
#include "Engine/World.h"
#include "TimerManager.h"

UBFNode_OnPeriodic::UBFNode_OnPeriodic(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("Tick")) };
}

void UBFNode_OnPeriodic::ExecuteBuffFlowInput(const FName& PinName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (PinName == TEXT("In"))
	{
		// 防止重复启动
		if (TimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
		}

		World->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UBFNode_OnPeriodic::OnTimerTick,
			FMath::Max(Interval, 0.1f),
			true,           // bLoop
			bFireImmediately ? 0.f : FMath::Max(Interval, 0.1f)  // FirstDelay
		);
	}
	else if (PinName == TEXT("Stop"))
	{
		if (TimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
			TimerHandle.Invalidate();
		}
	}
}

void UBFNode_OnPeriodic::OnTimerTick()
{
	// false = 不结束 Flow，保持 Timer 持续运行
	TriggerOutput(TEXT("Tick"), false);
}

void UBFNode_OnPeriodic::Cleanup()
{
	if (UWorld* World = GetWorld())
	{
		if (TimerHandle.IsValid())
		{
			World->GetTimerManager().ClearTimer(TimerHandle);
			TimerHandle.Invalidate();
		}
	}

	Super::Cleanup();
}
