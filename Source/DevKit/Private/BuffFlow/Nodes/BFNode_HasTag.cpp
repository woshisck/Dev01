#include "BuffFlow/Nodes/BFNode_HasTag.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_HasTag::UBFNode_HasTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins = { FFlowPin(TEXT("Yes")), FFlowPin(TEXT("No")) };
}

void UBFNode_HasTag::ExecuteInput(const FName& PinName)
{
	bool bHasTag = false;

	AActor* TargetActor = ResolveTarget(Target);
	if (TargetActor)
	{
		UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		if (ASC && Tag.IsValid())
		{
			bHasTag = ASC->HasMatchingGameplayTag(Tag);
		}
	}

	TriggerOutput(bHasTag ? TEXT("Yes") : TEXT("No"), true);
}
