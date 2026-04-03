#include "BuffFlow/Nodes/BFNode_CheckTargetType.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_CheckTargetType::UBFNode_CheckTargetType(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins = {
		FFlowPin(TEXT("对敌人")),
		FFlowPin(TEXT("对自己"))
	};
}

void UBFNode_CheckTargetType::ExecuteInput(const FName& PinName)
{
	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	if (!BFC)
	{
		TriggerOutput(TEXT("对自己"), true);
		return;
	}

	AActor* Target = BFC->LastEventContext.DamageReceiver.Get();
	AActor* Owner  = GetBuffOwner();

	// 目标有效且不是自己 → 敌人
	if (Target && Target != Owner)
	{
		TriggerOutput(TEXT("对敌人"), true);
	}
	else
	{
		TriggerOutput(TEXT("对自己"), true);
	}
}