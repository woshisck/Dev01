#include "BuffFlow/Nodes/BFNode_CompareFloat.h"

UBFNode_CompareFloat::UBFNode_CompareFloat(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	OutputPins = { FFlowPin(TEXT("True")), FFlowPin(TEXT("False")) };
}

void UBFNode_CompareFloat::ExecuteInput(const FName& PinName)
{
	bool bResult = false;

	switch (Operator)
	{
	case EBFCompareOp::GreaterThan:    bResult = (A > B); break;
	case EBFCompareOp::GreaterOrEqual: bResult = (A >= B); break;
	case EBFCompareOp::Equal:          bResult = FMath::IsNearlyEqual(A, B); break;
	case EBFCompareOp::LessOrEqual:    bResult = (A <= B); break;
	case EBFCompareOp::LessThan:       bResult = (A < B); break;
	case EBFCompareOp::NotEqual:       bResult = !FMath::IsNearlyEqual(A, B); break;
	}

	TriggerOutput(bResult ? TEXT("True") : TEXT("False"), true);
}
