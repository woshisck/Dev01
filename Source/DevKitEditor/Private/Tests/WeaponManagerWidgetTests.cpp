#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Abilities/GA_WeaponSkillTypes.h"
#include "Data/WeaponSkillDataAsset.h"
#include "GameplayTagContainer.h"
#include "Item/Weapon/WeaponDefinition.h"
#include "Item/Weapon/WeaponSpawner.h"
#include "Tools/WeaponManager/SWeaponManagerWidget.h"
#include "Tools/WeaponManager/WeaponDefinitionActorFactory.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponManagerWidgetContractTest,
	"DevKit.Weapon.Authoring.EditorWidgetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponManagerWidgetContractTest::RunTest(const FString& Parameters)
{
	const TSharedRef<SWeaponManagerWidget> Widget = SNew(SWeaponManagerWidget);

	TestTrue(TEXT("Weapon manager constructs all core panels"), Widget->HasCorePanelsForTesting());
	TestEqual(TEXT("Basic action table has 11 rows"), Widget->GetBasicActionRowCountForTesting(), 11);
	TestEqual(TEXT("Passive reaction table has 16 rows"), Widget->GetPassiveActionRowCountForTesting(), 16);
	TestTrue(TEXT("Native C++ skill catalogue contains the three initial skills"), Widget->GetNativeSkillTypeCountForTesting() >= 3);
	TestEqual(
		TEXT("Block skill has a Chinese display name"),
		GetDefault<UGA_WeaponSkill_Block>()->GetWeaponSkillDisplayName().ToString(),
		FString(TEXT("格挡")));
	TestFalse(
		TEXT("Block skill has a Chinese authoring description"),
		GetDefault<UGA_WeaponSkill_Block>()->GetWeaponSkillDescription().IsEmpty());
	TestEqual(
		TEXT("Thrust skill has a Chinese display name"),
		GetDefault<UGA_WeaponSkill_Thrust>()->GetWeaponSkillDisplayName().ToString(),
		FString(TEXT("突刺")));
	TestFalse(
		TEXT("Thrust skill has a Chinese authoring description"),
		GetDefault<UGA_WeaponSkill_Thrust>()->GetWeaponSkillDescription().IsEmpty());
	TestEqual(
		TEXT("Two-handed-sword combo has a Chinese display name"),
		GetDefault<UGA_WeaponSkill_THSwordCombo>()->GetWeaponSkillDisplayName().ToString(),
		FString(TEXT("双手剑连段")));
	TestFalse(
		TEXT("Two-handed-sword combo has a Chinese authoring description"),
		GetDefault<UGA_WeaponSkill_THSwordCombo>()->GetWeaponSkillDescription().IsEmpty());
	TestTrue(
		TEXT("/Game/Code/Weapon is the official weapon root"),
		SWeaponManagerWidget::IsOfficialWeaponPathForTesting(
			TEXT("/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword")));
	TestFalse(
		TEXT("WeaponDefinition assets outside the official root are test assets"),
		SWeaponManagerWidget::IsOfficialWeaponPathForTesting(
			TEXT("/Game/Developers/Test/DA_WPN_Test")));

	struct FExpectedSkillSlots
	{
		const TCHAR* TagName;
		int32 SlotCount;
	};
	const FExpectedSkillSlots ExpectedSlots[] = {
		{TEXT("Weapon.Skill.Block"), 1},
		{TEXT("Weapon.Skill.THSwordCombo"), 4},
		{TEXT("Weapon.Skill.Thrust"), 1},
	};

	for (const FExpectedSkillSlots& Expected : ExpectedSlots)
	{
		const FGameplayTag SkillTag = FGameplayTag::RequestGameplayTag(FName(Expected.TagName), false);
		TestTrue(FString::Printf(TEXT("%s is registered"), Expected.TagName), SkillTag.IsValid());
		TestEqual(
			FString::Printf(TEXT("%s exposes only its GA-declared montage slots"), Expected.TagName),
			Widget->GetRequiredMontageSlotCountForTesting(SkillTag),
			Expected.SlotCount);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTHSwordWeaponSkillAssetMigrationContractTest,
	"DevKit.Weapon.Authoring.THSwordAssetMigrationContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTHSwordWeaponSkillAssetMigrationContractTest::RunTest(const FString& Parameters)
{
	const UWeaponDefinition* Weapon = LoadObject<UWeaponDefinition>(
		nullptr,
		TEXT("/Game/Code/Weapon/TwoHandedSword/DA_WPN_THSword.DA_WPN_THSword"));
	if (!TestNotNull(TEXT("Two-handed-sword weapon definition loads"), Weapon))
	{
		return false;
	}

	UWeaponDefinitionActorFactory* PlacementFactory =
		NewObject<UWeaponDefinitionActorFactory>(GetTransientPackage());
	PlacementFactory->Configure(const_cast<UWeaponDefinition*>(Weapon));
	TestTrue(
		TEXT("Weapon drag factory places the shared WeaponSpawner actor class"),
		PlacementFactory->NewActorClass
			&& PlacementFactory->NewActorClass->IsChildOf(AWeaponSpawner::StaticClass()));
	if (PlacementFactory->NewActorClass)
	{
		TestEqual(
			TEXT("Weapon drag factory uses the shared BP_WeaponSpawner class"),
			PlacementFactory->NewActorClass->GetPathName(),
			FString(TEXT("/Game/Code/Weapon/BP_WeaponSpawner.BP_WeaponSpawner_C")));
	}
	FText PlacementError;
	const bool bCanPlaceWeapon =
		PlacementFactory->CanCreateActorFrom(FAssetData(Weapon), PlacementError);
	TestTrue(
		FString::Printf(TEXT("Configured two-handed sword can be dragged into a level: %s"), *PlacementError.ToString()),
		bCanPlaceWeapon);

	TestEqual(TEXT("Weapon exposes exactly the two migrated skills"), Weapon->AvailableWeaponSkills.Num(), 2);

	const UWeaponSkillDataAsset* DefaultSkill = Weapon->DefaultWeaponSkill;
	if (TestNotNull(TEXT("Weapon has an explicit default skill"), DefaultSkill))
	{
		TestTrue(
			TEXT("Default skill uses the dedicated two-handed-sword combo DA type"),
			DefaultSkill->IsA<UWeaponSkill_THSwordComboDataAsset>());
		TestTrue(
			TEXT("Default skill uses the dedicated two-handed-sword combo GA"),
			DefaultSkill->AbilityClass == UGA_WeaponSkill_THSwordCombo::StaticClass());
	}

	const FGameplayTag ComboTag =
		FGameplayTag::RequestGameplayTag(TEXT("Weapon.Skill.THSwordCombo"), false);
	const FGameplayTag BlockTag =
		FGameplayTag::RequestGameplayTag(TEXT("Weapon.Skill.Block"), false);
	const UWeaponSkillDataAsset* ComboSkill = nullptr;
	const UWeaponSkillDataAsset* BlockSkill = nullptr;

	for (const UWeaponSkillDataAsset* Skill : Weapon->AvailableWeaponSkills)
	{
		if (!Skill)
		{
			AddError(TEXT("AvailableWeaponSkills contains a null entry"));
			continue;
		}

		const FGameplayTag SkillTag = Skill->GetResolvedSkillTag();
		if (SkillTag == ComboTag)
		{
			ComboSkill = Skill;
		}
		else if (SkillTag == BlockTag)
		{
			BlockSkill = Skill;
		}
		else
		{
			AddError(FString::Printf(
				TEXT("Unexpected weapon skill tag on %s: %s"),
				*Skill->GetPathName(),
				*SkillTag.ToString()));
		}
	}

	if (TestNotNull(TEXT("Combo skill is present"), ComboSkill))
	{
		TestTrue(
			TEXT("Combo skill uses its dedicated DA type"),
			ComboSkill->IsA<UWeaponSkill_THSwordComboDataAsset>());
		TestTrue(
			TEXT("Combo skill uses its dedicated GA"),
			ComboSkill->AbilityClass == UGA_WeaponSkill_THSwordCombo::StaticClass());
		TestNotNull(TEXT("Combo skill keeps its montage/action data"), ComboSkill->AbilityData.Get());
		if (ComboSkill->AbilityData)
		{
			TestEqual(
				TEXT("Combo skill keeps the existing two-handed-sword skill data"),
				ComboSkill->AbilityData->GetPathName(),
				FString(TEXT("/Game/Docs/Data/Character/ExplicitActions/DA_Skill_THSword.DA_Skill_THSword")));
		}
	}

	if (TestNotNull(TEXT("Block skill is present"), BlockSkill))
	{
		TestTrue(
			TEXT("Block skill uses its dedicated DA type"),
			BlockSkill->IsA<UWeaponSkill_BlockDataAsset>());
		TestTrue(
			TEXT("Block skill uses its dedicated GA"),
			BlockSkill->AbilityClass == UGA_WeaponSkill_Block::StaticClass());
		TestNotNull(TEXT("Block skill keeps its montage/action data"), BlockSkill->AbilityData.Get());
		if (BlockSkill->AbilityData)
		{
			TestEqual(
				TEXT("Block skill keeps the existing guard skill data"),
				BlockSkill->AbilityData->GetPathName(),
				FString(TEXT("/Game/Docs/Data/Character/ExplicitActions/DA_WeaponSkill_Guard.DA_WeaponSkill_Guard")));
		}
	}

	return true;
}

#endif
