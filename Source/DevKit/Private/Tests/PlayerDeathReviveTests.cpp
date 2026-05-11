#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameModes/YogGameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPlayerDeathReviveOfferRulesTest,
	"DevKit.PlayerDeath.ReviveOfferRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPlayerDeathReviveOfferRulesTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("First player death can offer revive"),
		AYogGameMode::CanOfferPlayerDeathRevive(false, false));
	TestFalse(TEXT("Game over already triggered cannot offer revive"),
		AYogGameMode::CanOfferPlayerDeathRevive(true, false));
	TestFalse(TEXT("Used revive cannot be offered again"),
		AYogGameMode::CanOfferPlayerDeathRevive(false, true));

	TestEqual(TEXT("Revive health is 30 percent of max health"),
		AYogGameMode::CalculatePlayerReviveHealth(100.f, 0.3f), 30.f);
	TestEqual(TEXT("Revive health has a one point minimum for positive max health"),
		AYogGameMode::CalculatePlayerReviveHealth(2.f, 0.3f), 1.f);
	TestEqual(TEXT("Zero max health cannot produce revive health"),
		AYogGameMode::CalculatePlayerReviveHealth(0.f, 0.3f), 0.f);

	return true;
}

#endif
