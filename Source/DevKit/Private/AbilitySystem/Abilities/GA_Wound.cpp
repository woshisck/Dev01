#include "AbilitySystem/Abilities/GA_Wound.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"

// =====================================================================
// GA_Wound is currently DISABLED — being replaced by a pure GameplayEffect
// approach (tag check inside DamageExecution + GE-driven extra damage).
// Class shell is kept so Blueprints / GE_Wound_Marker.GrantedAbilities
// references still load. All trigger registration + behavior bodies are
// wrapped in `#if 0 ... #endif` and can be revived if the GE-only design
// turns out to be insufficient.
// =====================================================================

UGA_Wound::UGA_Wound(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

#if 0 // GA_Wound disabled — trigger registration suppressed
	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Wound"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
#endif
}

void UGA_Wound::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// GA_Wound disabled — immediately end if it somehow gets activated
	// (e.g., a stale BP still has the trigger configured at the BP level).
	EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	return;

#if 0 // Original behavior — replaced by pure GE approach
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
#endif
}

void UGA_Wound::OnDamageTaken(FGameplayEventData Payload)
{
#if 0 // GA_Wound disabled — wound damage is now expected to come from a GE path
	if (bIsApplyingWoundDamage) return;
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

	TGuardValue<bool> ReentrancyGuard(bIsApplyingWoundDamage, true);
	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

	if (InstigatorASC.IsValid())
	{
		InstigatorASC->LogDamageDealt(GetAvatarActorFromActorInfo(), ExtraDamage, FName("Wound"));
	}
#endif
}

void UGA_Wound::OnWoundedTagChanged(const FGameplayTag Tag, int32 NewCount)
{
#if 0 // GA_Wound disabled — no tag listener was registered
	if (NewCount <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
#endif
}

void UGA_Wound::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
#if 0 // GA_Wound disabled — no tag listener was registered, nothing to clean up
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayTag WoundedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Wounded"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(
			WoundedTag, EGameplayTagEventType::NewOrRemoved).Remove(TagChangeDelegateHandle);
	}
#endif
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
