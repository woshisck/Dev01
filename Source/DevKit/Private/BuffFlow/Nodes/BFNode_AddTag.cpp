#include "BuffFlow/Nodes/BFNode_AddTag.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_AddTag::UBFNode_AddTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
}

void UBFNode_AddTag::ExecuteInput(const FName& PinName)
{
	AActor* TargetActor = ResolveTarget(Target);
	if (TargetActor)
	{
		UYogAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UYogAbilitySystemComponent>();
		if (ASC && Tag.IsValid())
		{
			ASC->AddGameplayTagWithCount(Tag, Count);
		}
	}

	TriggerOutput(TEXT("Out"), true);
}
