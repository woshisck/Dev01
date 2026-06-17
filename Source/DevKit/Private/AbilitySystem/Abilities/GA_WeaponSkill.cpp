#include "AbilitySystem/Abilities/GA_WeaponSkill.h"

#include "GameplayTagContainer.h"

UGA_WeaponSkill::UGA_WeaponSkill(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.WeaponSkill")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1")));
	bHoldMontageUntilInputRelease = true;
}

void UGA_WeaponSkill::ConfigureWeaponSkillComboTag(const TCHAR* ComboTagName)
{
	const FGameplayTag BroadTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"));
	const FGameplayTag Combo1Tag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"));
	const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(FName(ComboTagName));

	AbilityTags.RemoveTag(Combo1Tag);
	ActivationOwnedTags.RemoveTag(Combo1Tag);
	AbilityTags.AddTag(BroadTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(BroadTag);
	ActivationOwnedTags.AddTag(ComboTag);
}

UGA_WeaponSkill_Combo1::UGA_WeaponSkill_Combo1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1"));
}

UGA_WeaponSkill_Combo2::UGA_WeaponSkill_Combo2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"));
}

UGA_WeaponSkill_Combo3::UGA_WeaponSkill_Combo3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo3"));
}

UGA_WeaponSkill_Combo4::UGA_WeaponSkill_Combo4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo4"));
}
