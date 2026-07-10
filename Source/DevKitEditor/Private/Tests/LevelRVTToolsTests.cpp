#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tools/LevelRVT/DevKitLevelRVTService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelRVTInfersBakeInfoFolderTest,
	"DevKitEditor.LevelRVT.InfersBakeInfoFolder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelRVTInfersBakeInfoFolderTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Persistent map under LevelAsset resolves to sibling BakeInfo"),
		FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth")),
		FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo")));

	TestEqual(
		TEXT("Sublevel under LevelAsset resolves to sibling BakeInfo"),
		FDevKitLevelRVTService::InferBakeInfoFolderFromWorldPackage(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Art")),
		FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo")));

	TestEqual(
		TEXT("Sublevel under LevelAsset resolves to DataBake sublevel"),
		FDevKitLevelRVTService::InferDataBakeLevelPackageFromWorldPackage(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Art")),
		FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_DataBake")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelRVTBuildsGroundAssetPathsTest,
	"DevKitEditor.LevelRVT.BuildsGroundAssetPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelRVTBuildsGroundAssetPathsTest::RunTest(const FString& Parameters)
{
	const FDevKitLevelRVTRequest Request{
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo"),
		TEXT("RVT_L1_CommonLevel_Corridor_S_Goth_Ground"),
		TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_S_Goth.Ground.Batched.01")
	};

	FText Error;
	const TOptional<FDevKitLevelRVTPaths> Paths = FDevKitLevelRVTService::BuildPaths(Request, Error);
	TestTrue(TEXT("Valid request builds paths"), Paths.IsSet());
	if (!Paths.IsSet())
	{
		AddError(Error.ToString());
		return false;
	}

	TestEqual(TEXT("BakeInfo folder"), Paths->BakeInfoFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo")));
	TestEqual(TEXT("RVT asset name"), Paths->RuntimeVirtualTextureName, FString(TEXT("RVT_L1_CommonLevel_Corridor_S_Goth_Ground")));
	TestEqual(TEXT("RVT package"), Paths->RuntimeVirtualTexturePackage, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo/RVT_L1_CommonLevel_Corridor_S_Goth_Ground")));
	TestEqual(TEXT("DataBake level package"), Paths->DataBakeLevelPackage, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_DataBake")));
	TestEqual(TEXT("Volume actor name"), Paths->VolumeActorName, FString(TEXT("RVT_L1_CommonLevel_Corridor_S_Goth_Ground_Volume")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelRVTBuildsDefaultAssetNameTest,
	"DevKitEditor.LevelRVT.BuildsDefaultAssetName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelRVTBuildsDefaultAssetNameTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Default name uses map folder"),
		FDevKitLevelRVTService::BuildDefaultGroundRVTNameFromWorldPackage(TEXT("/Game/Art/Map/Map_Data/L2_EliteLevel_Prison_01/LevelAsset/L2_EliteLevel_Prison_01_Art")),
		FString(TEXT("RVT_L2_EliteLevel_Prison_01_Ground")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitLevelRVTBuildsLayoutAssetNamesTest,
	"DevKitEditor.LevelRVT.BuildsLayoutAssetNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitLevelRVTBuildsLayoutAssetNamesTest::RunTest(const FString& Parameters)
{
	const FString BaseName(TEXT("RVT_L1_CommonLevel_Test_Ground"));
	TestEqual(
		TEXT("Default layout keeps the base asset name"),
		FDevKitLevelRVTService::BuildAssetNameForLayout(BaseName, EDevKitLevelRVTLayout::BaseColorNormalRoughness),
		BaseName);
	TestEqual(
		TEXT("Mask4 layout gets a stable suffix"),
		FDevKitLevelRVTService::BuildAssetNameForLayout(BaseName, EDevKitLevelRVTLayout::Mask4),
		BaseName + TEXT("_Mask4"));
	TestEqual(
		TEXT("World height layout gets a stable suffix"),
		FDevKitLevelRVTService::BuildAssetNameForLayout(BaseName, EDevKitLevelRVTLayout::WorldHeight),
		BaseName + TEXT("_WorldHeight"));

	return true;
}

#endif
