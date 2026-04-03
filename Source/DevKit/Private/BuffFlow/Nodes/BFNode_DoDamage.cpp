#include "BuffFlow/Nodes/BFNode_DoDamage.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"

UBFNode_DoDamage::UBFNode_DoDamage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = { FFlowPin(TEXT("In")) };
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_DoDamage::ExecuteInput(const FName& PinName)
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

	// 确定伤害量（优先 FlatDamage，否则用 LastEventContext 倍率）
	float DamageValue = FlatDamage;
	if (DamageValue <= 0.f)
	{
		if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
		{
			DamageValue = BFC->LastEventContext.DamageAmount * DamageMultiplier;
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
	Spec.Data->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(TEXT("Data.Damage")), -DamageValue);

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	TriggerOutput(TEXT("Out"), true);
}
