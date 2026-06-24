#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Animation/AnimMontage.h"
#include "Data/AbilityData.h"
#include "GameplayTagContainer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityDataCharacterStateFallbackFindsLegacyPlayerStateMontageTest,
	"DevKit.GameplayTags.AbilityDataCharacterStateFallbackFindsLegacyPlayerStateMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityDataCharacterStateFallbackFindsLegacyPlayerStateMontageTest::RunTest(const FString& Parameters)
{
	UAbilityData* AbilityData = NewObject<UAbilityData>();
	UAnimMontage* Montage = NewObject<UAnimMontage>();
	const FGameplayTag LegacyTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Attack.Combo1"), false);
	const FGameplayTag CharacterTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo1"), false);

	TestTrue(TEXT("legacy attack combo tag exists"), LegacyTag.IsValid());
	TestTrue(TEXT("character attack combo tag exists"), CharacterTag.IsValid());
	AbilityData->MontageMap.Add(LegacyTag, Montage);

	TestEqual(TEXT("Character.State lookup falls back to legacy PlayerState montage"), AbilityData->GetMontage(CharacterTag), Montage);
	TestTrue(TEXT("Character.State HasAbility falls back to legacy PlayerState montage"), AbilityData->HasAbility(CharacterTag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityDataLegacyPlayerStateFallbackFindsCharacterStateMontageTest,
	"DevKit.GameplayTags.AbilityDataLegacyPlayerStateFallbackFindsCharacterStateMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityDataLegacyPlayerStateFallbackFindsCharacterStateMontageTest::RunTest(const FString& Parameters)
{
	UAbilityData* AbilityData = NewObject<UAbilityData>();
	UAnimMontage* Montage = NewObject<UAnimMontage>();
	const FGameplayTag CharacterTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo2"), false);
	const FGameplayTag LegacyTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.WeaponSkill.Combo2"), false);

	TestTrue(TEXT("character weapon-skill combo tag exists"), CharacterTag.IsValid());
	TestTrue(TEXT("legacy weapon-skill combo tag exists"), LegacyTag.IsValid());
	AbilityData->MontageMap.Add(CharacterTag, Montage);

	TestEqual(TEXT("legacy PlayerState lookup falls back to Character.State montage"), AbilityData->GetMontage(LegacyTag), Montage);
	TestTrue(TEXT("legacy PlayerState HasAbility falls back to Character.State montage"), AbilityData->HasAbility(LegacyTag));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityDataLegacyDashAliasFallbackFindsCharacterDashMontageTest,
	"DevKit.GameplayTags.AbilityDataLegacyDashAliasFallbackFindsCharacterDashMontage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityDataLegacyDashAliasFallbackFindsCharacterDashMontageTest::RunTest(const FString& Parameters)
{
	UAbilityData* AbilityData = NewObject<UAbilityData>();
	UAnimMontage* Montage = NewObject<UAnimMontage>();
	const FGameplayTag CharacterTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Movement.Dash"), false);
	const FGameplayTag LegacyDash1Tag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.Dash.Dash1"), false);
	const FGameplayTag LegacyDashAtkTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.DashAtk"), false);

	TestTrue(TEXT("character dash tag exists"), CharacterTag.IsValid());
	TestTrue(TEXT("legacy dash dash1 tag exists"), LegacyDash1Tag.IsValid());
	TestTrue(TEXT("legacy dash attack tag exists"), LegacyDashAtkTag.IsValid());
	AbilityData->MontageMap.Add(CharacterTag, Montage);

	TestEqual(TEXT("legacy Dash.Dash1 lookup falls back to Character.State.Movement.Dash montage"), AbilityData->GetMontage(LegacyDash1Tag), Montage);
	TestEqual(TEXT("legacy DashAtk lookup falls back to Character.State.Movement.Dash montage"), AbilityData->GetMontage(LegacyDashAtkTag), Montage);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAbilityDataLegacyLightHeavyAliasesFallbackToFormalCombatTagsTest,
	"DevKit.GameplayTags.AbilityDataLegacyLightHeavyAliasesFallbackToFormalCombatTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityDataLegacyLightHeavyAliasesFallbackToFormalCombatTagsTest::RunTest(const FString& Parameters)
{
	UAbilityData* AbilityData = NewObject<UAbilityData>();
	UAnimMontage* AttackMontage = NewObject<UAnimMontage>();
	UAnimMontage* WeaponSkillMontage = NewObject<UAnimMontage>();
	const FGameplayTag CharacterAttackTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.Attack.Combo2"), false);
	const FGameplayTag CharacterWeaponSkillTag = FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo3"), false);
	const FGameplayTag LegacyLightTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.LightAtk.Combo2"), false);
	const FGameplayTag LegacyHeavyTag = FGameplayTag::RequestGameplayTag(TEXT("PlayerState.AbilityCast.HeavyAtk.Combo3"), false);

	TestTrue(TEXT("character attack combo tag exists"), CharacterAttackTag.IsValid());
	TestTrue(TEXT("character weapon-skill combo tag exists"), CharacterWeaponSkillTag.IsValid());
	TestTrue(TEXT("legacy light attack combo tag exists"), LegacyLightTag.IsValid());
	TestTrue(TEXT("legacy heavy attack combo tag exists"), LegacyHeavyTag.IsValid());
	AbilityData->MontageMap.Add(CharacterAttackTag, AttackMontage);
	AbilityData->MontageMap.Add(CharacterWeaponSkillTag, WeaponSkillMontage);

	TestEqual(TEXT("legacy LightAtk lookup falls back to Character.State.Skill.Attack montage"), AbilityData->GetMontage(LegacyLightTag), AttackMontage);
	TestEqual(TEXT("legacy HeavyAtk lookup falls back to Character.State.Skill.WeaponSkill montage"), AbilityData->GetMontage(LegacyHeavyTag), WeaponSkillMontage);

	return true;
}

#endif
