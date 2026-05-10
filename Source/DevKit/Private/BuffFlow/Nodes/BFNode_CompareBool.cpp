#include "BuffFlow/Nodes/BFNode_CompareBool.h"

#include "Types/FlowDataPinResults.h"

UBFNode_CompareBool::UBFNode_CompareBool(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	OutputPins = { FFlowPin(TEXT("True")), FFlowPin(TEXT("False")) };
}

void UBFNode_CompareBool::ExecuteInput(const FName& PinName)
{
	auto ResolvePin = [this](const FName& MemberName, const FFlowDataPinInputProperty_Bool& LocalValue) -> bool
	{
		FFlowDataPinResult_Bool Result = TryResolveDataPinAsBool(MemberName);
		if (Result.Result == EFlowDataPinResolveResult::Success)
		{
			return Result.Value;
		}
		return LocalValue.Value;
	};

	const bool bA = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_CompareBool, A), A);
	const bool bB = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_CompareBool, B), B);

	bool bResult = false;
	switch (Operator)
	{
	case EBFBoolCompareOp::Equal:
		bResult = (bA == bB);
		break;
	case EBFBoolCompareOp::NotEqual:
		bResult = (bA != bB);
		break;
	}

	TriggerOutput(bResult ? TEXT("True") : TEXT("False"), true);
}
