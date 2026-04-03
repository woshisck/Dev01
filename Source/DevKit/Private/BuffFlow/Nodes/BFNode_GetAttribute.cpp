#include "BuffFlow/Nodes/BFNode_GetAttribute.h"
#include "AbilitySystemComponent.h"

UBFNode_GetAttribute::UBFNode_GetAttribute(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBFNode_GetAttribute::ExecuteInput(const FName& PinName)
{
	CachedValue = 0.f;

	AActor* TargetActor = ResolveTarget(Target);
	if (TargetActor)
	{
		UAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
		if (ASC && Attribute.IsValid())
		{
			bool bFound = false;
			CachedValue = ASC->GetNumericAttribute(Attribute);
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
