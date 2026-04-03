#include "BuffFlow/Nodes/BFNode_AddBuff.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Data/YogBuffDefinition.h"
#include "GameplayEffect.h"

UBFNode_AddBuff::UBFNode_AddBuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OutputPins = { FFlowPin(TEXT("Out")), FFlowPin(TEXT("Failed")) };
}

void UBFNode_AddBuff::ExecuteInput(const FName& PinName)
{
	if (!BuffDefinition)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	AActor* TargetActor = ResolveTarget(Target);
	UAbilitySystemComponent* ASC = nullptr;
	if (TargetActor)
	{
		ASC = TargetActor->FindComponentByClass<UAbilitySystemComponent>();
	}

	if (!ASC)
	{
		TriggerOutput(TEXT("Failed"), true);
		return;
	}

	UGameplayEffect* TransientGE = BuffDefinition->CreateTransientGE(GetTransientPackage());
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpec Spec(TransientGE, Context, static_cast<float>(Level));
	FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	if (Handle.IsValid())
	{
		TriggerOutput(TEXT("Out"), true);
	}
	else
	{
		TriggerOutput(TEXT("Failed"), true);
	}
}
