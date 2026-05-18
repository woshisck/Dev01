#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "MetaProgression/MetaProgressionSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetaProgressionSettingsConfigTest,
	"DevKit.MetaProgression.SettingsReadConfiguredTables",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMetaProgressionSettingsConfigTest::RunTest(const FString& Parameters)
{
	const UMetaProgressionSettings* Settings = GetDefault<UMetaProgressionSettings>();
	if (!TestNotNull(TEXT("Meta progression settings CDO exists"), Settings))
	{
		return false;
	}

	const FString ExpectedUpgradePath = TEXT("/Game/MetaProgression/DT_MetaUpgradeNodes.DT_MetaUpgradeNodes");
	const FString ExpectedCurrencyPath = TEXT("/Game/MetaProgression/DT_MetaCurrencyRules.DT_MetaCurrencyRules");

	TestEqual(TEXT("Upgrade table path comes from DefaultGame.ini"),
		Settings->MetaUpgradeNodeTable.ToSoftObjectPath().ToString(), ExpectedUpgradePath);
	TestEqual(TEXT("Currency table path comes from DefaultGame.ini"),
		Settings->MetaCurrencyRuleTable.ToSoftObjectPath().ToString(), ExpectedCurrencyPath);

	TestNotNull(TEXT("Upgrade table asset loads"), Settings->MetaUpgradeNodeTable.LoadSynchronous());
	TestNotNull(TEXT("Currency table asset loads"), Settings->MetaCurrencyRuleTable.LoadSynchronous());

	return true;
}

#endif
