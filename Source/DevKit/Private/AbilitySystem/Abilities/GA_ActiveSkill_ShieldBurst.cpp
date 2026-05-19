#include "AbilitySystem/Abilities/GA_ActiveSkill_ShieldBurst.h"

#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"

UGA_ActiveSkill_ShieldBurst::UGA_ActiveSkill_ShieldBurst(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.ActiveSkill.ShieldBurst"), false));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Status.ActiveSkill.ShieldBurst"), false));
}

void UGA_ActiveSkill_ShieldBurst::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	SourceASC = ActorInfo ? Cast<UYogAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get()) : nullptr;
	SourceActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!SourceASC || !SourceActor)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	SourceASC->DealtDamage.AddUniqueDynamic(this, &UGA_ActiveSkill_ShieldBurst::HandlePlayerDamageDealt);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			BuffTimerHandle,
			this,
			&UGA_ActiveSkill_ShieldBurst::FinishBuff,
			FMath::Max(0.01f, BuffDuration),
			false);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UGA_ActiveSkill_ShieldBurst::HandlePlayerDamageDealt(UYogAbilitySystemComponent* TargetASC, float Damage)
{
	if (!SourceActor || !TargetASC || Damage <= 0.0f)
	{
		return;
	}

	AActor* TargetActor = TargetASC->GetAvatarActor();
	if (!TargetActor || TargetActor == SourceActor)
	{
		return;
	}

	static const FGameplayTag KnockbackTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
	if (!KnockbackTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.Instigator = SourceActor;
	Payload.Target = TargetActor;
	Payload.EventMagnitude = KnockbackDistance;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, KnockbackTag, Payload);
}

void UGA_ActiveSkill_ShieldBurst::FinishBuff()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_ActiveSkill_ShieldBurst::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(BuffTimerHandle);
	}

	if (SourceASC)
	{
		SourceASC->DealtDamage.RemoveDynamic(this, &UGA_ActiveSkill_ShieldBurst::HandlePlayerDamageDealt);
	}
	SourceASC = nullptr;
	SourceActor = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
