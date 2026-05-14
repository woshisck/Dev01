#include "BuffFlow/Nodes/BFNode_OnDamageDealt.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "BuffFlow/BuffFlowComponent.h"
#include "Character/YogCharacterBase.h"

UBFNode_OnDamageDealt::UBFNode_OnDamageDealt(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITOR
	Category = TEXT("BuffFlow|Trigger");
#endif
	InputPins = { FFlowPin(TEXT("In")), FFlowPin(TEXT("Stop")) };
	OutputPins = { FFlowPin(TEXT("OnDamage")) };
}

void UBFNode_OnDamageDealt::ExecuteBuffFlowInput(const FName& PinName)
{
	UYogAbilitySystemComponent* ASC = GetOwnerASC();

	if (PinName == TEXT("In"))
	{
		if (ASC)
		{
			BoundASC = ASC;
			// RemoveDynamic first: 防止 DealtDamage 广播期间 Flow 重入导致重复绑定（触发 Invocation List ensure）
			ASC->DealtDamage.RemoveDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
			ASC->DealtDamage.AddDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
		}
	}
	else if (PinName == TEXT("Stop"))
	{
		if (BoundASC.IsValid())
		{
			BoundASC->DealtDamage.RemoveDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
			BoundASC.Reset();
		}
	}
}

void UBFNode_OnDamageDealt::HandleDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage)
{
	if (bOncePerSwing)
	{
		if (GFrameCounter == LastTriggeredFrame)
			return;
		LastTriggeredFrame = GFrameCounter;
	}

	CachedDamage = Damage;
	LastDamageOutput.Value = Damage;

	// 填充事件上下文：自己是攻击者，TargetASC 是被击者
	if (UBuffFlowComponent* BFC = GetBuffFlowComponent())
	{
		BFC->LastEventContext.DamageCauser   = BFC->GetBuffOwner();
		BFC->LastEventContext.DamageReceiver = TargetASC ? TargetASC->GetAvatarActor() : nullptr;
		BFC->LastEventContext.DamageAmount   = Damage;

		UE_LOG(LogTemp, Warning, TEXT("[OnDamageDealt] Attacker=%s | Target=%s | Damage=%.1f"),
			*GetNameSafe(BFC->GetBuffOwner()),
			*GetNameSafe(BFC->LastEventContext.DamageReceiver.Get()),
			Damage);
	}

	// Trigger output but do NOT finish - keep listening
	TriggerOutput(TEXT("OnDamage"), false);
}

void UBFNode_OnDamageDealt::Cleanup()
{
	if (BoundASC.IsValid())
	{
		BoundASC->DealtDamage.RemoveDynamic(this, &UBFNode_OnDamageDealt::HandleDamageDealt);
		BoundASC.Reset();
	}

	Super::Cleanup();
}
