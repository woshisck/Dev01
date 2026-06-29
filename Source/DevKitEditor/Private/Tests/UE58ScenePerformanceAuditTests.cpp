#if WITH_DEV_AUTOMATION_TESTS

#include "Performance/UE58RuntimeProfilingPlan.h"
#include "Performance/UE58ScenePerformanceAudit.h"
#include "UI/GraphicsSettingsWidgetSetupCommandlet.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUE58ScenePerformanceAuditReportTest,
	"DevKitEditor.UE58Performance.SceneAudit.BuildsMarkdownReport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUE58ScenePerformanceAuditReportTest::RunTest(const FString& Parameters)
{
	FUE58ScenePerformanceAuditResult Result;
	Result.MapPath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	Result.bLoaded = true;
	Result.LevelCount = 3;
	Result.ActorCount = 120;
	Result.StaticMeshComponentCount = 65;
	Result.StaticMeshComponentWithMeshCount = 60;
	Result.StaticMeshMaterialSlotCount = 92;
	Result.MovableStaticMeshComponentCount = 4;
	Result.LightComponentCount = 12;
	Result.DirectionalLightCount = 1;
	Result.PointLightCount = 8;
	Result.SpotLightCount = 2;
	Result.RectLightCount = 1;
	Result.MovableLightCount = 5;
	Result.ShadowCastingLightCount = 7;

	const TArray<FString> Lines = FUE58ScenePerformanceAuditBuilder::BuildMarkdownReport(Result);

	TestTrue(TEXT("Report includes heading"), Lines.Contains(TEXT("# UE5.8 Scene Performance Audit")));
	TestTrue(TEXT("Report includes map path"), Lines.Contains(TEXT("- Map: `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`")));
	TestTrue(TEXT("Report includes actor count"), Lines.Contains(TEXT("- Actors: 120")));
	TestTrue(TEXT("Report includes static mesh component count"), Lines.Contains(TEXT("- StaticMesh components: 65")));
	TestTrue(TEXT("Report includes material slot upper-bound"), Lines.Contains(TEXT("- StaticMesh material slot upper-bound: 92")));
	TestTrue(TEXT("Report includes light count"), Lines.Contains(TEXT("- Light components: 12")));
	TestTrue(TEXT("Report includes dynamic light risk"), Lines.Contains(TEXT("- Movable lights: 5")));
	TestTrue(TEXT("Report includes shadow-casting light risk"), Lines.Contains(TEXT("- Shadow-casting lights: 7")));
	TestTrue(TEXT("Report includes batching note"), Lines.Contains(TEXT("- Material slot count is a draw-call pressure proxy before instancing, HLOD, and mesh draw command merging.")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUE58RuntimeProfilingPlanReportTest,
	"DevKitEditor.UE58Performance.RuntimeProfiling.BuildsScenarioMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FUE58RuntimeProfilingPlanReportTest::RunTest(const FString& Parameters)
{
	const TArray<FUE58RuntimeProfilingScenario> Scenarios =
		FUE58RuntimeProfilingPlanBuilder::BuildDefaultScenarios();

	TestEqual(TEXT("Default runtime profiling scenario count"), Scenarios.Num(), 6);
	TestEqual(TEXT("First scenario is the baseline"), Scenarios[0].Name, FString(TEXT("Baseline_LumenOff_NoBatch")));
	TestEqual(TEXT("Lumen Lite scenario targets handheld 15W"), Scenarios[1].Tier, FString(TEXT("Handheld15W")));
	TestTrue(TEXT("Batch + Lumen scenario requires generated proxy visibility"), Scenarios[3].bRequiresBatchProxy);

	FUE58RuntimeProfilingPlanOptions Options;
	Options.MapPath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	Options.ClusterName = TEXT("Prison_S_01");
	Options.CameraLabel = TEXT("WideShot_A");

	const TArray<FString> Lines = FUE58RuntimeProfilingPlanBuilder::BuildMarkdownReport(Options);
	const FString Report = FString::Join(Lines, TEXT("\n"));

	TestTrue(TEXT("Report includes heading"), Lines.Contains(TEXT("# UE5.8 Runtime Profiling Plan")));
	TestTrue(TEXT("Report includes map path"), Lines.Contains(TEXT("- Map: `/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01`")));
	TestTrue(TEXT("Report explicitly marks values as not measured"), Lines.Contains(TEXT("- Evidence status: NotMeasured")));
	TestTrue(TEXT("Report includes Lumen Lite scenario"), Report.Contains(TEXT("LumenLite_NoBatch")));
	TestTrue(TEXT("Report includes batch proxy plus Lumen Lite scenario"), Report.Contains(TEXT("BatchProxy_LumenLite")));
	TestTrue(TEXT("Report includes stat rhi capture command"), Report.Contains(TEXT("stat rhi")));
	TestTrue(TEXT("Report includes profilegpu capture command"), Report.Contains(TEXT("profilegpu")));
	TestTrue(TEXT("Report includes mesh draw call metric"), Report.Contains(TEXT("| Mesh draw calls | NotMeasured |")));
	TestTrue(TEXT("Report includes handheld acceptance check"), Report.Contains(TEXT("Handheld15W passes only if")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGraphicsSettingsWidgetSetupContractTest,
	"DevKitEditor.UI.GraphicsSettingsWidgetSetup.Contract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGraphicsSettingsWidgetSetupContractTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("Graphics settings setup commandlet writes the expected frontend WBP"),
		UGraphicsSettingsWidgetSetupCommandlet::GetTargetWidgetPackagePath(),
		FString(TEXT("/Game/UI/Frontend/WBP_GraphicsSettingsWidget")));
	TestEqual(
		TEXT("Graphics settings setup commandlet writes a commandlet report"),
		UGraphicsSettingsWidgetSetupCommandlet::GetReportFileName(),
		FString(TEXT("GraphicsSettingsWidgetSetupReport.md")));

	return true;
}

#endif
