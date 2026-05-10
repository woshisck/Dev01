#include "AbilitySystem/Abilities/GA_ApplyFinisherMark.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

namespace
{
FGameplayTag GetGAApplyFinisherMarkActionTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("Action.Mark.Apply.Finisher"));
}

FGameplayTag GetGAApplyFinisherMarkBuffTag()
{
	return FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Mark.Finisher"));
}
}

UGA_ApplyFinisherMark::UGA_ApplyFinisherMark(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// InstancedPerExecution 避免 NonInstanced 共享 CurrentActorInfo 导致的断言崩溃
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = GetGAApplyFinisherMarkActionTag();
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_ApplyFinisherMark::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!FinisherMarkGEClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_ApplyFinisherMark] FinisherMarkGEClass is not configured."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// TriggerEventData.Target 是被命中的敌人（由 FA 发送事件时写入 Payload 目标）
	AActor* TargetActor = TriggerEventData ? const_cast<AActor*>(TriggerEventData->Target.Get()) : nullptr;
	UAbilitySystemComponent* TargetASC = TargetActor
		? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor)
		: nullptr;

	if (!TargetASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_ApplyFinisherMark] Target has no ASC. Target=%s"), *GetNameSafe(TargetActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!TargetASC->HasMatchingGameplayTag(GetGAApplyFinisherMarkBuffTag()))
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(FinisherMarkGEClass, GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			// 将印记施加到敌人（Target），而非玩家自身
			TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get(), FPredictionKey());
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
