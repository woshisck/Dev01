#include "BuffFlow/Nodes/BFNode_MathInt.h"
#include "Types/FlowDataPinResults.h"

UBFNode_MathInt::UBFNode_MathInt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Math");
#endif
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_MathInt::ExecuteInput(const FName& PinName)
{
	auto ResolvePin = [this](const FName& MemberName, const FFlowDataPinInputProperty_Int32& LocalValue) -> int32
	{
		FFlowDataPinResult_Int Res = TryResolveDataPinAsInt(MemberName);
		return (Res.Result == EFlowDataPinResolveResult::Success) ? static_cast<int32>(Res.Value) : LocalValue.Value;
	};

	const int32 ValA = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_MathInt, A), A);
	const int32 ValB = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_MathInt, B), B);

	int32 Out = 0;
	switch (Operator)
	{
	case EBFMathOp::Add:      Out = ValA + ValB; break;
	case EBFMathOp::Subtract: Out = ValA - ValB; break;
	case EBFMathOp::Multiply: Out = ValA * ValB; break;
	case EBFMathOp::Divide:   Out = (ValB != 0) ? (ValA / ValB) : 0; break;
	}

	Result = FFlowDataPinOutputProperty_Int32(Out);
	TriggerOutput(TEXT("Out"), true);
}
