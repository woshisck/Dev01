#include "BuffFlow/Nodes/BFNode_OnKill.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_OnKill::UBFNode_OnKill(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnKill")) };
}

void UBFNode_OnKill::ExecuteInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			ASC->OnKilledTarget.AddDynamic(this, &UBFNode_OnKill::HandleKill);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->OnKilledTarget.RemoveDynamic(this, &UBFNode_OnKill::HandleKill);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnKill::HandleKill(AActor* Target, FVector DeathLocation)
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		// 供 HasTag 节点检查被击杀者身上的 tag
		BFC->LastEventContext.DamageReceiver = Target;
		// 供 SpawnActorAtLocation 节点读取生成位置
		BFC->LastKillLocation = DeathLocation;
	}

	TriggerOutput(TEXT("OnKill"), false);
}

void UBFNode_OnKill::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->OnKilledTarget.RemoveDynamic(this, &UBFNode_OnKill::HandleKill);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
