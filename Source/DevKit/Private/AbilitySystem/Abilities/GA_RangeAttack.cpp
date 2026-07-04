#include "AbilitySystem/Abilities/GA_RangeAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "AbilitySystem/Attribute/BaseAttributeSet.h"
#include "Animation/AN_FireProjectile.h"
#include "Character/PlayerCharacterBase.h"
#include "Character/YogCharacterBase.h"
#include "Data/RangedProjectileDefinition.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Projectile/BuffFlowProjectile.h"
#include "Projectile/YogBulletManagerSubsystem.h"

UGA_RangeAttack::UGA_RangeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
	FireEventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.RangeAttack.Fire"));
	HitEventTag  = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.RangeAttack.Hit"));

	const FGameplayTag AttackTag          = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack"));
	const FGameplayTag CharacterAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"));
	AbilityTags.AddTag(CharacterAttackTag);
	AbilityTags.AddTag(AttackTag);
	ActivationOwnedTags.AddTag(CharacterAttackTag);
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.HitReact")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Knockback")));
}

void UGA_RangeAttack::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!AttackMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_RangeAttack] No AttackMontage configured."));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	ApplyStatBeforeATKGE(ActorInfo);

	float AttackSpeedRate = 1.0f;
	if (UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get())
	{
		if (UGA_MeleeAttack::TryConsumeJustComboBonus(ASC))
		{
			constexpr float JustComboSpeedMultiplier = 1.2f;
			AttackSpeedRate *= JustComboSpeedMultiplier;
			UE_LOG(LogTemp, Verbose, TEXT("[JustCombo] Range attack consumed JustCombo bonus. Rate=%.2f"), AttackSpeedRate);
			ApplyJustComboGE(ActorInfo);
		}
	}

	FGameplayTagContainer EventFilter;
	EventFilter.AddTag(FireEventTag);
	EventFilter.AddTag(ResolveHitEventTag(ResolveProjectileDefinition(ActorInfo)));

	UYogAbilityTask_PlayMontageAndWaitForEvent* MontageTask =
		UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
			this, NAME_None, AttackMontage, EventFilter, AttackSpeedRate, NAME_None, true, 1.f);

	MontageTask->OnCompleted.AddDynamic(this, &UGA_RangeAttack::OnMontageCompleted);
	MontageTask->OnBlendOut.AddDynamic(this, &UGA_RangeAttack::OnMontageBlendOut);
	MontageTask->OnInterrupted.AddDynamic(this, &UGA_RangeAttack::OnMontageInterrupted);
	MontageTask->OnCancelled.AddDynamic(this, &UGA_RangeAttack::OnMontageCancelled);
	MontageTask->EventReceived.AddDynamic(this, &UGA_RangeAttack::OnEventReceived);

	MontageTask->ReadyForActivation();
}

void UGA_RangeAttack::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	ApplyStatAfterATKGE(ActorInfo, bWasCancelled, GetAbilityActionData());
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_RangeAttack::OnMontageCompleted(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
	if (IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, false);
	}
}

void UGA_RangeAttack::OnMontageBlendOut(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
	// Informational only; OnCompleted handles the end.
}

void UGA_RangeAttack::OnMontageInterrupted(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
	if (IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
	}
}

void UGA_RangeAttack::OnMontageCancelled(FGameplayTag /*EventTag*/, FGameplayEventData /*EventData*/)
{
	if (IsActive())
	{
		EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(), true, true);
	}
}

void UGA_RangeAttack::OnEventReceived(FGameplayTag EventTag, FGameplayEventData EventData)
{
	if (EventTag == FireEventTag)
	{
		SpawnProjectile(EventData, GetCurrentActorInfo());
	}
	else if (EventTag == ResolveHitEventTag(ResolveProjectileDefinition(GetCurrentActorInfo())))
	{
		ApplyHitDamage(EventData);
	}
}

const URangedProjectileDefinition* UGA_RangeAttack::ResolveProjectileDefinition(
	const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!ActorInfo)
	{
		return nullptr;
	}

	const APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get());
	if (!Player)
	{
		Player = Cast<APlayerCharacterBase>(ActorInfo->OwnerActor.Get());
	}

	const UWeaponDefinition* WeaponDefinition = Player ? Player->EquippedWeaponDef.Get() : nullptr;
	return WeaponDefinition ? WeaponDefinition->RangedProjectileDefinition.Get() : nullptr;
}

TSubclassOf<ABuffFlowProjectile> UGA_RangeAttack::ResolveProjectileClass(
	const URangedProjectileDefinition* ProjectileDefinition) const
{
	if (ProjectileDefinition && ProjectileDefinition->ProjectileClass)
	{
		return ProjectileDefinition->ProjectileClass;
	}

	return ProjectileClass;
}

FBuffFlowProjectileRuntimeConfig UGA_RangeAttack::ResolveProjectileConfig(
	const URangedProjectileDefinition* ProjectileDefinition) const
{
	return ProjectileDefinition ? ProjectileDefinition->ProjectileConfig : ProjectileConfig;
}

FGameplayTag UGA_RangeAttack::ResolveHitEventTag(const URangedProjectileDefinition* ProjectileDefinition) const
{
	if (ProjectileDefinition && ProjectileDefinition->HitEventTag.IsValid())
	{
		return ProjectileDefinition->HitEventTag;
	}

	return HitEventTag;
}

FGameplayTag UGA_RangeAttack::ResolveHitEffectContainerTag(
	const URangedProjectileDefinition* ProjectileDefinition) const
{
	if (ProjectileDefinition && ProjectileDefinition->HitEffectContainerTag.IsValid())
	{
		return ProjectileDefinition->HitEffectContainerTag;
	}

	return HitEffectContainerTag;
}

