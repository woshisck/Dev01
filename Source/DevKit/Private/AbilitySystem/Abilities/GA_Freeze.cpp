#include "AbilitySystem/Abilities/GA_Freeze.h"
#include "AbilitySystemComponent.h"

UGA_Freeze::UGA_Freeze(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Freeze"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Freeze::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!ASC || !Avatar)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	OriginLocation = Avatar->GetActorLocation();
	PenaltyDamage  = (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
		? TriggerEventData->EventMagnitude : DefaultPenaltyDamage;

	// 立即施加减速
	if (ChillEffect)
	{
		FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(ChillEffect, 1.f, Ctx);
		if (Spec.IsValid())
		{
			ChillHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	// FreezeDuration 后检查
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			CheckTimerHandle, this, &UGA_Freeze::OnFreezeTimeExpired,
			FreezeDuration, false);
	}
}

void UGA_Freeze::OnFreezeTimeExpired()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = CurrentActorInfo ? CurrentActorInfo->AvatarActor.Get() : nullptr;

	// 移除减速
	if (ASC && ChillHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ChillHandle);
		ChillHandle = FActiveGameplayEffectHandle();
	}

	// 距离判断：逃脱失败 → 冻结眩晕 + 惩罚
	if (Avatar && ASC && FrozenStunEffect)
	{
		const float DistFromOrigin = FVector::Dist2D(Avatar->GetActorLocation(), OriginLocation);
		if (DistFromOrigin < RequiredDistance)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(FrozenStunEffect, 1.f, Ctx);
			if (Spec.IsValid())
			{
				// 惩罚伤害通过 SetByCaller 传入
				static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
				if (DataTag.IsValid())
				{
					Spec.Data->SetSetByCallerMagnitude(DataTag, PenaltyDamage);
				}
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_Freeze::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CheckTimerHandle);
	}

	// 确保减速 GE 被移除
	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (ASC && ChillHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ChillHandle);
		ChillHandle = FActiveGameplayEffectHandle();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
