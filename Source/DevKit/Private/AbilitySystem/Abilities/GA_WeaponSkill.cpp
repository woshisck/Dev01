#include "AbilitySystem/Abilities/GA_WeaponSkill.h"

#include "Data/WeaponSkillDataAsset.h"
#include "GameplayTagContainer.h"

UGA_WeaponSkill::UGA_WeaponSkill(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Player.WeaponSkill")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill")));
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo1")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill")));
	ActivationOwnedTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1")));
	const FGameplayTag SharedSkillCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Cooldown.SkillShared"), false);
	if (SharedSkillCooldownTag.IsValid())
	{
		ActivationBlockedTags.AddTag(SharedSkillCooldownTag);
	}
	bHoldMontageUntilInputRelease = true;
	bResolveCombatDeck = true;
	CombatDeckActionSlot = ECombatDeckActionSlot::WeaponSkill;
	CombatDeckFlowRole = ECombatDeckFlowRole::Finisher;
	CombatDeckCommitTiming = ECombatCardTriggerTiming::OnCommit;
	bConsumeCombatDeckOnCommit = true;
	bStartsSharedSkillCooldown = true;
	SharedSkillCooldownFallbackDuration = 3.0f;
	WeaponSkillDataClass = UWeaponSkillDataAsset::StaticClass();
}

bool UGA_WeaponSkill::PrepareForInputActivation()
{
	return true;
}

const UWeaponSkillDataAsset* UGA_WeaponSkill::GetEquippedWeaponSkillData() const
{
	return Cast<UWeaponSkillDataAsset>(
		GetSourceObject(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo()));
}

void UGA_WeaponSkill::ConfigureWeaponSkillComboTag(const TCHAR* ComboTagName)
{
	const FGameplayTag CharacterBroadTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill"));
	const FGameplayTag CharacterComboTag = FGameplayTag::RequestGameplayTag(FName(ComboTagName));
	const FGameplayTag BroadTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill"));
	const FString LegacyComboTagName = FString(ComboTagName).Replace(
		TEXT("Character.State.Skill.WeaponSkill"),
		TEXT("PlayerState.AbilityCast.WeaponSkill"));
	const FGameplayTag ComboTag = FGameplayTag::RequestGameplayTag(FName(*LegacyComboTagName));
	const FGameplayTag SharedSkillCooldownTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.Cooldown.SkillShared"), false);
	const bool bIsCombo1 = FString(ComboTagName).EndsWith(TEXT(".Combo1"));

	// A dedicated multi-stage skill reuses this helper on every activation.
	// Keep exactly one stage tag so montage lookup never falls back to an
	// earlier stage that remains on the instance.
	for (int32 ComboSlot = 1; ComboSlot <= 4; ++ComboSlot)
	{
		const FGameplayTag ExistingCharacterComboTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("Character.State.Skill.WeaponSkill.Combo%d"), ComboSlot)));
		const FGameplayTag ExistingLegacyComboTag = FGameplayTag::RequestGameplayTag(
			FName(*FString::Printf(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo%d"), ComboSlot)));
		AbilityTags.RemoveTag(ExistingCharacterComboTag);
		AbilityTags.RemoveTag(ExistingLegacyComboTag);
		ActivationOwnedTags.RemoveTag(ExistingCharacterComboTag);
		ActivationOwnedTags.RemoveTag(ExistingLegacyComboTag);
	}
	if (bIsCombo1)
	{
		AbilityTags.AddTag(CharacterBroadTag);
		AbilityTags.AddTag(BroadTag);
		ActivationOwnedTags.AddTag(CharacterBroadTag);
		if (SharedSkillCooldownTag.IsValid())
		{
			ActivationBlockedTags.AddTag(SharedSkillCooldownTag);
		}
		bStartsSharedSkillCooldown = true;
	}
	else
	{
		AbilityTags.RemoveTag(CharacterBroadTag);
		AbilityTags.RemoveTag(BroadTag);
		ActivationOwnedTags.RemoveTag(CharacterBroadTag);
		ActivationOwnedTags.RemoveTag(BroadTag);
		if (SharedSkillCooldownTag.IsValid())
		{
			ActivationBlockedTags.RemoveTag(SharedSkillCooldownTag);
		}
		bStartsSharedSkillCooldown = false;
	}
	AbilityTags.AddTag(CharacterComboTag);
	AbilityTags.AddTag(ComboTag);
	ActivationOwnedTags.AddTag(CharacterComboTag);
}

UGA_WeaponSkill_Combo1::UGA_WeaponSkill_Combo1(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("Character.State.Skill.WeaponSkill.Combo1"));
}

UGA_WeaponSkill_Combo2::UGA_WeaponSkill_Combo2(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("Character.State.Skill.WeaponSkill.Combo2"));
}

UGA_WeaponSkill_Combo3::UGA_WeaponSkill_Combo3(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("Character.State.Skill.WeaponSkill.Combo3"));
}

UGA_WeaponSkill_Combo4::UGA_WeaponSkill_Combo4(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ConfigureWeaponSkillComboTag(TEXT("Character.State.Skill.WeaponSkill.Combo4"));
}
