#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tools/RVTMeshDecal/DevKitRVTMeshDecalService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitRVTMeshDecalInfersDefaultFolderTest,
	"DevKitEditor.RVTMeshDecal.InfersDefaultFolder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMeshDecalInfersDefaultFolderTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Default RVT decal folder sits under sibling BakeInfo folder"),
		FDevKitRVTMeshDecalService::InferDefaultFoliageTypeFolderFromWorldPackage(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Art")),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo/RVTDecalFoliage"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitRVTMeshDecalBuildsAssetNameTest,
	"DevKitEditor.RVTMeshDecal.BuildsAssetName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMeshDecalBuildsAssetNameTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Material name and priority are encoded into the foliage type name"),
		FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(TEXT("/Game/Art/Map/LevelMaterial/MI_Dirt_Crack.MI_Dirt_Crack"), 20),
		TEXT("FT_RVTDecal_MI_Dirt_Crack_P20"));

	TestEqual(
		TEXT("Negative priority keeps a readable minus token"),
		FDevKitRVTMeshDecalService::BuildDefaultFoliageTypeName(TEXT("/Game/Art/Map/LevelMaterial/MI_Dust.MI_Dust"), -5),
		TEXT("FT_RVTDecal_MI_Dust_PM5"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitRVTMeshDecalBuildsPathsTest,
	"DevKitEditor.RVTMeshDecal.BuildsPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitRVTMeshDecalBuildsPathsTest::RunTest(const FString& Parameters)
{
	const FDevKitRVTMeshDecalRequest Request{
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Test_01/BakeInfo/RVTDecalFoliage"),
		TEXT("/Game/EngineArt/Tools/SM_DecalPlane.SM_DecalPlane"),
		TEXT("/Game/Art/Map/LevelMaterial/MI_Dirt.MI_Dirt"),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Test_01/BakeInfo/RVT_L1_CommonLevel_Test_01_Ground.RVT_L1_CommonLevel_Test_01_Ground"),
		10,
		0.25f,
		2.0f,
		true,
		true,
		TEXT("")
	};

	FText Error;
	const TOptional<FDevKitRVTMeshDecalPaths> Paths = FDevKitRVTMeshDecalService::BuildPaths(Request, Error);

	TestTrue(TEXT("Paths are valid"), Paths.IsSet());
	if (!Paths.IsSet())
	{
		return false;
	}

	TestEqual(TEXT("Asset name"), Paths->FoliageTypeName, TEXT("FT_RVTDecal_MI_Dirt_P10"));
	TestEqual(TEXT("Package path"), Paths->FoliageTypePackage, TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Test_01/BakeInfo/RVTDecalFoliage/FT_RVTDecal_MI_Dirt_P10"));
	TestEqual(TEXT("Object path"), Paths->FoliageTypeObjectPath, TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Test_01/BakeInfo/RVTDecalFoliage/FT_RVTDecal_MI_Dirt_P10.FT_RVTDecal_MI_Dirt_P10"));

	return true;
}

#endif
