#include "AbilitySystem/Abilities/GA_KnockbackDebuff.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"

UGA_KnockbackDebuff::UGA_KnockbackDebuff(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.KnockbackDebuff"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_KnockbackDebuff::ActivateAbility(
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

	// 监听 Buff.Status.KnockbackDebuff 消失 → 结束 GA
	const FGameplayTag KBDebuffTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.KnockbackDebuff"));
	TagChangeDelegateHandle = ASC->RegisterGameplayTagEvent(
		KBDebuffTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGA_KnockbackDebuff::OnKnockbackDebuffTagChanged);

	// 监听受击事件
	const FGameplayTag DamagedTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Event.Damaged"));
	UAbilityTask_WaitGameplayEvent* WaitTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, DamagedTag, nullptr, false, true);
	WaitTask->EventReceived.AddDynamic(this, &UGA_KnockbackDebuff::OnDamageTaken);
	WaitTask->ReadyForActivation();
}

void UGA_KnockbackDebuff::OnDamageTaken(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor) return;

	// 触发击退（GA_Knockback 监听 Action.Knockback 自动激活）
	static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
	if (KnockbackTag.IsValid())
	{
		FGameplayEventData KBPayload;
		KBPayload.Instigator = Payload.Instigator; // 攻击者，用于计算击退方向
		KBPayload.Target     = AvatarActor;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(AvatarActor, KnockbackTag, KBPayload);
	}

	// 若目标有护甲：额外扣护甲（15%攻击伤害）
	static const FGameplayTag ArmoredTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Armored"), false);
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ArmoredTag.IsValid() && ASC->HasMatchingGameplayTag(ArmoredTag) && ArmorDamageEffect)
	{
		const float ArmorDamage = Payload.EventMagnitude * ArmorDamagePct;
		if (ArmorDamage > 0.f)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ArmorDamageEffect, 1.f, Ctx);
			if (Spec.IsValid())
			{
				static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
				if (DataTag.IsValid())
				{
					Spec.Data->SetSetByCallerMagnitude(DataTag, ArmorDamage);
				}
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}
}

void UGA_KnockbackDebuff::OnKnockbackDebuffTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_KnockbackDebuff::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayTag KBDebuffTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.KnockbackDebuff"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(
			KBDebuffTag, EGameplayTagEventType::NewOrRemoved).Remove(TagChangeDelegateHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
