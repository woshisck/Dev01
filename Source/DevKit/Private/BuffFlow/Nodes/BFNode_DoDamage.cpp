#include "BuffFlow/Nodes/BFNode_DoDamage.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Types/FlowDataPinResults.h"

UBFNode_DoDamage::UBFNode_DoDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };

	// DamageMultiplier 默认值 1.0
	DamageMultiplier.Value = 1.f;
}

void UBFNode_DoDamage::ExecuteBuffFlowInput(const FName& PinName)
{
	if (!DamageEffect.Get())
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

	AActor* TargetActor = ResolveTarget(TargetSelector);
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

	// 解析 FlatDamage（优先数据引脚，无连线取节点默认值）
	float FlatDamageValue = FlatDamage.Value;
	FFlowDataPinResult_Float FlatResult = TryResolveDataPinAsFloat(
		GET_MEMBER_NAME_CHECKED(UBFNode_DoDamage, FlatDamage));
	if (FlatResult.Result == EFlowDataPinResolveResult::Success)
	{
		FlatDamageValue = FlatResult.Value;
	}

	// 确定伤害量（优先 FlatDamage，否则用 LastEventContext 倍率）
	float DamageValue = FlatDamageValue;
	if (DamageValue <= 0.f)
	{
		float MultiplierValue = DamageMultiplier.Value;
		FFlowDataPinResult_Float MultResult = TryResolveDataPinAsFloat(
			GET_MEMBER_NAME_CHECKED(UBFNode_DoDamage, DamageMultiplier));
		if (MultResult.Result == EFlowDataPinResolveResult::Success)
		{
			MultiplierValue = MultResult.Value;
		}

		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			DamageValue = BFC->LastEventContext.DamageAmount * MultiplierValue;
		}
	}

	FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = OwnerASC->MakeOutgoingSpec(DamageEffect, 1.f, Context);
	if (!Spec.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 将伤害值注入 SetByCaller（GE 需配置对应的 Magnitude Data Tag）
	static const FGameplayTag DataDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
	if (DataDamageTag.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(DataDamageTag, -DamageValue);
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	TriggerOutput(TEXT("Out"), true);
}