bool UGA_RangeAttack::ShouldUseBulletManager(const URangedProjectileDefinition* ProjectileDefinition) const
{
	return ProjectileDefinition ? ProjectileDefinition->bUseBulletManager : bUseBulletManager;
}

float UGA_RangeAttack::ComputeProjectileMagnitude(
	UAbilitySystemComponent* SourceASC,
	const FBuffFlowProjectileRuntimeConfig& RuntimeConfig) const
{
	if (!SourceASC)
	{
		return RuntimeConfig.BaseEffectMagnitude;
	}

	const float Attack      = SourceASC->GetNumericAttribute(UBaseAttributeSet::GetAttackAttribute());
	const float AttackPower = SourceASC->GetNumericAttribute(UBaseAttributeSet::GetAttackPowerAttribute());

	return RuntimeConfig.BaseEffectMagnitude
		+ Attack      * RuntimeConfig.CreatorAttackMagnitudeScale
		+ AttackPower * RuntimeConfig.CreatorAttackPowerMagnitudeScale;
}

void UGA_RangeAttack::SpawnProjectile(const FGameplayEventData& FireEventData,
	const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo)
	{
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	if (!AvatarActor)
	{
		return;
	}

	UWorld* World = AvatarActor->GetWorld();
	if (!World)
	{
		return;
	}

	const URangedProjectileDefinition* ProjectileDefinition = ResolveProjectileDefinition(ActorInfo);
	const TSubclassOf<ABuffFlowProjectile> ResolvedProjectileClass =
		ResolveProjectileClass(ProjectileDefinition);
	const FGameplayTag ResolvedHitEventTag = ResolveHitEventTag(ProjectileDefinition);
	FBuffFlowProjectileRuntimeConfig RuntimeConfig = ResolveProjectileConfig(ProjectileDefinition);

	FTransform SpawnTransform = AvatarActor->GetActorTransform();
	if (const UAN_FireProjectile* FireNotify = Cast<UAN_FireProjectile>(FireEventData.OptionalObject))
	{
		if (!FireNotify->MuzzleSocketName.IsNone())
		{
			if (const AYogCharacterBase* Character = Cast<AYogCharacterBase>(AvatarActor))
			{
				if (USkeletalMeshComponent* Mesh = Character->GetMesh())
				{
					SpawnTransform = Mesh->GetSocketTransform(FireNotify->MuzzleSocketName);
				}
			}
		}
	}

	if (ShouldUseBulletManager(ProjectileDefinition))
	{
		UYogBulletManagerSubsystem* BulletMgr = World->GetSubsystem<UYogBulletManagerSubsystem>();
		if (!BulletMgr)
		{
			return;
		}

		UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();

		FYogBulletSpawnParams Params;
		Params.SpawnLocation         = SpawnTransform.GetLocation();
		Params.Direction             = SpawnTransform.GetRotation().GetForwardVector();
		Params.Speed                 = RuntimeConfig.Speed;
		Params.Lifetime              = FMath::Max(0.01f, RuntimeConfig.Lifetime);
		// Use the largest box extent side as the sphere radius.
		Params.CollisionRadius       = RuntimeConfig.CollisionBoxExtent.GetMax();
		Params.bPiercing             = !RuntimeConfig.bDestroyOnHitTrigger;
		Params.MaxHits               = 0;
		Params.HitEventTag           = ResolvedHitEventTag;
		Params.ExpireEventTag        = RuntimeConfig.ExpireGameplayEventTag;
		Params.bSendHitEventToCreator = RuntimeConfig.bSendTriggerEventToCreator;
		Params.EffectMagnitude       = ComputeProjectileMagnitude(SourceASC, RuntimeConfig);
		Params.SourceCharacter       = Cast<ACharacter>(AvatarActor);
		Params.SourceASC             = SourceASC;
		Params.HitNiagaraSystem      = nullptr;
		Params.ExpireNiagaraSystem   = nullptr;

		BulletMgr->SpawnBullet(Params);
		return;
	}

	if (!ResolvedProjectileClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner      = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ABuffFlowProjectile* Projectile = World->SpawnActor<ABuffFlowProjectile>(
		ResolvedProjectileClass, SpawnTransform, SpawnParams);

	if (!Projectile)
	{
		return;
	}

	// Override the two fields that enable the live-capture pattern:
	//   - TriggerGameplayEventTag → routes hit notification back to this ability
	//   - EffectClass = null → GA_RangeAttack creates the damage spec at hit time (live attributes)
	RuntimeConfig.TriggerGameplayEventTag    = ResolvedHitEventTag;
	RuntimeConfig.bSendTriggerEventToCreator = true;
	RuntimeConfig.EffectClass                = nullptr;

	Projectile->InitBuffFlowProjectile(AvatarActor, RuntimeConfig);
}

void UGA_RangeAttack::ApplyHitDamage(const FGameplayEventData& HitEventData)
{
	const FGameplayTag ResolvedHitEffectContainerTag =
		ResolveHitEffectContainerTag(ResolveProjectileDefinition(GetCurrentActorInfo()));

	if (!ResolvedHitEffectContainerTag.IsValid())
	{
		return;
	}

	// Spec is created NOW — attributes are read live from the ASC at this moment, so any
	// buffs acquired while the bullet was in flight are included in the damage calculation.
	const FYogGameplayEffectContainerSpec ContainerSpec =
		MakeEffectContainerSpec(ResolvedHitEffectContainerTag, HitEventData);

	if (ContainerSpec.HasValidTargets() && ContainerSpec.HasValidEffects())
	{
		ApplyEffectContainerSpec(ContainerSpec);
	}
}
