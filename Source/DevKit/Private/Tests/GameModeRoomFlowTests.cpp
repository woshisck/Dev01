#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Data/AltarDataAsset.h"
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

#endif
