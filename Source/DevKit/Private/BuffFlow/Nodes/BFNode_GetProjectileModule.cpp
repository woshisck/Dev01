#include "BuffFlow/Nodes/BFNode_GetProjectileModule.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"

UBFNode_GetProjectileModule::UBFNode_GetProjectileModule(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Module");
#endif
}

void UBFNode_GetProjectileModule::ExecuteInput(const FName& PinName)
{
	Speed = FFlowDataPinOutputProperty_Float(1400.f);
	Count = FFlowDataPinOutputProperty_Int32(1);
	ConeAngleDegrees = FFlowDataPinOutputProperty_Float(0.f);
	bSweepCollision = FFlowDataPinOutputProperty_Bool(false);
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

	const bool bEnabled = SourceRune->GetActiveModules().bProjectile;
	bModuleEnabled = FFlowDataPinOutputProperty_Bool(bEnabled);

	if (bEnabled)
	{
		const FRuneProjectileModule& Proj = SourceRune->GetProjectileModule();
		Speed = FFlowDataPinOutputProperty_Float(Proj.Speed);
		Count = FFlowDataPinOutputProperty_Int32(Proj.Count);
		ConeAngleDegrees = FFlowDataPinOutputProperty_Float(Proj.ConeAngleDegrees);
		bSweepCollision = FFlowDataPinOutputProperty_Bool(Proj.bSweepCollision);
	}

	TriggerFirstOutput(true);
}
