#include "BuffFlow/Nodes/BFNode_AddTag.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_AddTag::UBFNode_AddTag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Tag");
#endif
}

void UBFNode_AddTag::ExecuteBuffFlowInput(const FName& PinName)
{
	AActor* TargetActor = ResolveTarget(Target);
	if (TargetActor)
	{
		UYogAbilitySystemComponent* ASC = TargetActor->FindComponentByClass<UYogAbilitySystemComponent>();
		if (ASC && Tag.IsValid())
		{
			ASC->AddGameplayTagWithCount(Tag, Count);
			TotalCountAdded += Count;
			StoredASC = ASC;
		}
	}

	TriggerOutput(TEXT("Out"), true);
}

void UBFNode_AddTag::Cleanup()
{
	if (StoredASC.IsValid() && Tag.IsValid() && TotalCountAdded > 0)
	{
		StoredASC->RemoveGameplayTagWithCount(Tag, TotalCountAdded);
	}
	TotalCountAdded = 0;
	StoredASC.Reset();
	Super::Cleanup();
}
