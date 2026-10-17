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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorBuildsIndividualSublevelPathsTest,
	"DevKitEditor.MapCreator.BuildsIndividualSublevelPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorBuildsIndividualSublevelPathsTest::RunTest(const FString& Parameters)
{
	const FString Parent = TEXT("/Game/Art/Map/Example/LevelAsset/L1_CommonLevel_Example");
	FText Error;
	for (const FString& Suffix : FDevKitMapCreatorService::GetSublevelSuffixes())
	{
		const FDevKitSublevelRequest Request{TEXT("/Game/Art/Map/Example/LevelAsset/"), Suffix, TEXT("L1")};
		const TOptional<FString> Path = FDevKitMapCreatorService::BuildSublevelPath(Parent, Request, Error);
		TestTrue(*FString::Printf(TEXT("Path for %s"), *Suffix), Path.IsSet());
		if (Path.IsSet())
		{
			TestEqual(*FString::Printf(TEXT("Child naming for %s"), *Suffix), Path.GetValue(),
				FString::Printf(TEXT("%s_%s"), *Parent, *Suffix));
		}
	}
	const FDevKitSublevelRequest DefaultFolder{FString(), TEXT("Art"), TEXT("L2")};
	const TOptional<FString> DefaultPath = FDevKitMapCreatorService::BuildSublevelPath(Parent, DefaultFolder, Error);
	TestTrue(TEXT("Empty target uses parent map folder"), DefaultPath.IsSet());
	if (DefaultPath.IsSet())
	{
		TestEqual(TEXT("Default target path"), DefaultPath.GetValue(), Parent + TEXT("_Art"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorRejectsUnsafeSublevelPathsTest,
	"DevKitEditor.MapCreator.RejectsUnsafeSublevelPaths",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorRejectsUnsafeSublevelPathsTest::RunTest(const FString& Parameters)
{
	const FString Parent = TEXT("/Game/Art/Map/L1_Example");
	FText Error;
	const TArray<FString> BadFolders{
		TEXT("/Engine/Maps"), TEXT("/YogArt/Maps"), TEXT("/Game/Art/../Maps"), TEXT("X:/Temp/Maps")
	};
	for (const FString& Folder : BadFolders)
	{
		TestFalse(*FString::Printf(TEXT("Reject %s"), *Folder),
			FDevKitMapCreatorService::BuildSublevelPath(Parent, {Folder, TEXT("Art"), TEXT("L1")}, Error).IsSet());
	}
	TestFalse(TEXT("Reject unsupported child type"),
		FDevKitMapCreatorService::BuildSublevelPath(Parent, {TEXT("/Game/Art/Map"), TEXT("Other"), TEXT("L1")}, Error).IsSet());
	TestFalse(TEXT("Reject unsupported lighting layer"),
		FDevKitMapCreatorService::BuildSublevelPath(Parent, {TEXT("/Game/Art/Map"), TEXT("Light"), TEXT("L4")}, Error).IsSet());
	TestFalse(TEXT("Reject object path instead of package"),
		FDevKitMapCreatorService::BuildSublevelPath(Parent + TEXT(".L1_Example"),
			{TEXT("/Game/Art/Map"), TEXT("Art"), TEXT("L1")}, Error).IsSet());
	TestFalse(TEXT("Full stack cannot write engine content"),
		FDevKitMapCreatorService::BuildPaths({TEXT("/Engine/Maps"), TEXT("L1"), TEXT("CommonLevel"), TEXT("Example")}, Error).IsSet());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDevKitMapCreatorRejectsMissingEditorWorldTest,
	"DevKitEditor.MapCreator.RejectsMissingEditorWorld",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDevKitMapCreatorRejectsMissingEditorWorldTest::RunTest(const FString& Parameters)
{
	const FDevKitSublevelResult Created = FDevKitMapCreatorService::CreateAndAttachSublevel(nullptr,
		{TEXT("/Game/Art/Map"), TEXT("Art"), TEXT("L1")});
	TestFalse(TEXT("No world rejects creation"), Created.bSuccess);
	TestFalse(TEXT("No file creation on rejection"), Created.bAssetCreated);
	TestFalse(TEXT("No association on rejection"), Created.bAttached);
	TestFalse(TEXT("No parent save on rejection"), Created.bParentSaved);
	TestTrue(TEXT("Rejected operation explains cause"), !Created.Message.IsEmpty());
	const FDevKitSublevelResult Attached = FDevKitMapCreatorService::AttachExistingSublevel(nullptr,
		TEXT("/Game/Art/Map/Example_Art"));
	TestFalse(TEXT("No world rejects existing association"), Attached.bSuccess);
	TestFalse(TEXT("Existing asset remains untouched"), Attached.bAssetCreated);
	TestFalse(TEXT("No existing association on rejection"), Attached.bAttached);
	return true;
}

#endif
