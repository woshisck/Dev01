#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Tools/MapCreator/DevKitMapCreatorService.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorBuildsLevelStackPathsTest,
	"DevKitEditor.MapCreator.BuildsLevelStackPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorBuildsLevelStackPathsTest::RunTest(const FString& Parameters)
{
	const FDevKitMapCreatorRequest Request{
		TEXT("/Game/Art/Map/Map_Data"),
		TEXT("L1"),
		TEXT("CommonLevel"),
		TEXT("Corridor_S_Goth")
	};

	FText Error;
	const TOptional<FDevKitMapCreatorPaths> Paths = FDevKitMapCreatorService::BuildPaths(Request, Error);
	TestTrue(TEXT("Valid request builds paths"), Paths.IsSet());
	if (!Paths.IsSet())
	{
		AddError(Error.ToString());
		return false;
	}

	TestEqual(TEXT("Base name"), Paths->BaseName, FString(TEXT("L1_CommonLevel_Corridor_S_Goth")));
	TestEqual(TEXT("Target folder"), Paths->TargetFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth")));
	TestEqual(TEXT("Level asset folder"), Paths->LevelAssetFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset")));
	TestEqual(TEXT("Level material folder"), Paths->LevelMaterialFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelMaterial")));
	TestEqual(TEXT("Batched asset folder"), Paths->BatchedAssetFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BatchedAsset")));
	TestEqual(TEXT("Bake info folder"), Paths->BakeInfoFolder, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/BakeInfo")));
	TestEqual(TEXT("Persistent map"), Paths->PersistentMap, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth")));
	TestEqual(TEXT("Map definition"), Paths->MapDefinition, FString(TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/DA_L1_CommonLevel_Corridor_S_Goth")));

	const TArray<FString> ExpectedSublevels{
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Art"),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Batched"),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_DataBake"),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Gameplay"),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_Light"),
		TEXT("/Game/Art/Map/Map_Data/L1_CommonLevel_Corridor_S_Goth/LevelAsset/L1_CommonLevel_Corridor_S_Goth_PLA")
	};

	TestEqual(TEXT("Sublevel count"), Paths->Sublevels.Num(), ExpectedSublevels.Num());
	for (int32 Index = 0; Index < ExpectedSublevels.Num() && Index < Paths->Sublevels.Num(); ++Index)
	{
		TestEqual(FString::Printf(TEXT("Sublevel %d"), Index), Paths->Sublevels[Index], ExpectedSublevels[Index]);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorRejectsBadSuffixTest,
	"DevKitEditor.MapCreator.RejectsBadSuffix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorRejectsBadSuffixTest::RunTest(const FString& Parameters)
{
	const FDevKitMapCreatorRequest Request{
		TEXT("/Game/Art/Map/Map_Data"),
		TEXT("L2"),
		TEXT("BossLevel"),
		TEXT("Boss Room")
	};

	FText Error;
	const TOptional<FDevKitMapCreatorPaths> Paths = FDevKitMapCreatorService::BuildPaths(Request, Error);
	TestFalse(TEXT("Suffix containing a space is rejected"), Paths.IsSet());
	TestTrue(TEXT("Error explains suffix"), Error.ToString().Contains(TEXT("suffix")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorProvidesDefaultTemplatesTest,
	"DevKitEditor.MapCreator.ProvidesDefaultTemplates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorProvidesDefaultTemplatesTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Persistent template"),
		FDevKitMapCreatorService::GetPersistentTemplateMapPath(),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp")));

	TestEqual(
		TEXT("DataBake template"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("DataBake"), TEXT("L1")),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp_DataBake")));

	TestEqual(
		TEXT("Batched template"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Batched"), TEXT("L1")),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp_Batched")));

	TestEqual(
		TEXT("L1 uses dungeon light template"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Light"), TEXT("L1")),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp_Light_Dungeon")));

	TestEqual(
		TEXT("L2 uses outside light template"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Light"), TEXT("L2")),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp_Light_Outside")));

	TestEqual(
		TEXT("L3 uses inside light template"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Light"), TEXT("L3")),
		FString(TEXT("/Game/Art/Map/Templates/LevelAsset/L_Temp_Light_Inside")));

	TestTrue(
		TEXT("Unknown suffix remains blank"),
		FDevKitMapCreatorService::GetSublevelTemplateMapPath(TEXT("Unknown"), TEXT("L1")).IsEmpty());

	return true;
}

#endif
