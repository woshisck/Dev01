#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitPerformanceQuickEntryToolbarTest,
	"DevKitEditor.Toolbar.PerformanceQuickEntriesRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitPerformanceQuickEntryToolbarTest::RunTest(const FString& Parameters)
{
	UToolMenus* ToolMenus = UToolMenus::Get();
	if (!TestNotNull(TEXT("ToolMenus subsystem is available"), ToolMenus))
	{
		return false;
	}

	const FName PlayToolbarName(TEXT("LevelEditor.LevelEditorToolBar.PlayToolBar"));
	UToolMenu* PlayToolbar = ToolMenus->FindMenu(PlayToolbarName);
	if (!TestNotNull(TEXT("Level editor play toolbar menu is registered"), PlayToolbar))
	{
		return false;
	}

	const FToolMenuSection* PlaySection = PlayToolbar->FindSection(TEXT("Play"));
	if (!TestNotNull(TEXT("Level editor play toolbar contains the Play section"), PlaySection))
	{
		return false;
	}

	const FToolMenuEntry* Entry = PlaySection->FindEntry(TEXT("DevKitPlayFromMainMenu"));
	TestNotNull(TEXT("Custom entry-menu play toolbar button is registered"), Entry);

	const FName UserToolbarName(TEXT("LevelEditor.LevelEditorToolBar.User"));
	UToolMenu* UserToolbar = ToolMenus->FindMenu(UserToolbarName);
	if (!TestNotNull(TEXT("Level editor user toolbar menu is registered"), UserToolbar))
	{
		return false;
	}

	const FToolMenuSection* PerformanceSection = UserToolbar->FindSection(TEXT("DevKitPerformanceTools"));
	if (!TestNotNull(TEXT("Level editor user toolbar contains the DevKit performance section"), PerformanceSection))
	{
		return false;
	}

	const FToolMenuEntry* LauncherEntry = PerformanceSection->FindEntry(TEXT("OpenDevKitPerformanceToolsLauncher"));
	TestNotNull(TEXT("Performance tools launcher toolbar button is registered"), LauncherEntry);

	const FToolMenuEntry* MapCreatorEntry = PerformanceSection->FindEntry(TEXT("OpenDevKitMapCreator"));
	TestNotNull(TEXT("Map creator toolbar button is registered"), MapCreatorEntry);

	const FToolMenuEntry* LevelBatchProcessorEntry = PerformanceSection->FindEntry(TEXT("OpenDevKitLevelBatchProcessor"));
	TestNotNull(TEXT("Level batch processor toolbar button is registered"), LevelBatchProcessorEntry);

	const FName MainMenuName(TEXT("LevelEditor.MainMenu"));
	UToolMenu* MainMenu = ToolMenus->FindMenu(MainMenuName);
	if (!TestNotNull(TEXT("Level editor main menu is registered"), MainMenu))
	{
		return false;
	}

	const FToolMenuSection* MainMenuSection = MainMenu->FindSection(NAME_None);
	if (!TestNotNull(TEXT("Level editor main menu contains the top-level menu section"), MainMenuSection))
	{
		return false;
	}

	const FToolMenuEntry* YogToolEntry = MainMenuSection->FindEntry(TEXT("YogToolMenu"));
	TestNotNull(TEXT("YogTool top-level menu is registered"), YogToolEntry);

	return Entry != nullptr && LauncherEntry != nullptr && MapCreatorEntry != nullptr && LevelBatchProcessorEntry != nullptr && YogToolEntry != nullptr;
}

#endif
