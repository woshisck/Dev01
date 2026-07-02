#include "Misc/AutomationTest.h"
#include "Tools/EnvBatchSourceTagRules.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagBuildsSharedVTCGroupTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.BuildsSharedVTCGroup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagBuildsSharedVTCGroupTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = TEXT("L1_CommonLevel_Corridor_01a");
	Spec.ActorKind = TEXT("Prop");
	Spec.ProcessingMode = TEXT("Batched");
	Spec.VTCGroup = TEXT("VTC-A");
	Spec.SerialNumber = 2;

	TestEqual(TEXT("Source tag includes VTC group before serial"),
		BuildEnvBatchSourceTag(Spec),
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Prop.Batched.VTC-A.02")));

	TestEqual(TEXT("VTC group key omits serial so multiple serial groups can share one VTC"),
		BuildEnvBatchVTCGroupKey(Spec),
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Prop.Batched.VTC-A")));

	TestEqual(TEXT("Source tag prefix includes VTC group for next-serial lookup"),
		BuildEnvBatchSourceTagPrefix(Spec),
		FString(TEXT("EnvBatch.Source.L1_CommonLevel_Corridor_01a.Prop.Batched.VTC-A.")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagParsesNewAndLegacyTagsTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.ParsesNewAndLegacyTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagParsesNewAndLegacyTagsTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec NewSpec;
	TestTrue(TEXT("New VTC source tag parses"),
		ParseEnvBatchSourceTag(TEXT("EnvBatch.Source.L1.Prop.Instance.VTC-B.12"), NewSpec));
	TestEqual(TEXT("New source tag VTC group"), NewSpec.VTCGroup, FString(TEXT("VTC-B")));
	TestEqual(TEXT("New source tag serial"), NewSpec.SerialNumber, 12);
	TestTrue(TEXT("New source tag records explicit VTC group"), NewSpec.bHasExplicitVTCGroup);

	FEnvBatchSourceTagSpec LegacySpec;
	TestTrue(TEXT("Legacy source tag parses"),
		ParseEnvBatchSourceTag(TEXT("EnvBatch.Source.L1.Prop.Batched.03"), LegacySpec));
	TestEqual(TEXT("Legacy source tag receives default VTC group"), LegacySpec.VTCGroup, GetDefaultEnvBatchVTCGroup());
	TestEqual(TEXT("Legacy source tag serial"), LegacySpec.SerialNumber, 3);
	TestFalse(TEXT("Legacy source tag records no explicit VTC group"), LegacySpec.bHasExplicitVTCGroup);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEnvBatchSourceTagBuildsChannelSplitSharedPropVTCNamesTest,
	"DevKitEditor.MaterialBatch.EnvBatchSourceTag.BuildsChannelSplitSharedPropVTCNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnvBatchSourceTagBuildsChannelSplitSharedPropVTCNamesTest::RunTest(const FString& Parameters)
{
	FEnvBatchSourceTagSpec Spec;
	Spec.LevelName = TEXT("L1_CommonLevel_Corridor_01a");
	Spec.VTCGroup = TEXT("VTC-A");

	TestEqual(TEXT("SharedProp BaseColor VTC collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::BaseColor),
		FString(TEXT("VTC_L1_CommonLevel_Corridor_01a_VTC-A_SharedProp_BaseColor")));

	TestEqual(TEXT("SharedProp Normal VTC collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::Normal),
		FString(TEXT("VTC_L1_CommonLevel_Corridor_01a_VTC-A_SharedProp_Normal")));

	TestEqual(TEXT("SharedProp ORM VTC collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::ORM),
		FString(TEXT("VTC_L1_CommonLevel_Corridor_01a_VTC-A_SharedProp_ORM")));

	TestEqual(TEXT("SharedProp MaterialLight VTC collection name"),
		BuildEnvBatchSharedPropVTCCollectionName(Spec, EEnvBatchVTCChannel::MaterialLight),
		FString(TEXT("VTC_L1_CommonLevel_Corridor_01a_VTC-A_SharedProp_MaterialLight")));

	return true;
}

#endif
