#include "BuffFlow/Nodes/BFNode_RemoveTag.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_RemoveTag::UBFNode_RemoveTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBFNode_RemoveTag::ExecuteInput(const FName& PinName)
{
	AActor* TargetActor = ResolveTarget(Target);
	if (TargetActor)
	{
		UYogAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UYogAbilitySystemComponent>();
		if (ASC && Tag.IsValid())
		{
			ASC->RemoveGameplayTagWithCount(Tag, Count);
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
