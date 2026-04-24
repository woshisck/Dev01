#include "BuffFlow/Nodes/BFNode_Probability.h"
#include "Types/FlowDataPinResults.h"

UBFNode_Probability::UBFNode_Probability(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Condition");
#endif
	Chance.Value = 0.5f;
	OutputPins = { FFlowPin(TEXT("Pass")), FFlowPin(TEXT("Fail")) };
}

void UBFNode_Probability::ExecuteInput(const FName& PinName)
{
	FFlowDataPinResult_Float Res = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_Probability, Chance));

	const float ChanceValue = FMath::Clamp(
		(Res.Result == EFlowDataPinResolveResult::Success) ? Res.Value : Chance.Value,
		0.f, 1.f);

	const bool bPass = FMath::FRand() < ChanceValue;
	TriggerOutput(bPass ? TEXT("Pass") : TEXT("Fail"), true);
}
