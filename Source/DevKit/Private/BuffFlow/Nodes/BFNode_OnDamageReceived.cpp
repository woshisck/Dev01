#include "BuffFlow/Nodes/BFNode_OnDamageReceived.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_OnDamageReceived::UBFNode_OnDamageReceived(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnDamage")) };
}

void UBFNode_OnDamageReceived::ExecuteInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			ASC->ReceivedDamage.AddDynamic(this, &UBFNode_OnDamageReceived::HandleDamageReceived);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->ReceivedDamage.RemoveDynamic(this, &UBFNode_OnDamageReceived::HandleDamageReceived);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnDamageReceived::HandleDamageReceived(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	CachedDamage = Damage;
	TriggerOutput(TEXT("OnDamage"), false);
}

void UBFNode_OnDamageReceived::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->ReceivedDamage.RemoveDynamic(this, &UBFNode_OnDamageReceived::HandleDamageReceived);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
