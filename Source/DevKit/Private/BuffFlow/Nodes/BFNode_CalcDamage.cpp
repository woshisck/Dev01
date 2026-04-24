#include "BuffFlow/Nodes/BFNode_CalcDamage.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Types/FlowDataPinResults.h"

UBFNode_CalcDamage::UBFNode_CalcDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Damage");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
	BaseValue = FFlowDataPinInputProperty_Float(20.f);
}

void UBFNode_CalcDamage::ExecuteInput(const FName& PinName)
{
	AActor* TargetActor = ResolveTarget(Target);
	if (!TargetActor)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	FFlowDataPinResult_Float BaseResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_CalcDamage, BaseValue));
	const float ResolvedBase = (BaseResult.Result == EFlowDataPinResolveResult::Success)
		? BaseResult.Value : BaseValue.Value;

	float Result = 0.f;

	if (BaseMode == EBFDamageBaseMode::Flat)
	{
		Result = ResolvedBase;
	}
	else
	{
		const UBaseAttributeSet* BaseAttrSet = TargetASC->GetSet<UBaseAttributeSet>();
		if (!BaseAttrSet)
		{
			TriggerOutput(TEXT("Failed"), true);
			return;
		}

		const float MaxHP = BaseAttrSet->GetMaxHealth();
		const float CurHP = BaseAttrSet->GetHealth();

		switch (BaseMode)
		{
		case EBFDamageBaseMode::PercentMaxHP:
			Result = MaxHP * ResolvedBase;
			break;
		case EBFDamageBaseMode::PercentCurrentHP:
			Result = CurHP * ResolvedBase;
			break;
		case EBFDamageBaseMode::PercentMissingHP:
			Result = (MaxHP - CurHP) * ResolvedBase;
			break;
		default:
			break;
		}
	}

	auto ApplyMult = [&](const FGameplayTag& Tag, EBFTagConditionMode Cond, float Mult)
	{
		if (!Tag.IsValid()) return;
		const bool bHas = TargetASC->HasMatchingGameplayTag(Tag);
		const bool bApply = (Cond == EBFTagConditionMode::HasTag) ? bHas : !bHas;
		if (bApply)
			Result *= Mult;
	};
	ApplyMult(MultTag1, MultCondition1, MultValue1);
	ApplyMult(MultTag2, MultCondition2, MultValue2);
	ApplyMult(MultTag3, MultCondition3, MultValue3);

	FinalDamage = FFlowDataPinOutputProperty_Float(FMath::Max(Result, 0.f));

	TriggerOutput(TEXT("Out"), true);
}
