#include "BuffFlow/Nodes/BFNode_Delay.h"
#include "Types/FlowDataPinResults.h"

UBFNode_Delay::UBFNode_Delay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Utility");
#endif
	Duration = FFlowDataPinInputProperty_Float(1.f);
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Cancel")) };
	OutputPins = { FFlowPin(TEXT("Completed")), FFlowPin(TEXT("Cancelled")) };
}

void UBFNode_Delay::ExecuteBuffFlowInput(const FName& PinName)
{
	// ── Cancel 引脚 ───────────────────────────────────────────────────────
	if (PinName == TEXT("Cancel"))
	{
		if (DelayTimer.IsValid())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().ClearTimer(DelayTimer);
			}
		}
		TriggerOutput(TEXT("Cancelled"), true);
		return;
	}

	// ── In 引脚：启动计时 ─────────────────────────────────────────────────

	// 优先读取连入的数据引脚，无连线则使用节点上填写的固定值
	FFlowDataPinResult_Float PinResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_Delay, Duration));
	const float ResolvedDuration = (PinResult.Result == EFlowDataPinResolveResult::Success)
		? PinResult.Value
		: Duration.Value;

	if (ResolvedDuration <= 0.f)
	{
		// 时间 <= 0 视为立即完成
		TriggerOutput(TEXT("Completed"), true);
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		TriggerOutput(TEXT("Completed"), true);
		return;
	}

	// 重置上一次计时（In 引脚被多次触发时刷新计时）
	World->GetTimerManager().ClearTimer(DelayTimer);
	World->GetTimerManager().SetTimer(DelayTimer, [this]()
	{
		TriggerOutput(TEXT("Completed"), true);
	}, ResolvedDuration, false);

	// bFinish=false：保持节点活跃，Cleanup 才能清理计时器
	// 不调用 TriggerOutput，等计时器回调
}

void UBFNode_Delay::Cleanup()
{
	if (DelayTimer.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DelayTimer);
		}
	}
	Super::Cleanup();
}
