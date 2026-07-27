#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Abilities/GA_WeaponSkillTypes.h"
#include "Data/AbilityData.h"
#include "Data/WeaponSkillDataAsset.h"
#include "Item/Weapon/WeaponDefinition.h"

namespace
{
	const TCHAR* BasicActionTags[] = {
		TEXT("Character.State.Skill.Attack.Combo1"),
		TEXT("Character.State.Skill.Attack.Combo2"),
		TEXT("Character.State.Skill.Attack.Combo3"),
		TEXT("Character.State.Skill.Attack.Combo4"),
		TEXT("Character.State.Movement.Dash"),
		TEXT("Character.State.Movement.Dash.Combo1"),
		TEXT("Character.State.Movement.Dash.Combo2"),
		TEXT("Character.State.Movement.Dash.Combo3"),
		TEXT("Character.State.Movement.Dash.Combo4"),
		TEXT("Character.State.Skill.Reload"),
		TEXT("Character.State.Equipment.SwitchWeapon"),
	};

	const TCHAR* PassiveReactionTags[] = {
		TEXT("Action.HitReact.Front"),
		TEXT("Action.HitReact.Back"),
		TEXT("Action.HitReact.Blocked"),
		TEXT("Action.HitReact.Parried"),
		TEXT("Action.HitReact.Left"),
		TEXT("Action.HitReact.Right"),
		TEXT("Action.HitReact.Heavy"),
		TEXT("Action.Stun"),
		TEXT("Action.GuardBreak"),
		TEXT("Action.Knockdown"),
		TEXT("Action.GetUp"),
		TEXT("Action.Launch"),
		TEXT("Action.Landing.Hard"),
		TEXT("Action.Execution.Victim"),
		TEXT("Action.Backstab.Victim"),
		TEXT("Action.Dead"),
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponActionAuthoringContractTest,
	"DevKit.Weapon.Authoring.ActionDataContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponActionAuthoringContractTest::RunTest(const FString& Parameters)
{
	UWeaponAttackAbilityMontageData* AttackData = NewObject<UWeaponAttackAbilityMontageData>();
	TestNotNull(TEXT("Weapon attack data can be constructed"), AttackData);
	for (const TCHAR* TagName : BasicActionTags)
	{
		const FGameplayTag ActionTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		TestTrue(FString::Printf(TEXT("%s is registered"), TagName), ActionTag.IsValid());
		TestTrue(FString::Printf(TEXT("%s is an attack-data row"), TagName), AttackData->MontageMap.Contains(ActionTag));
	}

	UWeaponPassiveAbilityMontageData* PassiveData = NewObject<UWeaponPassiveAbilityMontageData>();
	TestNotNull(TEXT("Weapon passive data can be constructed"), PassiveData);
	for (const TCHAR* TagName : PassiveReactionTags)
	{
		const FGameplayTag PassiveTag = FGameplayTag::RequestGameplayTag(FName(TagName), false);
		TestTrue(FString::Printf(TEXT("%s is registered"), TagName), PassiveTag.IsValid());
		TestTrue(FString::Printf(TEXT("%s is a passive-data row"), TagName), PassiveData->PassiveMap.Contains(PassiveTag));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponSkillAuthoringContractTest,
	"DevKit.Weapon.Authoring.SkillIdentityContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSkillAuthoringContractTest::RunTest(const FString& Parameters)
{
	struct FExpectedSkill
	{
		TSubclassOf<UGA_WeaponSkill> AbilityClass;
		TSubclassOf<UWeaponSkillDataAsset> DataAssetClass;
		const TCHAR* SkillTagName;
		int32 MontageSlotCount;
	};

	const FExpectedSkill ExpectedSkills[] = {
		{UGA_WeaponSkill_THSwordCombo::StaticClass(), UWeaponSkill_THSwordComboDataAsset::StaticClass(), TEXT("Weapon.Skill.THSwordCombo"), 4},
		{UGA_WeaponSkill_Block::StaticClass(), UWeaponSkill_BlockDataAsset::StaticClass(), TEXT("Weapon.Skill.Block"), 1},
		{UGA_WeaponSkill_Thrust::StaticClass(), UWeaponSkill_ThrustDataAsset::StaticClass(), TEXT("Weapon.Skill.Thrust"), 1},
	};

	for (const FExpectedSkill& Expected : ExpectedSkills)
	{
		const UGA_WeaponSkill* AbilityCDO = Expected.AbilityClass->GetDefaultObject<UGA_WeaponSkill>();
		TestNotNull(FString::Printf(TEXT("%s has a CDO"), *Expected.AbilityClass->GetName()), AbilityCDO);
		if (!AbilityCDO)
		{
			continue;
		}
		TestEqual(
			FString::Printf(TEXT("%s has its stable skill tag"), *Expected.AbilityClass->GetName()),
			AbilityCDO->GetWeaponSkillTag(),
			FGameplayTag::RequestGameplayTag(FName(Expected.SkillTagName), false));
		TestEqual(
			FString::Printf(TEXT("%s declares required montage slots"), *Expected.AbilityClass->GetName()),
			AbilityCDO->GetRequiredMontageSlots().Num(),
			Expected.MontageSlotCount);
		TestEqual(
			FString::Printf(TEXT("%s declares its dedicated C++ DA class"), *Expected.AbilityClass->GetName()),
			AbilityCDO->GetWeaponSkillDataClass().LoadSynchronous(),
			Expected.DataAssetClass.Get());

		UWeaponSkillDataAsset* LegacySkillAsset = NewObject<UWeaponSkillDataAsset>();
		LegacySkillAsset->AbilityClass = Expected.AbilityClass;
		TestEqual(
			FString::Printf(TEXT("%s legacy DA resolves the GA tag"), *Expected.AbilityClass->GetName()),
			LegacySkillAsset->GetResolvedSkillTag(),
			AbilityCDO->GetWeaponSkillTag());
	}
	return true;
}

#endif
