#include "BuffFlow/Nodes/BFNode_OnDash.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_OnDash::UBFNode_OnDash(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnDash")) };
}

void UBFNode_OnDash::ExecuteBuffFlowInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			ASC->OnDashExecuted.AddDynamic(this, &UBFNode_OnDash::HandleDash);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->OnDashExecuted.RemoveDynamic(this, &UBFNode_OnDash::HandleDash);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnDash::HandleDash()
{
	TriggerOutput(TEXT("OnDash"), false);
}

void UBFNode_OnDash::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->OnDashExecuted.RemoveDynamic(this, &UBFNode_OnDash::HandleDash);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
