#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Data/RoomDataAsset.h"
#include "GameModes/YogGameMode.h"
#include "Map/RewardPickup.h"

namespace RewardPickupTests
{
FLootOption MakeLootOption(ELootType LootType, int32 Amount = 1)
{
	FLootOption Option;
	Option.LootType = LootType;
	Option.Amount = Amount;
	return Option;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRewardPickupImmediateGrantRoutesByLootTypeTest,
	"DevKit.RewardPickup.ImmediateGrantRoutesByLootType",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRewardPickupImmediateGrantRoutesByLootTypeTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Gold-only pickup grants immediately"),
		ARewardPickup::ShouldGrantLootImmediatelyForOptions({
			RewardPickupTests::MakeLootOption(ELootType::Gold, 50)
		}));

	TestTrue(TEXT("Material-only pickup grants immediately"),
		ARewardPickup::ShouldGrantLootImmediatelyForOptions({
			RewardPickupTests::MakeLootOption(ELootType::Material, 1)
		}));

	TestTrue(TEXT("Currency and material pickup grants immediately"),
		ARewardPickup::ShouldGrantLootImmediatelyForOptions({
			RewardPickupTests::MakeLootOption(ELootType::Gold, 50),
			RewardPickupTests::MakeLootOption(ELootType::Material, 1)
		}));

	TestFalse(TEXT("Rune pickup opens selection UI"),
		ARewardPickup::ShouldGrantLootImmediatelyForOptions({
			RewardPickupTests::MakeLootOption(ELootType::Rune, 0)
		}));

	TestFalse(TEXT("Any pickup containing a rune opens selection UI"),
		ARewardPickup::ShouldGrantLootImmediatelyForOptions({
			RewardPickupTests::MakeLootOption(ELootType::Gold, 50),
			RewardPickupTests::MakeLootOption(ELootType::Rune, 0)
		}));

	TestFalse(TEXT("Empty pickup cannot grant immediately"),
		ARewardPickup::ShouldGrantLootImmediatelyForOptions({}));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameModeExtraRewardPickupDisabledByDefaultTest,
	"DevKit.RewardPickup.ExtraRewardPickupDisabledByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameModeExtraRewardPickupDisabledByDefaultTest::RunTest(const FString& Parameters)
{
	URoomDataAsset* CombatRoom = NewObject<URoomDataAsset>();
	TestNotNull(TEXT("Combat room data exists"), CombatRoom);
	if (!CombatRoom)
	{
		return false;
	}

	CombatRoom->bIsHubRoom = false;

	TestFalse(TEXT("Extra RewardPickup is disabled while the temporary gate is off"),
		AYogGameMode::ShouldAllowExtraRewardPickupForRoom(
			CombatRoom,
			false,
			true,
			false));

	TestTrue(TEXT("Extra RewardPickup can be explicitly re-enabled for normal combat rooms"),
		AYogGameMode::ShouldAllowExtraRewardPickupForRoom(
			CombatRoom,
			false,
			true,
			true));

	CombatRoom->bIsHubRoom = true;
	TestFalse(TEXT("Hub rooms never spawn extra RewardPickup"),
		AYogGameMode::ShouldAllowExtraRewardPickupForRoom(
			CombatRoom,
			false,
			true,
			true));

	CombatRoom->bIsHubRoom = false;
	TestFalse(TEXT("Sacrifice event rooms do not spawn extra RewardPickup"),
		AYogGameMode::ShouldAllowExtraRewardPickupForRoom(
			CombatRoom,
			true,
			true,
			true));

	TestFalse(TEXT("Missing RewardPickupClass prevents extra RewardPickup"),
		AYogGameMode::ShouldAllowExtraRewardPickupForRoom(
			CombatRoom,
			false,
			false,
			true));

	return true;
}

#endif
