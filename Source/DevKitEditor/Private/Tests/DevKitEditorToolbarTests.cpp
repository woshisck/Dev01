#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "ToolMenu.h"
#include "ToolMenuEntry.h"
#include "ToolMenuSection.h"
#include "ToolMenus.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitNoCustomEntryMenuPlayButtonTest,
	"DevKitEditor.Toolbar.NoCustomEntryMenuPlayButtonRegistered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitNoCustomEntryMenuPlayButtonTest::RunTest(const FString& Parameters)
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
	TestNull(TEXT("Custom entry-menu play toolbar button is not registered"), Entry);
	return Entry == nullptr;
}

#endif
