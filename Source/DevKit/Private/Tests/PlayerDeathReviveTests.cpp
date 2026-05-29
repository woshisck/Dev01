#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "GameModes/YogGameMode.h"
#include "UI/YogGameOverWidget.h"

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
	TestFalse(TEXT("Final death game over is owned by the GameOver screen, not RunSummary"),
		AYogGameMode::ShouldBroadcastRunSummaryForPlayerDeathGameOver(false));
	TestFalse(TEXT("Revive choice game over never opens RunSummary behind it"),
		AYogGameMode::ShouldBroadcastRunSummaryForPlayerDeathGameOver(true));

	TArray<EYogGameOverMenuAction> GameOverActions;
	UYogGameOverWidget::BuildDeathMenuActions(true, GameOverActions);
	TestEqual(TEXT("Revive death menu has revive and return to hub"), GameOverActions.Num(), 2);
	TestTrue(TEXT("Revive death menu offers revive first"),
		GameOverActions.IsValidIndex(0) && GameOverActions[0] == EYogGameOverMenuAction::Revive);
	TestTrue(TEXT("Revive death menu offers return to hub second"),
		GameOverActions.IsValidIndex(1) && GameOverActions[1] == EYogGameOverMenuAction::ReturnToHub);

	UYogGameOverWidget::BuildDeathMenuActions(false, GameOverActions);
	TestEqual(TEXT("Final death menu only has return to hub"), GameOverActions.Num(), 1);
	TestTrue(TEXT("Final death menu offers return to hub"),
		GameOverActions.IsValidIndex(0) && GameOverActions[0] == EYogGameOverMenuAction::ReturnToHub);

	TestEqual(TEXT("Revive health is 30 percent of max health"),
		AYogGameMode::CalculatePlayerReviveHealth(100.f, 0.3f), 30.f);
	TestEqual(TEXT("Revive health has a one point minimum for positive max health"),
		AYogGameMode::CalculatePlayerReviveHealth(2.f, 0.3f), 1.f);
	TestEqual(TEXT("Zero max health cannot produce revive health"),
		AYogGameMode::CalculatePlayerReviveHealth(0.f, 0.3f), 0.f);

	return true;
}

#endif
