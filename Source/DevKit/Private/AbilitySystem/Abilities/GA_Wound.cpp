#include "AbilitySystem/Abilities/GA_Wound.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

UGA_Wound::UGA_Wound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Wound"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Wound::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ExtraDamage = (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
		? TriggerEventData->EventMagnitude : DefaultExtraDamage;

	// 缓存发起者 ASC
	if (TriggerEventData && TriggerEventData->Instigator)
	{
		if (UYogAbilitySystemComponent* YASC = Cast<UYogAbilitySystemComponent>(
			TriggerEventData->Instigator->FindComponentByClass<UAbilitySystemComponent>()))
		{
			InstigatorASC = YASC;
		}
	}

	// 监听 Buff.Status.Wounded 消失 → 结束 GA
	const FGameplayTag WoundedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Wounded"));
	TagChangeDelegateHandle = ASC->RegisterGameplayTagEvent(
		WoundedTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGA_Wound::OnWoundedTagChanged);

	// 监听受击事件（Ability.Event.Damaged 由 DamageAttributeSet 广播给受伤角色）
	const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"));
	UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, DamagedTag, nullptr, false, true);
	WaitTask->EventReceived.AddDynamic(this, &UGA_Wound::OnDamageTaken);
	WaitTask->ReadyForActivation();
}

void UGA_Wound::OnDamageTaken(FGameplayEventData Payload)
{
	if (!WoundDamageEffect || ExtraDamage <= 0.f) return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(WoundDamageEffect, 1.f, Context);
	if (!Spec.IsValid()) return;

	// 用 SetByCaller 传入额外伤害值
	static const FGameplayTag DataDamageTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
	if (DataDamageTag.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(DataDamageTag, ExtraDamage);
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	if (InstigatorASC.IsValid())
	{
		InstigatorASC->LogDamageDealt(GetAvatarActorFromActorInfo(), ExtraDamage, FName("Wound"));
	}
}

void UGA_Wound::OnWoundedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Wound::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayTag WoundedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Wounded"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(
			WoundedTag, EGameplayTagEventType::NewOrRemoved).Remove(TagChangeDelegateHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
