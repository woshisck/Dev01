#include "AbilitySystem/Abilities/GA_ActiveSkill_ShieldBurst.h"

#include "AbilitySystem/Abilities/GA_Knockback.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayTagContainer.h"
#include "Projectile/SlashWaveProjectile.h"

namespace
{
bool TryAppendProjectileDirectionFromContext(
	FGameplayEventData& Payload,
	const FGameplayEffectContextHandle& DamageContext)
{
	auto TryAppendFromObject = [&Payload](const UObject* Object) -> bool
	{
		const ASlashWaveProjectile* Projectile = Cast<ASlashWaveProjectile>(Object);
		if (!Projectile)
		{
			return false;
		}

		FVector ProjectileDirection = FVector::ZeroVector;
		if (!Projectile->TryGetKnockbackDirectionOverride(ProjectileDirection))
		{
			return false;
		}

		Payload.OptionalObject = Projectile;
		UGA_Knockback::AppendAttackDirectionTargetData(Payload, ProjectileDirection, Projectile);
		return true;
	};

	if (!DamageContext.IsValid())
	{
		return false;
	}

	if (TryAppendFromObject(DamageContext.GetSourceObject()))
	{
		return true;
	}

	if (TryAppendFromObject(DamageContext.GetEffectCauser()))
	{
		return true;
	}

	return TryAppendFromObject(DamageContext.GetInstigator());
}
}

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

	DealtDamageWithContextHandle = SourceASC->DealtDamageWithContext.AddUObject(
		this,
		&UGA_ActiveSkill_ShieldBurst::HandlePlayerDamageDealtWithContext);

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
	HandlePlayerDamageDealtWithContext(TargetASC, Damage, FGameplayEffectContextHandle());
}

FGameplayEventData UGA_ActiveSkill_ShieldBurst::MakeKnockbackPayload(
	AActor* InSourceActor,
	AActor* TargetActor,
	float InKnockbackDistance,
	const FGameplayEffectContextHandle& DamageContext)
{
	FGameplayEventData Payload;
	Payload.EventTag = FGameplayTag::RequestGameplayTag(TEXT("Action.Knockback"), false);
	Payload.Instigator = InSourceActor;
	Payload.Target = TargetActor;
	Payload.EventMagnitude = InKnockbackDistance;
	TryAppendProjectileDirectionFromContext(Payload, DamageContext);
	return Payload;
}

void UGA_ActiveSkill_ShieldBurst::HandlePlayerDamageDealtWithContext(
	UYogAbilitySystemComponent* TargetASC,
	float Damage,
	const FGameplayEffectContextHandle& DamageContext)
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

	FGameplayEventData Payload = MakeKnockbackPayload(SourceActor, TargetActor, KnockbackDistance, DamageContext);
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
		if (DealtDamageWithContextHandle.IsValid())
		{
			SourceASC->DealtDamageWithContext.Remove(DealtDamageWithContextHandle);
			DealtDamageWithContextHandle.Reset();
		}
	}
	SourceASC = nullptr;
	SourceActor = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
