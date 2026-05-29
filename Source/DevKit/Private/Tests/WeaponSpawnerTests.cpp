#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Item/Weapon/WeaponSpawner.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWeaponSpawnerTutorialVisibilityRulesTest,
	"DevKit.WeaponSpawner.TutorialVisibilityRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWeaponSpawnerTutorialVisibilityRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Always spawners are enabled during first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::Always,
			true));
	TestTrue(TEXT("Always spawners are enabled outside first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::Always,
			false));

	TestTrue(TEXT("FirstRunTutorialOnly spawners are enabled during first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::FirstRunTutorialOnly,
			true));
	TestFalse(TEXT("FirstRunTutorialOnly spawners are disabled outside first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::FirstRunTutorialOnly,
			false));

	TestFalse(TEXT("NonTutorialOnly spawners are disabled during first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::NonTutorialOnly,
			true));
	TestTrue(TEXT("NonTutorialOnly spawners are enabled outside first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::NonTutorialOnly,
			false));

	TestTrue(TEXT("Tagged first-run tutorial weapons are enabled only during first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::Always,
			true,
			true,
			false));
	TestFalse(TEXT("Tagged first-run tutorial weapons are disabled for normal runs"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::Always,
			false,
			true,
			false));

	TestFalse(TEXT("Tagged main-run start weapons are disabled during first-run tutorial"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::Always,
			true,
			false,
			true));
	TestTrue(TEXT("Tagged main-run start weapons are enabled for normal runs"),
		AWeaponSpawner::ShouldEnableForFirstRunTutorialState(
			EWeaponSpawnerTutorialVisibility::Always,
			false,
			false,
			true));

	return true;
}

#endif
