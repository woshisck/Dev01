#include "AbilitySystem/Abilities/GA_ApplyFinisherMark.h"

#include "AbilitySystemComponent.h"

static const FGameplayTag TAG_Action_Mark_Apply_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Action.Mark.Apply.Finisher"));

static const FGameplayTag TAG_Buff_Status_Mark_Finisher =
	FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Mark.Finisher"));

UGA_ApplyFinisherMark::UGA_ApplyFinisherMark(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::NonInstanced;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = TAG_Action_Mark_Apply_Finisher;
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

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!ASC->HasMatchingGameplayTag(TAG_Buff_Status_Mark_Finisher))
	{
		FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(FinisherMarkGEClass, GetAbilityLevel());
		if (SpecHandle.IsValid())
		{
			ApplyGameplayEffectSpecToOwner(Handle, ActorInfo, ActivationInfo, SpecHandle);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
