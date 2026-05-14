#include "BuffFlow/Nodes/BFNode_CompareFloat.h"
#include "Types/FlowDataPinResults.h"

UBFNode_CompareFloat::UBFNode_CompareFloat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	OutputPins = { FFlowPin(TEXT("True")), FFlowPin(TEXT("False")) };
}

void UBFNode_CompareFloat::ExecuteBuffFlowInput(const FName& PinName)
{
	// 优先从连接的数据引脚读取，若无连接则使用节点上直接编辑的值
	auto ResolvePin = [this](const FName& MemberName, const FFlowDataPinInputProperty_Float& LocalValue) -> float
	{
		FFlowDataPinResult_Float Result = TryResolveDataPinAsFloat(MemberName);
		if (Result.Result == EFlowDataPinResolveResult::Success)
			return Result.Value;
		return LocalValue.Value;
	};

	const float ValA = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_CompareFloat, A), A);
	const float ValB = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_CompareFloat, B), B);

	bool bResult = false;
	switch (Operator)
	{
	case EBFCompareOp::GreaterThan:    bResult = (ValA > ValB); break;
	case EBFCompareOp::GreaterOrEqual: bResult = (ValA >= ValB); break;
	case EBFCompareOp::Equal:          bResult = FMath::IsNearlyEqual(ValA, ValB); break;
	case EBFCompareOp::LessOrEqual:    bResult = (ValA <= ValB); break;
	case EBFCompareOp::LessThan:       bResult = (ValA < ValB); break;
	case EBFCompareOp::NotEqual:       bResult = !FMath::IsNearlyEqual(ValA, ValB); break;
	}

	TriggerOutput(bResult ? TEXT("True") : TEXT("False"), true);
}
