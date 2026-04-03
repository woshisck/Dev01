#include "BuffFlow/Nodes/BFNode_RemoveBuff.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/YogBuffDefinition.h"
#include "AbilitySystemComponent.h"

UBFNode_RemoveBuff::UBFNode_RemoveBuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UBFNode_RemoveBuff::ExecuteInput(const FName& PinName)
{
	if (!BuffDefinition)
	{
		TriggerOutput(TEXT("Out"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	UAbilitySystemComponent* ASC = nullptr;
	if (TargetActor)
	{
		ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	if (ASC)
	{
		// Remove all active GEs whose source matches this buff definition name
		FGameplayTagContainer Tags;
		ASC->RemoveActiveEffectsWithTags(Tags);
	}

	TriggerOutput(TEXT("Out"), true);
}
