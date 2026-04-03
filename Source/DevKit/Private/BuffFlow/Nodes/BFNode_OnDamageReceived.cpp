#include "BuffFlow/Nodes/BFNode_OnDamageReceived.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_OnDamageReceived::UBFNode_OnDamageReceived(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnDamage")) };
}

void UBFNode_OnDamageReceived::ExecuteInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			ASC->ReceivedDamage.AddDynamic(this, &UBFNode_OnDamageReceived::HandleDamageReceived);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->ReceivedDamage.RemoveDynamic(this, &UBFNode_OnDamageReceived::HandleDamageReceived);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnDamageReceived::HandleDamageReceived(UYogAbilitySystemComponent* SourceASC, float Damage)
{
	CachedDamage = Damage;

	// 填充事件上下文：SourceASC 是攻击者，自己是被击者
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->LastEventContext.DamageCauser   = SourceASC ? SourceASC->GetAvatarActor() : nullptr;
		BFC->LastEventContext.DamageReceiver = BFC->GetBuffOwner();
		BFC->LastEventContext.DamageAmount   = Damage;
	}

	TriggerOutput(TEXT("OnDamage"), false);
}

void UBFNode_OnDamageReceived::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->ReceivedDamage.RemoveDynamic(this, &UBFNode_OnDamageReceived::HandleDamageReceived);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
