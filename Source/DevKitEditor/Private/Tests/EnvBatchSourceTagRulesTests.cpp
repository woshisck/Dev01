#include "Misc/AutomationTest.h"
#include "Tools/EnvBatchSourceTagRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagBuildsSharedTextureCollectionGroupTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.BuildsSharedTextureCollectionGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagBuildsSharedTextureCollectionGroupTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = TEXT("L1_CommonLevel_Corridor_01a");
	Spec.ActorKind = TEXT("Prop");
	Spec.ProcessingMode = TEXT("Batched");
	Spec.VTCGroup = TEXT("TC-A");
	Spec.bHasExplicitVTCGroup = true;
	Spec.SerialNumber = 2;

	TestEqual(TEXT("Source tag includes texture collection group before serial"),
		BuildEnvBatchSourceTag(Spec),
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Prop.Batched.TC-A.02")));

	TestEqual(TEXT("Texture collection group key omits serial so multiple serial groups can share one collection"),
		BuildEnvBatchVTCGroupKey(Spec),
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Prop.Batched.TC-A")));

	TestEqual(TEXT("Source tag prefix includes texture collection group for next-serial lookup"),
		BuildEnvBatchSourceTagPrefix(Spec),
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Prop.Batched.TC-A.")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagParsesNewAndLegacyTagsTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.ParsesNewAndLegacyTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagParsesNewAndLegacyTagsTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec NewSpec;
	TestTrue(TEXT("New texture collection source tag parses"),
		ParseEnvBatchSourceTag(TEXT("EnvBatch.Source.L1.Prop.Instance.TC-B.12"), NewSpec));
	TestEqual(TEXT("New source tag texture collection group"), NewSpec.VTCGroup, FString(TEXT("TC-B")));
	TestEqual(TEXT("New source tag serial"), NewSpec.SerialNumber, 12);
	TestTrue(TEXT("New source tag records explicit texture collection group"), NewSpec.bHasExplicitVTCGroup);

	FEnvBatchSourceTagSpec LegacySpec;
	TestTrue(TEXT("Legacy source tag parses"),
		ParseEnvBatchSourceTag(TEXT("EnvBatch.Source.L1.Prop.Batched.03"), LegacySpec));
	TestEqual(TEXT("Legacy source tag receives default texture collection group"), LegacySpec.VTCGroup, GetDefaultEnvBatchVTCGroup());
	TestEqual(TEXT("Legacy source tag serial"), LegacySpec.SerialNumber, 3);
	TestFalse(TEXT("Legacy source tag records no explicit texture collection group"), LegacySpec.bHasExplicitVTCGroup);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagBuildsGroundBatchedTagsTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.BuildsGroundBatchedTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagBuildsGroundBatchedTagsTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = TEXT("L1_CommonLevel_Corridor_01a");
	Spec.ActorKind = TEXT("Ground");
	Spec.ProcessingMode = TEXT("Batched");
	Spec.bHasExplicitVTCGroup = false;
	Spec.SerialNumber = 4;

	const FString SourceTag = BuildEnvBatchSourceTag(Spec);
	TestEqual(TEXT("Ground batched source tag omits texture collection group for RVT authoring"),
		SourceTag,
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Ground.Batched.04")));

	FEnvBatchSourceTagSpec ParsedSpec;
	TestTrue(TEXT("Ground batched source tag parses"), ParseEnvBatchSourceTag(SourceTag, ParsedSpec));
	TestEqual(TEXT("Parsed actor kind"), ParsedSpec.ActorKind, FString(TEXT("Ground")));
	TestEqual(TEXT("Parsed processing mode"), ParsedSpec.ProcessingMode, FString(TEXT("Batched")));
	TestEqual(TEXT("Parsed texture collection group"), ParsedSpec.VTCGroup, GetDefaultEnvBatchVTCGroup());
	TestEqual(TEXT("Parsed serial"), ParsedSpec.SerialNumber, 4);
	TestFalse(TEXT("Parsed ground tag records no explicit texture collection group"), ParsedSpec.bHasExplicitVTCGroup);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagBuildsChannelSplitSharedPropTextureCollectionNamesTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.BuildsChannelSplitSharedPropTextureCollectionNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagBuildsChannelSplitSharedPropTextureCollectionNamesTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = TEXT("L1_CommonLevel_Corridor_01a");
	Spec.VTCGroup = TEXT("TC-A");

	TestEqual(TEXT("SharedProp BaseColor texture collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::BaseColor),
		FString(TEXT("TC_L1_CommonLevel_Corridor_01a_TC-A_SharedProp_BaseColor")));

	TestEqual(TEXT("SharedProp Normal texture collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::Normal),
		FString(TEXT("TC_L1_CommonLevel_Corridor_01a_TC-A_SharedProp_Normal")));

	TestEqual(TEXT("SharedProp ORM texture collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::ORM),
		FString(TEXT("TC_L1_CommonLevel_Corridor_01a_TC-A_SharedProp_ORM")));

	TestEqual(TEXT("SharedProp MaterialLight texture collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::MaterialLight),
		FString(TEXT("TC_L1_CommonLevel_Corridor_01a_TC-A_SharedProp_MaterialLight")));

	return true;
}

#endif
