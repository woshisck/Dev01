#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tools/LevelBatch/DevKitLevelBatchService.h"
#include "Tools/MapCreator/DevKitMapCreatorService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorIncludesBatchedSublevelTest,
	"DevKitEditor.LevelBatch.MapCreatorIncludesBatchedSublevel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorIncludesBatchedSublevelTest::RunTest(const FString& Parameters)
{
	TestTrue(
		TEXT("Map creator creates a Batched sublevel for generated proxy actors"),
		FDevKitMapCreatorService::GetSublevelSuffixes().Contains(TEXT("Batched")));

	TestEqual(
		TEXT("Batched template path"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Batched"), TEXT("L1")),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp_Batched")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelBatchBuildsConventionalPathsTest,
	"DevKitEditor.LevelBatch.BuildsConventionalPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelBatchBuildsConventionalPathsTest::RunTest(const FString& Parameters)
{
	const FDevKitLevelBatchPaths Paths = FDevKitLevelBatchService::BuildPaths(
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom"));

	TestEqual(TEXT("Level name"), Paths.LevelName, FString(TEXT("L1_CommonLevel_TestRoom")));
	TestEqual(TEXT("Level asset folder"), Paths.LevelAssetFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom/LevelAsset")));
	TestEqual(TEXT("Batched asset folder"), Paths.BatchedAssetFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom/BatchedAsset")));
	TestEqual(TEXT("Persistent map"), Paths.PersistentMapPackage, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom/LevelAsset/L1_CommonLevel_TestRoom")));
	TestEqual(TEXT("Batched map"), Paths.BatchedMapPackage, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom/LevelAsset/L1_CommonLevel_TestRoom_Batched")));
	TestTrue(TEXT("Status file is stored under BatchedAsset"), Paths.StatusFilePath.EndsWith(TEXT("Content/Art/Map/Map_Data/L1_CommonLevel_TestRoom/BatchedAsset/EnvBatchStatus.json")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelBatchBuildsPartialApplyCommandTest,
	"DevKitEditor.LevelBatch.BuildsPartialApplyCommand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelBatchBuildsPartialApplyCommandTest::RunTest(const FString& Parameters)
{
	const FDevKitLevelBatchPaths Paths = FDevKitLevelBatchService::BuildPaths(
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom"));
	const FDevKitLevelBatchCommand Command = FDevKitLevelBatchService::BuildPartialApplyCommand(Paths, TEXT("Mid"));

	TestTrue(TEXT("Runs MaterialBatchBuild"), Command.Arguments.Contains(TEXT("-run=MaterialBatchBuild")));
	TestTrue(TEXT("Uses persistent map"), Command.Arguments.Contains(TEXT("-Map=/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom/LevelAsset/L1_CommonLevel_TestRoom")));
	TestTrue(TEXT("Uses BatchedAsset output root"), Command.Arguments.Contains(TEXT("-OutputRoot=/Game/Art/Map/Map_Data/L1_CommonLevel_TestRoom/BatchedAsset")));
	TestTrue(TEXT("Uses EnvBatch level tag prefix"), Command.Arguments.Contains(TEXT("-RequireTag=EnvBatch.Source.L1_CommonLevel_TestRoom.")));
	TestTrue(TEXT("Writes proxy mesh in partial apply"), Command.Arguments.Contains(TEXT("-ApplyProxyMeshOnly")));
	TestTrue(TEXT("Writes batch material in partial apply"), Command.Arguments.Contains(TEXT("-ApplyBatchMaterialOnly")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelBatchCleanupKeepsGeneratedActorsTest,
	"DevKitEditor.LevelBatch.CleanupKeepsGeneratedActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelBatchCleanupKeepsGeneratedActorsTest::RunTest(const FString& Parameters)
{
	const FDevKitLevelBatchCleanupPolicy Policy = FDevKitLevelBatchService::GetCleanupPolicy();
	TestTrue(TEXT("Cleanup restores source actors"), Policy.bRestoreSourceActors);
	TestFalse(TEXT("Cleanup does not delete generated actors"), Policy.bDeleteGeneratedActors);
	TestFalse(TEXT("Cleanup does not delete generated assets"), Policy.bDeleteGeneratedAssets);
	return true;
}

#endif
