#include "BuffFlow/Nodes/BFNode_CompareInt.h"
#include "Types/FlowDataPinResults.h"

UBFNode_CompareInt::UBFNode_CompareInt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	OutputPins = { FFlowPin(TEXT("True")), FFlowPin(TEXT("False")) };
}

void UBFNode_CompareInt::ExecuteBuffFlowInput(const FName& PinName)
{
	auto ResolvePin = [this](const FName& MemberName, const FFlowDataPinInputProperty_Int32& LocalValue) -> int32
	{
		FFlowDataPinResult_Int Result = TryResolveDataPinAsInt(MemberName);
		if (Result.Result == EFlowDataPinResolveResult::Success)
			return static_cast<int32>(Result.Value);
		return LocalValue.Value;
	};

	const int32 ValA = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_CompareInt, A), A);
	const int32 ValB = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_CompareInt, B), B);

	bool bResult = false;
	switch (Operator)
	{
	case EBFCompareOp::GreaterThan:    bResult = (ValA > ValB);  break;
	case EBFCompareOp::GreaterOrEqual: bResult = (ValA >= ValB); break;
	case EBFCompareOp::Equal:          bResult = (ValA == ValB); break;
	case EBFCompareOp::LessOrEqual:    bResult = (ValA <= ValB); break;
	case EBFCompareOp::LessThan:       bResult = (ValA < ValB);  break;
	case EBFCompareOp::NotEqual:       bResult = (ValA != ValB); break;
	}

	TriggerOutput(bResult ? TEXT("True") : TEXT("False"), true);
}