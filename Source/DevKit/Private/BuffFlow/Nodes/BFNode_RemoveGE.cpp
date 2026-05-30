#include "BuffFlow/Nodes/BFNode_RemoveGE.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_RemoveGE::UBFNode_RemoveGE(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Removed")), FFlowPin(TEXT("NotFound")), FFlowPin(TEXT("Failed")) };
	StacksToRemove = FFlowDataPinInputProperty_Int32(1);
}

void UBFNode_RemoveGE::ExecuteBuffFlowInput(const FName& PinName)
{
	if (!Effect)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TArray<AActor*> TargetActors = ResolveAllTargets(Target);
	if (TargetActors.IsEmpty())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	int32 StackCount = -1; // -1 = remove all stacks
	if (RemoveMode == EBFRemoveMode::OneStack)
	{
		StackCount = 1;
	}
	else if (RemoveMode == EBFRemoveMode::CustomCount)
	{
		FFlowDataPinResult_Int Res = TryResolveDataPinAsInt(GET_MEMBER_NAME_CHECKED(UBFNode_RemoveGE, StacksToRemove));
		StackCount = (Res.Result == EFlowDataPinResolveResult::Success) ? static_cast<int32>(Res.Value) : StacksToRemove.Value;
		StackCount = FMath::Max(StackCount, 1);
	}

	FGameplayEffectQuery Query;
	Query.EffectDefinition = Effect;

	int32 TotalRemoved = 0;
	bool bAnyASCFound = false;

	for (AActor* TargetActor : TargetActors)
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
		UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
		if (!TargetASC)
		{
			continue;
		}

		bAnyASCFound = true;

		if (TargetASC->GetActiveEffects(Query).IsEmpty())
		{
			continue;
		}

		TotalRemoved += TargetASC->RemoveActiveEffects(Query, StackCount);
	}

	RemovedCount = FFlowDataPinOutputProperty_Int32(TotalRemoved);

	if (!bAnyASCFound)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TriggerOutput(TotalRemoved > 0 ? TEXT("Removed") : TEXT("NotFound"), true);
}
