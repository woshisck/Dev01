#include "BuffFlow/Nodes/BFNode_GetRuneTuningValue.h"

#include "BuffFlow/BuffFlowComponent.h"
#include "Data/RuneDataAsset.h"
#include "FlowAsset.h"

TArray<FString> UBFNode_GetRuneTuningValue::GetPresetKeyNames()
{
	return {
		// 攻击
		TEXT("Attack.Damage"),
		TEXT("Attack.Damage.01"), TEXT("Attack.Damage.02"), TEXT("Attack.Damage.03"),
		// 燃烧
		TEXT("Burn.Damage"),
		TEXT("Burn.Damage.01"), TEXT("Burn.Damage.02"), TEXT("Burn.Damage.03"),
		TEXT("Burn.Duration"),
		TEXT("Burn.Duration.01"), TEXT("Burn.Duration.02"),
		// 中毒
		TEXT("Poison.Stack"),
		TEXT("Poison.Stack.01"), TEXT("Poison.Stack.02"),
		TEXT("Poison.Duration"),
		TEXT("Poison.Duration.01"), TEXT("Poison.Duration.02"),
		// 月光
		TEXT("Moonlight.ProjectileCount"),
		TEXT("Moonlight.ProjectileCount.01"), TEXT("Moonlight.ProjectileCount.02"),
		TEXT("Moonlight.ProjectileSpeed"),
		TEXT("Moonlight.ProjectileSpeed.01"),
		// 终结技
		TEXT("Finisher.Damage"),
		TEXT("Finisher.Damage.01"), TEXT("Finisher.Damage.02"),
		TEXT("Finisher.AOERadius"),
		TEXT("Finisher.AOERadius.01"),
		TEXT("DetonationDamage"),
		TEXT("KnockbackDistance"),
	};
}

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
	const FName ActiveKey = GetActiveKey();
	if (!BuffFlowComponent || !FlowAsset || ActiveKey.IsNone())
	{
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	URuneDataAsset* SourceRune = BuffFlowComponent->GetActiveSourceRuneData(FlowAsset);
	if (!SourceRune)
	{
		UE_LOG(LogTemp, Warning, TEXT("BFNode_GetRuneTuningValue: no source rune for Flow=%s Key=%s"),
			*GetNameSafe(FlowAsset), *ActiveKey.ToString());
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	FRuneTuningScalar Scalar;
	if (!SourceRune->GetRuneTuningScalar(ActiveKey, Scalar))
	{
		UE_LOG(LogTemp, Warning, TEXT("BFNode_GetRuneTuningValue: rune %s has no tuning key %s"),
			*GetNameSafe(SourceRune), *ActiveKey.ToString());
		TriggerOutput(TEXT("NotFound"), true);
		return;
	}

	// Use GetRuneTuningValue so Formula / MMC / Context sources are evaluated correctly,
	// not just the raw Scalar.Value which only works for Literal source.
	Value = FFlowDataPinOutputProperty_Float(SourceRune->GetRuneTuningValue(ActiveKey, DefaultValue));
	bFound = FFlowDataPinOutputProperty_Bool(true);
	TriggerOutput(TEXT("Found"), true);
}
