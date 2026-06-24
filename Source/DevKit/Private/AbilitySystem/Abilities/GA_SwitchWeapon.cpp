#include "AbilitySystem/Abilities/GA_SwitchWeapon.h"

#include "AbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "GameplayTagContainer.h"

UGA_SwitchWeapon::UGA_SwitchWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	const FGameplayTag SwitchWeaponTag =
		FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.SwitchWeapon"));
	const FGameplayTag CharacterSwitchWeaponTag =
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Equipment.SwitchWeapon"));

	AbilityTags.AddTag(CharacterSwitchWeaponTag);
	AbilityTags.AddTag(SwitchWeaponTag);
	ActivationOwnedTags.AddTag(CharacterSwitchWeaponTag);
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast")));
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill")));
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash")));
	CancelAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Equipment.SwitchWeapon")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Dead")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.HitReact")));
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Buff.Knockback")));
	ActivationBlockedTags.AddTag(CharacterSwitchWeaponTag);
	ActivationBlockedTags.AddTag(SwitchWeaponTag);
	bListenForComboWindow = false;
}

bool UGA_SwitchWeapon::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const APlayerCharacterBase* Player =
		ActorInfo ? Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	return Player && Player->CanSwitchWeapon();
}

void UGA_SwitchWeapon::PreActivate(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	FOnGameplayAbilityEnded::FDelegate* OnGameplayAbilityEndedDelegate,
	const FGameplayEventData* TriggerEventData)
{
	const APlayerCharacterBase* Player =
		ActorInfo ? Cast<APlayerCharacterBase>(ActorInfo->AvatarActor.Get()) : nullptr;
	bSwitchStartedInRecoveryWindow = Player && Player->IsInPostAttackRecoveryWindow();

	const FGameplayTagContainer SavedCancelTags = CancelAbilitiesWithTag;
	CancelAbilitiesWithTag.Reset();

	Super::PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);

	CancelAbilitiesWithTag = SavedCancelTags;

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC || SavedCancelTags.IsEmpty())
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> HandlesToCancel;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (!Spec.IsActive() || !Spec.Ability || Spec.Handle == Handle)
		{
			continue;
		}

		if (Spec.Ability->AbilityTags.HasAny(SavedCancelTags))
		{
			HandlesToCancel.Add(Spec.Handle);
		}
	}

	for (const FGameplayAbilitySpecHandle& CancelHandle : HandlesToCancel)
	{
		ASC->CancelAbilityHandle(CancelHandle);
	}
}

void UGA_SwitchWeapon::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	bWeaponSwitchedThisActivation = false;

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UGA_SwitchWeapon::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);

	bSwitchStartedInRecoveryWindow = false;
}

void UGA_SwitchWeapon::HandleMontageEnded(bool bWasCancelled)
{
	if (bWasCancelled || bWeaponSwitchedThisActivation)
	{
		return;
	}

	if (APlayerCharacterBase* Player = Cast<APlayerCharacterBase>(GetAvatarActorFromActorInfo()))
	{
		if (Player->CanSwitchWeapon())
		{
			Player->SwitchWeapon(bSwitchStartedInRecoveryWindow);
			bWeaponSwitchedThisActivation = true;
		}
	}
}
