#include "AbilitySystem/Abilities/GA_Rend.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"

UGA_Rend::UGA_Rend(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag    = FGameplayTag::RequestGameplayTag(TEXT("Buff.Event.Rend"));
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Rend::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC || !ActorInfo->AvatarActor.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	DamagePerTrigger = (TriggerEventData && TriggerEventData->EventMagnitude > 0.f)
		? TriggerEventData->EventMagnitude : DefaultDamagePerTrigger;

	AccumulatedDistance = 0.f;
	StationaryTimer     = 0.f;
	LastPosition        = ActorInfo->AvatarActor->GetActorLocation();

	if (TriggerEventData && TriggerEventData->Instigator)
	{
		InstigatorASC = Cast<UYogAbilitySystemComponent>(
			TriggerEventData->Instigator->FindComponentByClass<UAbilitySystemComponent>());
	}

	// 监听 Buff.Status.Rended 消失
	const FGameplayTag RendedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Rended"));
	TagChangeDelegateHandle = ASC->RegisterGameplayTagEvent(
		RendedTag, EGameplayTagEventType::NewOrRemoved)
		.AddUObject(this, &UGA_Rend::OnRendTagChanged);

	// 启动 Tick 计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TickTimerHandle, this, &UGA_Rend::RendTick,
			TickInterval, true, TickInterval);
	}
}

void UGA_Rend::RendTick()
{
	if (!CurrentActorInfo || !CurrentActorInfo->AvatarActor.IsValid()) return;

	const FVector CurrentPos = CurrentActorInfo->AvatarActor->GetActorLocation();
	const float Delta = FVector::Dist2D(CurrentPos, LastPosition);
	LastPosition = CurrentPos;

	if (Delta < 1.f) // 静止判定
	{
		StationaryTimer += TickInterval;
		if (StationaryTimer >= StationaryTimeout)
		{
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
			return;
		}
	}
	else
	{
		StationaryTimer = 0.f;
		AccumulatedDistance += Delta;
	}

	// 每累计 DamagePerUnits 单位触发一次伤害
	if (AccumulatedDistance >= DamagePerUnits && RendDamageEffect)
	{
		const int32 Triggers = FMath::FloorToInt(AccumulatedDistance / DamagePerUnits);
		AccumulatedDistance -= Triggers * DamagePerUnits;
		const float TotalDamage = Triggers * DamagePerTrigger;

		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC)
		{
			FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
			FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(RendDamageEffect, 1.f, Ctx);
			if (Spec.IsValid())
			{
				static const FGameplayTag DataTag = FGameplayTag::RequestGameplayTag(TEXT("Data.Damage"), false);
				if (DataTag.IsValid())
				{
					Spec.Data->SetSetByCallerMagnitude(DataTag, TotalDamage);
				}
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());

				if (InstigatorASC.IsValid())
				{
					InstigatorASC->LogDamageDealt(GetAvatarActorFromActorInfo(), TotalDamage, FName("Rend"));
				}
			}
		}
	}
}

void UGA_Rend::OnRendTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount <= 0)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
	}
}

void UGA_Rend::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TickTimerHandle);
	}
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		const FGameplayTag RendedTag = FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.Rended"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(
			RendedTag, EGameplayTagEventType::NewOrRemoved).Remove(TagChangeDelegateHandle);
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
