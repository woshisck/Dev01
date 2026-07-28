#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Abilities/GA_WeaponSkillTypes.h"
#include "AbilitySystem/YogAbilitySystemComponent.h"
#include "Character/PlayerCharacterBase.h"
#include "Component/CharacterDataComponent.h"
#include "Data/AbilityData.h"
#include "Data/CharacterData.h"
#include "Data/WeaponSkillDataAsset.h"
#include "Engine/World.h"
#include "Item/Weapon/WeaponDefinition.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWeaponSkillContainerTest,
	"DevKit.Combat.WeaponSkill.ContainerAndDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSkillContainerTest::RunTest(const FString& Parameters)
{
	UWeaponDefinition* Weapon = NewObject<UWeaponDefinition>();
	UWeaponSkillDataAsset* Block = NewObject<UWeaponSkillDataAsset>(Weapon);
	UWeaponSkillDataAsset* Thrust = NewObject<UWeaponSkillDataAsset>(Weapon);
	UWeaponSkillDataAsset* Incomplete = NewObject<UWeaponSkillDataAsset>(Weapon);
	UWeaponSkillDataAsset* Unsupported = NewObject<UWeaponSkillDataAsset>(Weapon);

	Block->AbilityClass = UGA_WeaponSkill_Block::StaticClass();
	Thrust->AbilityClass = UGA_WeaponSkill_Thrust::StaticClass();
	Block->AbilityData = NewObject<UWeaponSkillAbilityMontageData>(Block);
	Thrust->AbilityData = NewObject<UWeaponSkillAbilityMontageData>(Thrust);
	Incomplete->AbilityClass = UGA_WeaponSkill_Block::StaticClass();
	Weapon->AvailableWeaponSkills = { Block, Thrust };
	Weapon->DefaultWeaponSkill = Thrust;

	TestTrue(TEXT("Listed block skill is compatible"), Weapon->CanEquipWeaponSkill(Block));
	TestTrue(TEXT("Listed thrust skill is compatible"), Weapon->CanEquipWeaponSkill(Thrust));
	TestFalse(TEXT("Unlisted skill is rejected"), Weapon->CanEquipWeaponSkill(Unsupported));
	TestEqual(TEXT("Explicit listed default wins"), Weapon->ResolveDefaultWeaponSkill(), Thrust);

	Weapon->DefaultWeaponSkill = Unsupported;
	TestEqual(TEXT("Invalid default falls back to first valid entry"), Weapon->ResolveDefaultWeaponSkill(), Block);

	Weapon->AvailableWeaponSkills = { Incomplete, Thrust };
	TestFalse(TEXT("Listed skill without AbilityData is invalid"), Weapon->CanEquipWeaponSkill(Incomplete));
	TestEqual(TEXT("Fallback skips incomplete entries"), Weapon->ResolveDefaultWeaponSkill(), Thrust);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FUnarmedWeaponSkillGrantTest,
	"DevKit.Combat.WeaponSkill.UnarmedGrantAndReplacement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUnarmedWeaponSkillGrantTest::RunTest(const FString& Parameters)
{
	UWorld* World = GWorld;
	TestNotNull(TEXT("Automation world exists"), World);
	if (!World)
	{
		return false;
	}

	APlayerCharacterBase* Player = World->SpawnActor<APlayerCharacterBase>();
	TestNotNull(TEXT("Player spawned"), Player);
	if (!Player)
	{
		return false;
	}

	UWeaponDefinition* UnarmedWeapon = NewObject<UWeaponDefinition>(Player);
	UWeaponSkillDataAsset* Block = NewObject<UWeaponSkillDataAsset>(UnarmedWeapon);
	UWeaponSkillDataAsset* Thrust = NewObject<UWeaponSkillDataAsset>(UnarmedWeapon);
	Block->AbilityClass = UGA_WeaponSkill_Block::StaticClass();
	Thrust->AbilityClass = UGA_WeaponSkill_Thrust::StaticClass();
	Block->AbilityData = NewObject<UWeaponSkillAbilityMontageData>(Block);
	Thrust->AbilityData = NewObject<UWeaponSkillAbilityMontageData>(Thrust);
	UnarmedWeapon->AvailableWeaponSkills = { Block, Thrust };
	UnarmedWeapon->DefaultWeaponSkill = Block;

	UCharacterData* RuntimeCharacterData = NewObject<UCharacterData>(Player);
	RuntimeCharacterData->AbilityData = NewObject<UAbilityData>(RuntimeCharacterData);
	Player->GetCharacterDataComponent()->SetCharacterData(RuntimeCharacterData);
	Player->DefaultUnarmedWeaponDef = UnarmedWeapon;
	Player->ResetToDefaultUnarmedCombatState();

	TestNull(TEXT("Unarmed loadout does not occupy the real primary weapon slot"), Player->EquippedWeaponDef.Get());
	TestEqual(TEXT("Unarmed loadout selects its default weapon skill"), Player->GetEquippedWeaponSkill(), Block);
	TestTrue(TEXT("Unarmed selected skill is treated as equipped"), Player->HasEquippedWeaponSkill());
	TestNotNull(TEXT("Unarmed skill data is merged into runtime CharacterData"), RuntimeCharacterData->AbilityData.Get());

	UYogAbilitySystemComponent* ASC = Player->GetASC();
	TestNotNull(TEXT("Player ability system exists"), ASC);
	const FGameplayAbilitySpecHandle BlockHandle = Player->EquippedWeaponSkillAbilityHandle;
	TestTrue(TEXT("Unarmed skill receives an exact ability handle"), BlockHandle.IsValid());
	const FGameplayAbilitySpec* BlockSpec = ASC ? ASC->FindAbilitySpecFromHandle(BlockHandle) : nullptr;
	TestNotNull(TEXT("Granted block spec exists"), BlockSpec);
	TestEqual(TEXT("Granted block spec keeps the block DA as SourceObject"),
		BlockSpec ? BlockSpec->SourceObject.Get() : nullptr,
		static_cast<UObject*>(Block));

	TestTrue(TEXT("Unarmed skill can be replaced through the same equip API"), Player->EquipWeaponSkill(Thrust));
	TestEqual(TEXT("Replacement selects thrust"), Player->GetEquippedWeaponSkill(), Thrust);
	TestTrue(TEXT("Outgoing block spec is removed"), !ASC || ASC->FindAbilitySpecFromHandle(BlockHandle) == nullptr);
	const FGameplayAbilitySpec* ThrustSpec = ASC ? ASC->FindAbilitySpecFromHandle(Player->EquippedWeaponSkillAbilityHandle) : nullptr;
	TestNotNull(TEXT("Granted thrust spec exists"), ThrustSpec);
	TestEqual(TEXT("Granted thrust spec keeps the thrust DA as SourceObject"),
		ThrustSpec ? ThrustSpec->SourceObject.Get() : nullptr,
		static_cast<UObject*>(Thrust));

	Player->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTHSwordWeaponSkillComboStageTest,
	"DevKit.Combat.WeaponSkill.THSwordComboStagePreparation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTHSwordWeaponSkillComboStageTest::RunTest(const FString& Parameters)
{
	UGA_WeaponSkill_THSwordCombo* ComboAbility = NewObject<UGA_WeaponSkill_THSwordCombo>();
	TestNotNull(TEXT("Dedicated two-handed-sword combo GA constructs"), ComboAbility);
	if (!ComboAbility)
	{
		return false;
	}

	TestTrue(
		TEXT("A fresh or closed chain prepares combo slot 1"),
		ComboAbility->PrepareComboSlotForTesting(false, false));
	TestEqual(TEXT("Fresh chain starts at slot 1"), ComboAbility->GetPreparedComboSlotForTesting(), 1);

	ComboAbility->SetCurrentComboSlotForTesting(1);
	TestTrue(
		TEXT("An open combo window advances the same dedicated GA to slot 2"),
		ComboAbility->PrepareComboSlotForTesting(true, true));
	TestEqual(TEXT("Open chain prepares slot 2"), ComboAbility->GetPreparedComboSlotForTesting(), 2);
	ComboAbility->ApplyPreparedComboTagForTesting();

	const FGameplayTag Combo1Tag =
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo1"), false);
	const FGameplayTag Combo2Tag =
		FGameplayTag::RequestGameplayTag(TEXT("Character.State.Skill.WeaponSkill.Combo2"), false);
	TestFalse(TEXT("Slot 1 tag is removed before slot 2 montage lookup"), ComboAbility->AbilityTags.HasTagExact(Combo1Tag));
	TestTrue(TEXT("Slot 2 tag is the sole prepared character combo tag"), ComboAbility->AbilityTags.HasTagExact(Combo2Tag));

	ComboAbility->SetCurrentComboSlotForTesting(4);
	TestFalse(
		TEXT("The dedicated GA rejects a fifth stage"),
		ComboAbility->PrepareComboSlotForTesting(true, true));

	ComboAbility->SetCurrentComboSlotForTesting(2);
	TestTrue(
		TEXT("A closed window restarts at slot 1 rather than skipping ahead"),
		ComboAbility->PrepareComboSlotForTesting(true, false));
	TestEqual(TEXT("Closed chain resets to slot 1"), ComboAbility->GetPreparedComboSlotForTesting(), 1);

	return true;
}

#endif
