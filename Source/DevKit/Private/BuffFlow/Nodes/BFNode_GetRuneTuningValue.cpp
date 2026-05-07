#include "BuffFlow/Nodes/BFNode_GetRuneTuningValue.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"

UBFNode_GetRuneTuningValue::UBFNode_GetRuneTuningValue(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Rune");
#endif
	OutputPins = { FFlowPin(TEXT("Found")), FFlowPin(TEXT("NotFound")) };
}

void UBFNode_GetRuneTuningValue::ExecuteInput(const FName& PinName)
{
	Value = FFlowDataPinOutputProperty_Float(DefaultValue);
	bFound = FFlowDataPinOutputProperty_Bool(false);

	UBuffFlowComponent* BuffFlowComponent = GetBuffFlowComponent();
	UFlowAsset* FlowAsset = GetFlowAsset();
	if (!BuffFlowComponent || !FlowAsset || Key.IsNone())
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	URuneDataAsset* SourceRune = BuffFlowComponent->GetActiveSourceRuneData(FlowAsset);
	if (!SourceRune)
	{
		UE_LOG(LogTemp, Warning, TEXT("BFNode_GetRuneTuningValue: no source rune for Flow=%s Key=%s"),
			*GetNameSafe(FlowAsset), *Key.ToString());
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	FRuneTuningScalar Scalar;
	if (!SourceRune->GetRuneTuningScalar(Key, Scalar))
	{
		UE_LOG(LogTemp, Warning, TEXT("BFNode_GetRuneTuningValue: rune %s has no tuning key %s"),
			*GetNameSafe(SourceRune), *Key.ToString());
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	Value = FFlowDataPinOutputProperty_Float(Scalar.Value);
	bFound = FFlowDataPinOutputProperty_Bool(true);
	TriggerOutput(TEXT("Found"), true);
}
