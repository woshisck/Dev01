#include "BuffFlow/Nodes/BFNode_GetAuraModule.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"

UBFNode_GetAuraModule::UBFNode_GetAuraModule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Module");
#endif
}

void UBFNode_GetAuraModule::ExecuteBuffFlowInput(const FName& PinName)
{
	Length = FFlowDataPinOutputProperty_Float(520.f);
	Width = FFlowDataPinOutputProperty_Float(220.f);
	Height = FFlowDataPinOutputProperty_Float(120.f);
	Duration = FFlowDataPinOutputProperty_Float(3.f);
	TickInterval = FFlowDataPinOutputProperty_Float(1.f);
	bModuleEnabled = FFlowDataPinOutputProperty_Bool(false);

	UBuffFlowComponent* BFC = GetBuffFlowComponent();
	UFlowAsset* FlowAsset = GetFlowAsset();
	if (!BFC || !FlowAsset)
	{
		TriggerFirstOutput(true);
		return;
	}

	const URuneDataAsset* SourceRune = BFC->GetActiveSourceRuneData(FlowAsset);
	if (!SourceRune)
	{
		TriggerFirstOutput(true);
		return;
	}

	const bool bEnabled = SourceRune->GetActiveModules().bAura;
	bModuleEnabled = FFlowDataPinOutputProperty_Bool(bEnabled);

	if (bEnabled)
	{
		const FRuneAuraModule& Aura = SourceRune->GetAuraModule();
		Length = FFlowDataPinOutputProperty_Float(Aura.Length);
		Width = FFlowDataPinOutputProperty_Float(Aura.Width);
		Height = FFlowDataPinOutputProperty_Float(Aura.Height);
		Duration = FFlowDataPinOutputProperty_Float(Aura.Duration);
		TickInterval = FFlowDataPinOutputProperty_Float(Aura.TickInterval);
	}

	TriggerFirstOutput(true);
}
