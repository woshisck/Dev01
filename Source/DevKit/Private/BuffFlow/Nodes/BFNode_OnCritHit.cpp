#include "BuffFlow/Nodes/BFNode_OnCritHit.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_OnCritHit::UBFNode_OnCritHit(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins  = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnCrit")) };
}

void UBFNode_OnCritHit::ExecuteInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			ASC->OnCritHit.AddDynamic(this, &UBFNode_OnCritHit::HandleCritHit);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->OnCritHit.RemoveDynamic(this, &UBFNode_OnCritHit::HandleCritHit);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnCritHit::HandleCritHit(UYogAbilitySystemComponent* TargetASC, float Damage)
{
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->LastEventContext.DamageCauser   = BFC->GetBuffOwner();
		BFC->LastEventContext.DamageReceiver = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
		BFC->LastEventContext.DamageAmount   = Damage;
	}

	TriggerOutput(TEXT("OnCrit"), false);
}

void UBFNode_OnCritHit::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->OnCritHit.RemoveDynamic(this, &UBFNode_OnCritHit::HandleCritHit);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
