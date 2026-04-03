#include "BuffFlow/Nodes/BFNode_OnDamageDealt.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_OnDamageDealt::UBFNode_OnDamageDealt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnDamage")) };
}

void UBFNode_OnDamageDealt::ExecuteInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			ASC->DealtDamage.AddDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->DealtDamage.RemoveDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnDamageDealt::HandleDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage)
{
	CachedDamage = Damage;
	// Trigger output but do NOT finish - keep listening
	TriggerOutput(TEXT("OnDamage"), false);
}

void UBFNode_OnDamageDealt::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->DealtDamage.RemoveDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
