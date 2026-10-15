#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/AltarDataAsset.h"
#include "Data/CampaignDataAsset.h"
#include "Data/RoomDataAsset.h"
#include "GameModes/YogGameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameModeNonCombatEventRoomTest,
	"DevKit.GameMode.NonCombatEventRoom",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameModeNonCombatEventRoomTest::RunTest(const FString& Parameters)
{
	URoomDataAsset* EventRoom = NewObject<URoomDataAsset>();
	EventRoom->SacrificeEventAltarData = NewObject<UAltarDataAsset>(EventRoom);
	EventRoom->EnemyPool.Reset();

	TestTrue(
		TEXT("Empty sacrifice event room skips combat reward flow"),
		AYogGameMode::ShouldSkipCombatForRoom(EventRoom));

	URoomDataAsset* CombatEventRoom = NewObject<URoomDataAsset>();
	CombatEventRoom->SacrificeEventAltarData = NewObject<UAltarDataAsset>(CombatEventRoom);
	CombatEventRoom->EnemyPool.AddDefaulted();

	TestFalse(
		TEXT("Sacrifice event room with enemies still uses combat flow"),
		AYogGameMode::ShouldSkipCombatForRoom(CombatEventRoom));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameModeEditorPIEPreservesCurrentMapTest,
	"DevKit.GameMode.EditorPIEPreservesCurrentMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameModeEditorPIEPreservesCurrentMapTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Plain PIE without an explicit room keeps the editor's current map"),
		AYogGameMode::ShouldPreserveCurrentMapForEditorPlay(true, false));

	TestFalse(
		TEXT("Frontend or portal room data keeps using the explicit room flow"),
		AYogGameMode::ShouldPreserveCurrentMapForEditorPlay(true, true));

	TestFalse(
		TEXT("Non-PIE worlds still use configured starting room flow"),
		AYogGameMode::ShouldPreserveCurrentMapForEditorPlay(false, false));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameModeFindRoomDataForLoadedMapTest,
	"DevKit.GameMode.FindRoomDataForLoadedMap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameModeFindRoomDataForLoadedMapTest::RunTest(const FString& Parameters)
{
	UCampaignDataAsset* Campaign = NewObject<UCampaignDataAsset>();

	URoomDataAsset* StartRoom = NewObject<URoomDataAsset>(Campaign);
	StartRoom->RoomName = FName(TEXT("/Game/Maps/Dungeon/FirstLevel"));

	URoomDataAsset* SecondRoom = NewObject<URoomDataAsset>(Campaign);
	SecondRoom->RoomName = FName(TEXT("/Game/Maps/Dungeon/SecondLevel"));

	Campaign->DefaultStartingRoom = StartRoom;
	Campaign->RoomPool.Add(StartRoom);
	Campaign->RoomPool.Add(SecondRoom);

	TestEqual(
		TEXT("Starting room resolves from the loaded map's short name"),
		AYogGameMode::FindRoomDataForLoadedMap(Campaign, TEXT("FirstLevel")),
		StartRoom);

	TestEqual(
		TEXT("Map name matching ignores case"),
		AYogGameMode::FindRoomDataForLoadedMap(Campaign, TEXT("secondlevel")),
		SecondRoom);

	TestNull(
		TEXT("Unknown map falls back to no room data"),
		AYogGameMode::FindRoomDataForLoadedMap(Campaign, TEXT("SomeArtScratchMap")));

	TestNull(
		TEXT("Null campaign is handled"),
		AYogGameMode::FindRoomDataForLoadedMap(nullptr, TEXT("SecondLevel")));

	return true;
}

#endif
