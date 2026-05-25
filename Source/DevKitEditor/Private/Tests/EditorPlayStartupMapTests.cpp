#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
bool ReadGameMapsSetting(const TCHAR* Key, FString& OutValue)
{
	return GConfig && GConfig->GetString(TEXT("/Script/EngineSettings.GameMapsSettings"), Key, OutValue, GEngineIni);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEditorGreenPlayUsesCurrentMapConfigTest,
	"DevKitEditor.Play.GreenPlayUsesCurrentMapByDefault",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEditorGreenPlayUsesCurrentMapConfigTest::RunTest(const FString& Parameters)
{
	FString GameDefaultMap;
	FString ServerDefaultMap;

	TestTrue(TEXT("GameDefaultMap setting is present"), ReadGameMapsSetting(TEXT("GameDefaultMap"), GameDefaultMap));
	TestTrue(TEXT("ServerDefaultMap setting is present"), ReadGameMapsSetting(TEXT("ServerDefaultMap"), ServerDefaultMap));

	TestEqual(TEXT("Green editor Play should use the current editor map instead of forcing the frontend map"),
		GameDefaultMap,
		FString(TEXT("None")));
	TestEqual(TEXT("Standalone editor Play should use the current editor map instead of forcing the frontend map"),
		ServerDefaultMap,
		FString(TEXT("None")));

	return true;
}

#endif
