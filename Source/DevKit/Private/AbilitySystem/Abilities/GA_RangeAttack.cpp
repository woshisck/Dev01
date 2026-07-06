#include "AbilitySystem/Abilities/GA_RangeAttack.h"
#include "AbilitySystem/AbilityTask/YogAbilityTask_PlayMontageAndWaitForEvent.h"
#include "AbilitySystem/Abilities/GA_MeleeAttack.h"
#include "Character/YogCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/CharacterData.h"
#include "Data/AbilityData.h"
#include "Data/MontageConfigDA.h"
#include "AbilitySystemComponent.h"

UGA_RangeAttack::UGA_RangeAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	bRetriggerInstancedAbility = true;
	HitEventTag = FGameplayTag::RequestGameplayTag(FName("GameplayEvent.RangeAttack.Hit"));

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

	UAnimMontage* MontageToPlay = ResolveAttackMontage(ActorInfo);
	if (!MontageToPlay)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GA_RangeAttack] No attack montage found on weapon AttackAbilityData or AttackMontage fallback."));
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
	EventFilter.AddTag(HitEventTag);

	UYogAbilityTask_PlayMontageAndWaitForEvent* MontageTask =
		UYogAbilityTask_PlayMontageAndWaitForEvent::PlayMontageAndWaitForEvent(
			this, NAME_None, MontageToPlay, EventFilter, AttackSpeedRate, NAME_None, true, 1.f);

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
	if (EventTag == HitEventTag)
	{
		ApplyHitDamage(EventData);
	}
}

UAnimMontage* UGA_RangeAttack::ResolveAttackMontage(const FGameplayAbilityActorInfo* ActorInfo) const
{
	AYogCharacterBase* OwnerChar = Cast<AYogCharacterBase>(GetOwningActorFromActorInfo());
	UCharacterDataComponent* CDC = OwnerChar ? OwnerChar->GetCharacterDataComponent() : nullptr;
	UCharacterData* CD = CDC ? CDC->GetCharacterData() : nullptr;

	if (CD && CD->AbilityData)
	{
		static const FGameplayTag PreferredTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack"), false);

		FGameplayTag MontageKey;
		if (PreferredTag.IsValid() && AbilityTags.HasTagExact(PreferredTag))
		{
			MontageKey = PreferredTag;
		}
		if (!MontageKey.IsValid())
		{
			for (const FGameplayTag& Tag : AbilityTags) { MontageKey = Tag; break; }
		}

		if (MontageKey.IsValid())
		{
			UAnimMontage* Montage = CD->AbilityData->GetMontage(MontageKey);

			// Context-tag branch selection can override the base montage (mirrors GA_MeleeAttack).
			FGameplayTagContainer ContextTags;
			if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
			{
				ASC->GetOwnedGameplayTags(ContextTags);
			}
			if (UMontageConfigDA* Config = CD->AbilityData->GetMontageConfig(MontageKey, ContextTags))
			{
				if (Config->Montage)
				{
					Montage = Config->Montage;
				}
			}

			if (Montage)
			{
				return Montage;
			}
		}
	}

	return AttackMontage;
}

void UGA_RangeAttack::ApplyHitDamage(const FGameplayEventData& HitEventData)
{
	if (!HitEffectContainerTag.IsValid())
	{
		return;
	}

	// Spec is created NOW — attributes are read live from the ASC at this moment, so any
	// buffs acquired while the bullet was in flight are included in the damage calculation.
	const FYogGameplayEffectContainerSpec ContainerSpec =
		MakeEffectContainerSpec(HitEffectContainerTag, HitEventData);

	if (ContainerSpec.HasValidTargets() && ContainerSpec.HasValidEffects())
	{
		ApplyEffectContainerSpec(ContainerSpec);
	}
}
