#include "Component/ComboRuntimeComponent.h"

#include "Component/CombatDeckComponent.h"

UComboRuntimeComponent::UComboRuntimeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UComboRuntimeComponent::SetWeaponSkillAbility(TSubclassOf<UYogGameplayAbility> InAbility)
{
	WeaponSkillAbility = InAbility;
}

void UComboRuntimeComponent::EnsureWeaponComboAbilitiesGranted(APlayerCharacterBase* PlayerOwner)
{
}

bool UComboRuntimeComponent::TryActivateCombo(ECardRequiredAction InputAction, APlayerCharacterBase* PlayerOwner)
{
	return false;
}

bool UComboRuntimeComponent::TryActivateAttack(APlayerCharacterBase* PlayerOwner)
{
	return false;
}

bool UComboRuntimeComponent::TryActivateWeaponSkill(APlayerCharacterBase* PlayerOwner)
{
	return false;
}

bool UComboRuntimeComponent::TryActivateDash(APlayerCharacterBase* PlayerOwner)
{
	return false;
}

bool UComboRuntimeComponent::TryActivateSkill(APlayerCharacterBase* PlayerOwner)
{
	return false;
}

bool UComboRuntimeComponent::TryActivateSpecial(APlayerCharacterBase* PlayerOwner)
{
	return TryActivateSkill(PlayerOwner);
}

bool UComboRuntimeComponent::TryActivateSkillCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner)
{
	return false;
}

bool UComboRuntimeComponent::TryActivateSpecialCombo(TSubclassOf<UYogGameplayAbility> AbilityClass, APlayerCharacterBase* PlayerOwner)
{
	return TryActivateSkillCombo(AbilityClass, PlayerOwner);
}

void UComboRuntimeComponent::ResetCombo()
{
}

void UComboRuntimeComponent::ClearRuntimeCombatLooseTags()
{
}

void UComboRuntimeComponent::RegisterActiveAttackAbility(const FGuid& AttackGuid)
{
}

bool UComboRuntimeComponent::HandleAttackAbilityEnded(const FGuid& EndedAttackGuid)
{
	return false;
}

FCombatDeckActionContext UComboRuntimeComponent::BuildAttackContext(
	ECombatCardTriggerTiming TriggerTiming,
	APlayerCharacterBase* PlayerOwner) const
{
	FCombatDeckActionContext Context;
	Context.TriggerTiming = TriggerTiming;
	return Context;
}
