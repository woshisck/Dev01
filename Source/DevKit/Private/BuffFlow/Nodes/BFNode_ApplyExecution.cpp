#include "BuffFlow/Nodes/BFNode_ApplyExecution.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_ApplyExecution::UBFNode_ApplyExecution(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins  = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyExecution::ExecuteBuffFlowInput(const FName& PinName)
{
	if (!ExecCalcClass)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UYogAbilitySystemComponent* OwnerASC = GetOwnerASC();
	if (!OwnerASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

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

	// ExecCalcClass 变化时重建 GE（通常只在第一次执行时构建一次）
	if (!CachedGE || CachedExecClass != ExecCalcClass)
	{
		CachedGE = NewObject<UGameplayEffect>(GetTransientPackage(), NAME_None, RF_Transient);
		CachedGE->DurationPolicy = EGameplayEffectDurationType::Instant;

		FGameplayEffectExecutionDefinition ExecDef;
		ExecDef.CalculationClass = ExecCalcClass;
		CachedGE->Executions.Add(ExecDef);

		CachedExecClass = ExecCalcClass;
	}

	// 构建 Spec（以 BuffOwner 为 Source，Level=1）
	FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = OwnerASC->MakeOutgoingSpec(CachedGE->GetClass(), 1.f, Context);

	// 无法用 MakeOutgoingSpec + Class 直接用 CachedGE，改用 FGameplayEffectSpec 直接构造
	FGameplayEffectSpec DirectSpec(CachedGE, Context, 1.f);

	// 填写 SetByCaller 槽（Tag 非空才赋值）
	auto ApplySetByCaller = [&](const FGameplayTag& Tag, const FName& PinMemberName,
	                             const FFlowDataPinInputProperty_Float& DefaultVal)
	{
		if (!Tag.IsValid()) return;
		FFlowDataPinResult_Float Res = TryResolveDataPinAsFloat(PinMemberName);
		const float Val = (Res.Result == EFlowDataPinResolveResult::Success) ? Res.Value : DefaultVal.Value;
		DirectSpec.SetSetByCallerMagnitude(Tag, Val);
	};
	ApplySetByCaller(SetByCallerTag1, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyExecution, SetByCallerValue1), SetByCallerValue1);
	ApplySetByCaller(SetByCallerTag2, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyExecution, SetByCallerValue2), SetByCallerValue2);
	ApplySetByCaller(SetByCallerTag3, GET_MEMBER_NAME_CHECKED(UBFNode_ApplyExecution, SetByCallerValue3), SetByCallerValue3);

	FActiveGameplayEffectHandle Handle = TargetASC->ApplyGameplayEffectSpecToSelf(DirectSpec);

	if (Handle.IsValid())
	{
		TriggerOutput(TEXT("Out"), true);
	}
	else
	{
		TriggerOutput(TEXT("Failed"), true);
	}
}
