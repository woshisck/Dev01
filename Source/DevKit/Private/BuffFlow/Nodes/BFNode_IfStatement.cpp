#include "BuffFlow/Nodes/BFNode_IfStatement.h"
#include "Types/FlowDataPinResults.h"

UBFNode_IfStatement::UBFNode_IfStatement(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	OutputPins = { FFlowPin(TEXT("True")), FFlowPin(TEXT("False")) };
}

void UBFNode_IfStatement::ExecuteInput(const FName& PinName)
{
	FFlowDataPinResult_Bool Result = TryResolveDataPinAsBool(GET_MEMBER_NAME_CHECKED(UBFNode_IfStatement, Condition));

	const bool bCondition = (Result.Result == EFlowDataPinResolveResult::Success)
		? Result.Value
		: Condition.Value;

	TriggerOutput(bCondition ? TEXT("True") : TEXT("False"), true);
}
