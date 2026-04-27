// Copyright https://github.com/MothCocoon/FlowGraph/graphs/contributors

#include "Nodes/Route/FlowNode_Branch.h"
#include "AddOns/FlowNodeAddOn_PredicateAND.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(FlowNode_Branch)

UFlowNode_Branch::UFlowNode_Branch(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("Route|Logic");
	NodeDisplayStyle = FlowNodeStyle::Logic;
#endif
	InputPins.Empty();
	InputPins.Add(FFlowPin(FName(TEXT("Evaluate"))));

	OutputPins.Empty();
	OutputPins.Add(FFlowPin(FName(TEXT("True"))));
	OutputPins.Add(FFlowPin(FName(TEXT("False"))));

	AllowedSignalModes = {EFlowSignalMode::Enabled, EFlowSignalMode::Disabled};
}

EFlowAddOnAcceptResult UFlowNode_Branch::AcceptFlowNodeAddOnChild_Implementation(const UFlowNodeAddOn* AddOnTemplate, const TArray<UFlowNodeAddOn*>& AdditionalAddOnsToAssumeAreChildren) const
{
	if (IFlowPredicateInterface::ImplementsInterfaceSafe(AddOnTemplate))
	{
		return EFlowAddOnAcceptResult::TentativeAccept;
	}

	return Super::AcceptFlowNodeAddOnChild_Implementation(AddOnTemplate, AdditionalAddOnsToAssumeAreChildren);
}

void UFlowNode_Branch::ExecuteInput(const FName& PinName)
{
	const bool bResult = UFlowNodeAddOn_PredicateAND::EvaluatePredicateAND(AddOns);
	TriggerOutput(bResult ? FName(TEXT("True")) : FName(TEXT("False")), true);
}
