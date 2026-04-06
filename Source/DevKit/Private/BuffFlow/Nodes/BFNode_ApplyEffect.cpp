#include "BuffFlow/Nodes/BFNode_ApplyEffect.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"

UBFNode_ApplyEffect::UBFNode_ApplyEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Effect");
#endif
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_ApplyEffect::ExecuteInput(const FName& PinName)
{
	if (!Effect)
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

	// 获取目标的 ASC（支持任何实现了 IAbilitySystemInterface 的 Actor）
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(TargetActor);
	UAbilitySystemComponent* TargetASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	// 以 BuffOwner 为来源构建 Spec，施加到目标
	FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = OwnerASC->MakeOutgoingSpec(Effect, Level, Context);
	if (!Spec.IsValid())
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	TargetASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	// bFinish=false：保持节点活跃，允许事件多次触发（如每次命中都刷新 GE_HeatBuff）
	TriggerOutput(TEXT("Out"), false);
}
