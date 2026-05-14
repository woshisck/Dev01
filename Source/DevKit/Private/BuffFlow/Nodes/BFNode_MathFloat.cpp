#include "BuffFlow/Nodes/BFNode_MathFloat.h"
#include "Types/FlowDataPinResults.h"

UBFNode_MathFloat::UBFNode_MathFloat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Math");
#endif
	OutputPins = { FFlowPin(TEXT("Out")) };
}

void UBFNode_MathFloat::ExecuteBuffFlowInput(const FName& PinName)
{
	auto ResolvePin = [this](const FName& MemberName, const FFlowDataPinInputProperty_Float& LocalValue) -> float
	{
		FFlowDataPinResult_Float Res = TryResolveDataPinAsFloat(MemberName);
		return (Res.Result == EFlowDataPinResolveResult::Success) ? Res.Value : LocalValue.Value;
	};

	const float ValA = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_MathFloat, A), A);
	const float ValB = ResolvePin(GET_MEMBER_NAME_CHECKED(UBFNode_MathFloat, B), B);

	float Out = 0.f;
	switch (Operator)
	{
	case EBFMathOp::Add:      Out = ValA + ValB; break;
	case EBFMathOp::Subtract: Out = ValA - ValB; break;
	case EBFMathOp::Multiply: Out = ValA * ValB; break;
	case EBFMathOp::Divide:   Out = (FMath::Abs(ValB) > SMALL_NUMBER) ? (ValA / ValB) : 0.f; break;
	}

	Result = FFlowDataPinOutputProperty_Float(Out);
	TriggerOutput(TEXT("Out"), true);
}
