#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

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

#endif
