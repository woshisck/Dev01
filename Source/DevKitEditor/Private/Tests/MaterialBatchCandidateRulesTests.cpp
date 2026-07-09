#if WITH_DEV_AUTOMATION_TESTS

#include "MaterialBatch/MaterialBatchCandidateRules.h"
#include "MaterialBatch/MaterialBatchAuditHelpers.h"
#include "MaterialBatch/MaterialBatchBuildPlan.h"
#include "MaterialBatch/MaterialBatchMaterialAudit.h"
#include "System/MaterialBatchMappingDataAsset.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchCandidateRulesClassifyStaticMeshTest,
	"DevKitEditor.MaterialBatch.CandidateRules.ClassifiesStaticMeshInputs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchCandidateRulesClassifyStaticMeshTest::RunTest(const FString& Parameters)
{
	FMaterialBatchComponentScanInput ValidInput;
	ValidInput.bIsStaticMeshComponent = true;
	ValidInput.bHasStaticMobility = true;
	ValidInput.MaterialSlotCount = 1;
	ValidInput.LodCount = 2;

	const FMaterialBatchCandidateDecision ValidDecision = FMaterialBatchCandidateRules::ClassifyComponent(ValidInput);
	TestTrue(TEXT("Static mesh input with static mobility, material slot, and LOD is eligible"), ValidDecision.bEligible);
	TestEqual(TEXT("Eligible input records the eligible reason"), ValidDecision.Reason, EMaterialBatchCandidateRejectReason::None);

	FMaterialBatchComponentScanInput DynamicInput = ValidInput;
	DynamicInput.bHasStaticMobility = false;
	const FMaterialBatchCandidateDecision DynamicDecision = FMaterialBatchCandidateRules::ClassifyComponent(DynamicInput);
	TestFalse(TEXT("Dynamic mobility input is rejected"), DynamicDecision.bEligible);
	TestEqual(TEXT("Dynamic mobility records the expected reason"), DynamicDecision.Reason, EMaterialBatchCandidateRejectReason::DynamicMobility);

	FMaterialBatchComponentScanInput NoMaterialInput = ValidInput;
	NoMaterialInput.MaterialSlotCount = 0;
	const FMaterialBatchCandidateDecision NoMaterialDecision = FMaterialBatchCandidateRules::ClassifyComponent(NoMaterialInput);
	TestFalse(TEXT("Static mesh input without material slots is rejected"), NoMaterialDecision.bEligible);
	TestEqual(TEXT("Missing material slots record the expected reason"), NoMaterialDecision.Reason, EMaterialBatchCandidateRejectReason::NoMaterialSlots);

	FMaterialBatchComponentScanInput ExcludedInput = ValidInput;
	ExcludedInput.bHasExplicitExcludeTag = true;
	const FMaterialBatchCandidateDecision ExcludedDecision = FMaterialBatchCandidateRules::ClassifyComponent(ExcludedInput);
	TestFalse(TEXT("Explicit exclude tag rejects otherwise valid input"), ExcludedDecision.bEligible);
	TestEqual(TEXT("Explicit exclude tag records the expected reason"), ExcludedDecision.Reason, EMaterialBatchCandidateRejectReason::ExplicitExcludeTag);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchCandidateRulesClassifyActorTagsTest,
	"DevKitEditor.MaterialBatch.CandidateRules.ClassifiesActorTags",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchCandidateRulesClassifyActorTagsTest::RunTest(const FString& Parameters)
{
	FMaterialBatchComponentScanInput BaseInput;
	BaseInput.bIsStaticMeshComponent = true;
	BaseInput.bHasStaticMobility = true;
	BaseInput.MaterialSlotCount = 1;
	BaseInput.LodCount = 1;

	const FMaterialBatchComponentScanInput ExcludeInput =
		FMaterialBatchCandidateRules::BuildInputFromTags(BaseInput, { FName(TEXT("EnvBatch.Exclude")) });
	const FMaterialBatchCandidateDecision ExcludeDecision = FMaterialBatchCandidateRules::ClassifyComponent(ExcludeInput);
	TestFalse(TEXT("EnvBatch.Exclude rejects otherwise valid input"), ExcludeDecision.bEligible);
	TestEqual(TEXT("EnvBatch.Exclude records explicit exclude"), ExcludeDecision.Reason, EMaterialBatchCandidateRejectReason::ExplicitExcludeTag);

	const FMaterialBatchComponentScanInput GameplayInput =
		FMaterialBatchCandidateRules::BuildInputFromTags(BaseInput, { FName(TEXT("GameplayCritical")) });
	const FMaterialBatchCandidateDecision GameplayDecision = FMaterialBatchCandidateRules::ClassifyComponent(GameplayInput);
	TestFalse(TEXT("GameplayCritical rejects otherwise valid input"), GameplayDecision.bEligible);
	TestEqual(TEXT("GameplayCritical records gameplay critical reason"), GameplayDecision.Reason, EMaterialBatchCandidateRejectReason::GameplayCriticalTag);

	const FMaterialBatchComponentScanInput InteractiveInput =
		FMaterialBatchCandidateRules::BuildInputFromTags(BaseInput, { FName(TEXT("Interactive")) });
	const FMaterialBatchCandidateDecision InteractiveDecision = FMaterialBatchCandidateRules::ClassifyComponent(InteractiveInput);
	TestFalse(TEXT("Interactive rejects otherwise valid input"), InteractiveDecision.bEligible);
	TestEqual(TEXT("Interactive records interactive reason"), InteractiveDecision.Reason, EMaterialBatchCandidateRejectReason::InteractiveTag);

	const FMaterialBatchComponentScanInput IncludeInput =
		FMaterialBatchCandidateRules::BuildInputFromTags(BaseInput, { FName(TEXT("EnvBatch.Include")) });
	const FMaterialBatchCandidateDecision IncludeDecision = FMaterialBatchCandidateRules::ClassifyComponent(IncludeInput);
	TestTrue(TEXT("EnvBatch.Include remains eligible when no reject tags exist"), IncludeDecision.bEligible);
	TestTrue(TEXT("EnvBatch.Include records include intent"), IncludeInput.bHasExplicitIncludeTag);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchAuditHelpersMapFilenameTest,
	"DevKitEditor.MaterialBatch.AuditHelpers.ResolvesLongPackageMapFilename",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchAuditHelpersMapFilenameTest::RunTest(const FString& Parameters)
{
	const FString ExistingMapPackageName = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	const FString MapFilename = FMaterialBatchAuditHelpers::ResolveMapFilename(ExistingMapPackageName);

	TestTrue(TEXT("Existing map package resolves to a .umap filename"), MapFilename.EndsWith(TEXT(".umap")));
	TestTrue(TEXT("Existing map filename points at a file on disk"), FPaths::FileExists(MapFilename));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanDryRunNamesTest,
	"DevKitEditor.MaterialBatch.BuildPlan.CreatesDryRunGeneratedAssetNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanDryRunNamesTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.MapPath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	Options.RootPath = TEXT("/Game/Art");
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");
	Options.TextureBackend = TEXT("VTAtlas");
	Options.SurfaceKind = TEXT("Wall");
	Options.BakePolicy = TEXT("StaticBake");
	Options.SourceProxyExclusivityGroup = TEXT("Prison_S_01_SourceProxy");
	Options.OutputRoot = TEXT("/Game/Generated/MaterialBatch");
	Options.bDryRun = true;

	const FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	TestTrue(TEXT("Plan remains dry-run"), Plan.bDryRun);
	TestEqual(TEXT("Cluster name is sanitized for packages"), Plan.SanitizedClusterName, FString(TEXT("Prison_S_01")));
	TestEqual(
		TEXT("Proxy mesh output package is deterministic"),
		Plan.ProxyMeshPackage,
		FString(TEXT("/Game/Generated/MaterialBatch/Mid/Prison_S_01/SM_BatchProxy_Prison_S_01")));
	TestEqual(
		TEXT("Property texture output package is deterministic"),
		Plan.PropertyTexturePackage,
		FString(TEXT("/Game/Generated/MaterialBatch/Mid/Prison_S_01/T_PropTexture_Prison_S_01")));
	TestEqual(
		TEXT("Batch parent material path is explicit"),
		Plan.BatchParentMaterialPackage,
		FString(TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas.M_Env_Baked_VTAtlas")));
	TestEqual(
		TEXT("Mask Texture2DArray output package is deterministic"),
		Plan.MaskArrayPackage,
		FString(TEXT("/Game/Generated/MaterialBatch/Mid/Prison_S_01/T2DA_Mask_Prison_S_01")));
	TestEqual(TEXT("Texture backend is recorded"), Plan.TextureBackend, FString(TEXT("VTAtlas")));
	TestEqual(TEXT("Surface kind is recorded"), Plan.SurfaceKind, FString(TEXT("Wall")));
	TestEqual(TEXT("Bake policy is recorded"), Plan.BakePolicy, FString(TEXT("StaticBake")));
	TestEqual(TEXT("Source/proxy exclusivity group is sanitized"), Plan.SourceProxyExclusivityGroup, FString(TEXT("Prison_S_01_SourceProxy")));
	TestEqual(TEXT("Layer backend defaults to streaming level"), Plan.SourceProxyLayerPlan.LayerBackend, FString(TEXT("StreamingLevel")));
	TestEqual(TEXT("Source layer name falls back before map scan"), Plan.SourceProxyLayerPlan.SourceLayerName, FString(TEXT("CurrentStreamingLevel")));
	TestEqual(TEXT("Proxy layer name is deterministic"), Plan.SourceProxyLayerPlan.ProxyLayerName, FString(TEXT("SL_Prison_S_01_SourceProxy_Proxy")));
	TestEqual(TEXT("Baked layer name is deterministic"), Plan.SourceProxyLayerPlan.BakedLayerName, FString(TEXT("SL_Prison_S_01_SourceProxy_Baked")));
	TestEqual(TEXT("Layer plan contains four performance tiers"), Plan.SourceProxyLayerPlan.TierSelections.Num(), 4);
	TestTrue(TEXT("Mid tier prefers proxy layer"), Plan.SourceProxyLayerPlan.TierSelections[2].bLoadProxyLayer);
	TestTrue(TEXT("Low tier prefers baked layer"), Plan.SourceProxyLayerPlan.TierSelections[3].bLoadBakedLayer);
	TestEqual(
		TEXT("VT atlas package is deterministic"),
		Plan.VTAtlasPackage,
		FString(TEXT("/Game/Generated/MaterialBatch/Mid/Prison_S_01/VT_Atlas_Prison_S_01")));

	const TArray<FString> ReportLines = FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(Plan);
	TestTrue(TEXT("Report states that no source assets are modified"), ReportLines.Contains(TEXT("- Mode: dry-run; no assets are modified or generated.")));
	TestTrue(TEXT("Report lists MaterialBatchBuild commandlet name"), ReportLines.Contains(TEXT("# Material Batch Build Plan")));
	TestTrue(TEXT("Report documents batch parent material contract"), ReportLines.Contains(TEXT("## Batch Material Parent Contract")));
	TestTrue(TEXT("Report documents source/proxy/baked layer plan"), ReportLines.Contains(TEXT("## Source/Proxy/Baked Layer Plan")));
	TestTrue(TEXT("Report documents Mid tier proxy preference"), ReportLines.Contains(TEXT("| Mid | No | Yes | No | Prefer generated Proxy layer with VT atlas batch material. |")));
	TestTrue(TEXT("Report documents source/proxy/baked layer readiness"), ReportLines.Contains(TEXT("## Source/Proxy/Baked Layer Readiness")));
	TestTrue(TEXT("Report documents property texture parameter"), ReportLines.Contains(TEXT("- Property texture parameter: `_PropTexture`")));
	TestTrue(TEXT("Report documents Texture2DArray partial apply"), ReportLines.Contains(TEXT("- `-ApplyTextureArraysOnly` writes Texture2DArray assets from eligible slice plans.")));
	TestTrue(TEXT("Report documents proxy mesh partial apply"), ReportLines.Contains(TEXT("- `-ApplyProxyMeshOnly` writes a LOD0 merged proxy StaticMesh with UV7.x batchMaterialIndex data.")));
	TestTrue(TEXT("Report limits remaining scope to full apply and map replacement"), ReportLines.Contains(TEXT("- Full `-Apply` and map actor replacement remain disabled until generated proxy meshes are reviewed.")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanCandidateSummaryTest,
	"DevKitEditor.MaterialBatch.BuildPlan.ReportsCandidateSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanCandidateSummaryTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison_S_01");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildCandidateSummary Summary;
	Summary.SourceKind = TEXT("MapComponent");
	Summary.SourceFoundCount = 5;
	Summary.SourceInspectedCount = 4;
	Summary.BatchCandidateCount = 3;
	Summary.RejectedCount = 1;

	FMaterialBatchBuildPlanBuilder::ApplyCandidateSummary(Plan, Summary);

	TestEqual(TEXT("Plan stores candidate count"), Plan.CandidateSummary.BatchCandidateCount, 3);
	TestEqual(TEXT("Plan stores rejected count"), Plan.CandidateSummary.RejectedCount, 1);

	const TArray<FString> ReportLines = FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(Plan);
	TestTrue(TEXT("Report includes source kind"), ReportLines.Contains(TEXT("- Source kind: MapComponent")));
	TestTrue(TEXT("Report includes candidate count"), ReportLines.Contains(TEXT("- Batch candidates: 3")));
	TestTrue(TEXT("Report includes rejected count"), ReportLines.Contains(TEXT("- Rejected: 1")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanEntriesTest,
	"DevKitEditor.MaterialBatch.BuildPlan.ReportsPlannedEntriesAndMaterialIndices",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanEntriesTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison_S_01");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry CandidateEntry;
	CandidateEntry.SourceKind = TEXT("MapComponent");
	CandidateEntry.SourcePath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActorName = TEXT("Wall_A");
	CandidateEntry.ComponentName = TEXT("StaticMeshComponent0");
	CandidateEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	CandidateEntry.bHasWorldTransform = true;
	CandidateEntry.WorldLocation = FVector(100.0, 200.0, 300.0);
	CandidateEntry.WorldRotation = FRotator(0.0, 90.0, 0.0);
	CandidateEntry.WorldScale = FVector(1.0, 2.0, 1.0);
	CandidateEntry.MaterialSlotCount = 2;
	CandidateEntry.MaterialSlotNames = { TEXT("WallBase"), TEXT("WallTrim") };
	CandidateEntry.MaterialPaths = {
		TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase"),
		TEXT("/Game/Art/Materials/MI_WallTrim.MI_WallTrim")
	};
	CandidateEntry.EnvBatchTags = { TEXT("EnvBatch.Source.LOD0"), TEXT("EnvBatch.BakeStaticDecal.Wall") };
	CandidateEntry.ActualStreamingLevelName = TEXT("L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActualLevelPackageName = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActualLayerNames = {
		TEXT("L1_CommonLevel_Prison_S_01"),
		TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01")
	};
	CandidateEntry.LodCount = 3;
	CandidateEntry.bCandidate = true;

	FMaterialBatchBuildPlannedEntry RejectedEntry;
	RejectedEntry.SourceKind = TEXT("MapComponent");
	RejectedEntry.ActorName = TEXT("Door_A");
	RejectedEntry.ComponentName = TEXT("StaticMeshComponent0");
	RejectedEntry.AssetPath = TEXT("(no static mesh)");
	RejectedEntry.bCandidate = false;
	RejectedEntry.RejectReason = TEXT("NotStaticMeshComponent");

	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { CandidateEntry, RejectedEntry });

	TestEqual(TEXT("Plan stores planned entries"), Plan.PlannedEntries.Num(), 2);
	TestEqual(TEXT("Candidate entry starts at material index zero"), Plan.PlannedEntries[0].FirstBatchMaterialIndex, 0);
	TestEqual(TEXT("Candidate entry reserves one index per material slot"), Plan.PlannedEntries[0].BatchMaterialIndexCount, 2);
	TestEqual(TEXT("Rejected entry does not reserve material indices"), Plan.PlannedEntries[1].BatchMaterialIndexCount, 0);
	TestEqual(TEXT("Plan records next material index"), Plan.NextBatchMaterialIndex, 2);
	TestEqual(TEXT("Plan creates one material row per candidate material slot"), Plan.PlannedMaterialRows.Num(), 2);
	TestEqual(TEXT("First material row uses first batch index"), Plan.PlannedMaterialRows[0].BatchMaterialIndex, 0);
	TestEqual(TEXT("Second material row uses next batch index"), Plan.PlannedMaterialRows[1].BatchMaterialIndex, 1);
	TestEqual(TEXT("Material rows reference their source entry"), Plan.PlannedMaterialRows[0].SourceEntryIndex, 0);
	TestEqual(TEXT("Material rows preserve slot indices"), Plan.PlannedMaterialRows[1].MaterialSlotIndex, 1);
	TestEqual(TEXT("Material rows preserve slot names"), Plan.PlannedMaterialRows[0].MaterialSlotName, FString(TEXT("WallBase")));
	TestEqual(
		TEXT("Material rows preserve material paths"),
		Plan.PlannedMaterialRows[1].MaterialPath,
		FString(TEXT("/Game/Art/Materials/MI_WallTrim.MI_WallTrim")));

	FMaterialBatchBuildTextureChannelPlan BaseColorChannel;
	BaseColorChannel.ChannelName = TEXT("BaseColor");
	BaseColorChannel.ParameterName = TEXT("BaseMap");
	BaseColorChannel.TexturePath = TEXT("/Game/Art/Textures/T_Wall_D.T_Wall_D");
	BaseColorChannel.TextureClass = TEXT("Texture2D");
	BaseColorChannel.bFoundTexture = true;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(BaseColorChannel);

	const TArray<FString> ReportLines = FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(Plan);
	TestTrue(TEXT("Report includes planned entry table header"), ReportLines.Contains(TEXT("## Planned Batch Entries")));
	TestTrue(TEXT("Report includes geometry merge plan section"), ReportLines.Contains(TEXT("## Geometry Merge Plan")));
	TestTrue(TEXT("Report includes material slot remap section"), ReportLines.Contains(TEXT("## Material Slot Remap")));
	TestTrue(
		TEXT("Report includes merge source transform"),
		ReportLines.Contains(TEXT("| 0 | `Wall_A` | `StaticMeshComponent0` | `/Game/Art/Env/SM_Wall_A.SM_Wall_A` | No | 100.000,200.000,300.000 | 0.000,90.000,0.000 | 1.000,2.000,1.000 | 0 | 2 |")));
	TestTrue(
		TEXT("Report includes material slot to batch index remap"),
		ReportLines.Contains(TEXT("| 0 | 1 | 1 |")));
	TestTrue(TEXT("Report includes planned material row table header"), ReportLines.Contains(TEXT("## Planned Material Rows")));
	TestTrue(TEXT("Report includes texture channel table header"), ReportLines.Contains(TEXT("## Planned Texture Channels")));
	TestTrue(TEXT("Report includes actual layer readiness count"), ReportLines.Contains(TEXT("- Actual layer matches: 1")));
	TestTrue(TEXT("Report includes matched actual layer status"), ReportLines.Contains(TEXT("| 0 | `Wall_A` | `StaticMeshComponent0` | Source | `L1_CommonLevel_Prison_S_01` | L1_CommonLevel_Prison_S_01, /Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01 | `L1_CommonLevel_Prison_S_01` | Yes | Yes | MatchedExpectedLayer | Ready to validate against Source layer. | EnvBatch.Source.LOD0, EnvBatch.BakeStaticDecal.Wall |")));
	TestTrue(TEXT("Report documents source/proxy asset readiness"), ReportLines.Contains(TEXT("## Source/Proxy Asset Readiness")));
	TestTrue(TEXT("Report documents default source LOD0"), ReportLines.Contains(TEXT("- Default Source LOD index: 0")));
	TestTrue(TEXT("Report documents default proxy LOD1"), ReportLines.Contains(TEXT("- Default Proxy LOD index: 1")));
	TestTrue(TEXT("Report records generated proxy fallback asset pair"), ReportLines.Contains(TEXT("| 0 | `Wall_A` | `StaticMeshComponent0` | Source | `/Game/Art/Env/SM_Wall_A.SM_Wall_A` | `/Game/Generated/MaterialBatch/Mid/Prison_S_01/SM_BatchProxy_Prison_S_01` | 0 | 1 | Yes | No | Yes | ReadyGeneratedProxy | Source defaults to LOD0 and generated cluster proxy defaults to LOD1 for asset pairing. | EnvBatch.Source.LOD0, EnvBatch.BakeStaticDecal.Wall |")));
	TestTrue(
		TEXT("Report includes texture channel row"),
		ReportLines.Contains(TEXT("| 0 | BaseColor | `BaseMap` | `/Game/Art/Textures/T_Wall_D.T_Wall_D` | Texture2D | Yes |")));
	TestTrue(
		TEXT("Report includes candidate material index range"),
		ReportLines.Contains(TEXT("| MapComponent | `Wall_A` | `StaticMeshComponent0` | `L1_CommonLevel_Prison_S_01` | `/Game/Art/Env/SM_Wall_A.SM_Wall_A` | 2 | 3 | EnvBatch.Source.LOD0, EnvBatch.BakeStaticDecal.Wall | Candidate | None | 0 | 2 |")));
	TestTrue(
		TEXT("Report includes rejected entry reason"),
		ReportLines.Contains(TEXT("| MapComponent | `Door_A` | `StaticMeshComponent0` | `` | `(no static mesh)` | 0 | 0 |  | Rejected | NotStaticMeshComponent | -1 | 0 |")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanGroundSourcesBakeVertexColorsTest,
	"DevKitEditor.MaterialBatch.BuildPlan.GroundSourcesBakeVertexColors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanGroundSourcesBakeVertexColorsTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Ground_Batch");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry GroundEntry;
	GroundEntry.SourceKind = TEXT("MapComponent");
	GroundEntry.ActorName = TEXT("Ground_A");
	GroundEntry.ComponentName = TEXT("StaticMeshComponent0");
	GroundEntry.AssetPath = TEXT("/Game/Art/Env/SM_Ground_A.SM_Ground_A");
	GroundEntry.MaterialSlotCount = 1;
	GroundEntry.MaterialSlotNames = { TEXT("Ground") };
	GroundEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_Ground.MI_Ground") };
	GroundEntry.EnvBatchTags = { TEXT("EnvBatch.Source.L1_Test.Ground.Batched.Ground.01") };
	GroundEntry.bCandidate = true;

	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { GroundEntry });

	const FMaterialBatchBuildProxyMeshPayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildProxyMeshPayload(Plan);
	TestEqual(TEXT("Proxy payload has one source"), Payload.Sources.Num(), 1);
	TestTrue(TEXT("Ground.Batched source bakes component vertex colors"),
		Payload.Sources.IsValidIndex(0) && Payload.Sources[0].bBakeInstanceVertexColors);

	const TArray<FString> ReportLines = FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(Plan);
	TestTrue(TEXT("Report marks ground source vertex color bake"),
		ReportLines.Contains(TEXT("| 0 | `Ground_A` | `StaticMeshComponent0` | `/Game/Art/Env/SM_Ground_A.SM_Ground_A` | Yes | 0.000,0.000,0.000 | 0.000,0.000,0.000 | 1.000,1.000,1.000 | 0 | 1 |")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanManifestTest,
	"DevKitEditor.MaterialBatch.BuildPlan.CreatesJsonManifest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanManifestTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.RootPath = TEXT("/Game/Art");
	Options.MapPath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");
	Options.TextureBackend = TEXT("VTAtlas");
	Options.SurfaceKind = TEXT("Wall");
	Options.BakePolicy = TEXT("StaticBake");
	Options.SourceProxyExclusivityGroup = TEXT("Prison S 01 SourceProxy");
	Options.bValidateSourceProxyExclusivity = true;
	Options.OutputRoot = TEXT("/Game/Generated/MaterialBatch");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildCandidateSummary Summary;
	Summary.SourceKind = TEXT("MapComponent");
	Summary.SourceFoundCount = 2;
	Summary.SourceInspectedCount = 2;
	Summary.BatchCandidateCount = 1;
	Summary.RejectedCount = 1;
	FMaterialBatchBuildPlanBuilder::ApplyCandidateSummary(Plan, Summary);

	FMaterialBatchBuildPlannedEntry CandidateEntry;
	CandidateEntry.SourceKind = TEXT("MapComponent");
	CandidateEntry.SourcePath = Options.MapPath;
	CandidateEntry.ActorName = TEXT("Wall_A");
	CandidateEntry.ComponentName = TEXT("StaticMeshComponent0");
	CandidateEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	CandidateEntry.bHasWorldTransform = true;
	CandidateEntry.WorldLocation = FVector(100.0, 200.0, 300.0);
	CandidateEntry.WorldRotation = FRotator(0.0, 90.0, 0.0);
	CandidateEntry.WorldScale = FVector(1.0, 2.0, 1.0);
	CandidateEntry.MaterialSlotCount = 2;
	CandidateEntry.MaterialSlotNames = { TEXT("WallBase"), TEXT("WallTrim") };
	CandidateEntry.MaterialPaths = {
		TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase"),
		TEXT("/Game/Art/Materials/MI_WallTrim.MI_WallTrim")
	};
	CandidateEntry.EnvBatchTags = { TEXT("EnvBatch.Source.LOD0"), TEXT("EnvBatch.BakeStaticDecal.Wall") };
	CandidateEntry.ActualStreamingLevelName = TEXT("L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActualLevelPackageName = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActualLayerNames = {
		TEXT("L1_CommonLevel_Prison_S_01"),
		TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01")
	};
	CandidateEntry.LodCount = 3;
	CandidateEntry.bCandidate = true;

	FMaterialBatchBuildPlannedEntry RejectedEntry;
	RejectedEntry.SourceKind = TEXT("MapComponent");
	RejectedEntry.SourcePath = Options.MapPath;
	RejectedEntry.ActorName = TEXT("Door_A");
	RejectedEntry.ComponentName = TEXT("StaticMeshComponent0");
	RejectedEntry.AssetPath = TEXT("(no static mesh)");
	RejectedEntry.EnvBatchTags = { TEXT("EnvBatch.Source.LOD0"), TEXT("EnvBatch.Proxy.Mid") };
	RejectedEntry.bCandidate = false;
	RejectedEntry.RejectReason = TEXT("NotStaticMeshComponent");

	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { CandidateEntry, RejectedEntry });
	Plan.TagDiagnostics.ActorCount = 2;
	Plan.TagDiagnostics.SourceActorCount = 1;
	Plan.TagDiagnostics.ProxyActorCount = 1;
	Plan.TagDiagnostics.BakedActorCount = 1;
	Plan.TagDiagnostics.BakeStaticDecalActorCount = 1;
	Plan.TagDiagnostics.SourceProxyConflictActorCount = 1;
	Plan.TagDiagnostics.Warnings.Add(TEXT("Source/Proxy exclusivity conflict in group Prison_S_01_SourceProxy: actor `Wall_A` has EnvBatch.Source.LOD0, EnvBatch.Proxy.Mid"));

	const FString Manifest = FMaterialBatchBuildPlanBuilder::BuildJsonManifest(Plan);
	TestTrue(TEXT("Manifest contains sanitized cluster"), Manifest.Contains(TEXT("\"cluster\":\"Prison_S_01\"")));
	TestTrue(TEXT("Manifest contains tier"), Manifest.Contains(TEXT("\"tier\":\"Mid\"")));
	TestTrue(TEXT("Manifest contains texture backend"), Manifest.Contains(TEXT("\"textureBackend\":\"VTAtlas\"")));
	TestTrue(TEXT("Manifest contains surface kind"), Manifest.Contains(TEXT("\"surfaceKind\":\"Wall\"")));
	TestTrue(TEXT("Manifest contains bake policy"), Manifest.Contains(TEXT("\"bakePolicy\":\"StaticBake\"")));
	TestTrue(TEXT("Manifest contains source/proxy exclusivity group"), Manifest.Contains(TEXT("\"sourceProxyExclusivityGroup\":\"Prison_S_01_SourceProxy\"")));
	TestTrue(TEXT("Manifest contains tag diagnostics"), Manifest.Contains(TEXT("\"tagDiagnostics\":")));
	TestTrue(TEXT("Manifest records source actor count"), Manifest.Contains(TEXT("\"sourceActorCount\":1")));
	TestTrue(TEXT("Manifest records source/proxy conflicts"), Manifest.Contains(TEXT("\"sourceProxyConflictActorCount\":1")));
	TestTrue(TEXT("Manifest contains source/proxy layer plan"), Manifest.Contains(TEXT("\"sourceProxyLayerPlan\":")));
	TestTrue(TEXT("Manifest records streaming layer backend"), Manifest.Contains(TEXT("\"layerBackend\":\"StreamingLevel\"")));
	TestTrue(TEXT("Manifest records source layer name"), Manifest.Contains(TEXT("\"sourceLayer\":\"L1_CommonLevel_Prison_S_01\"")));
	TestTrue(TEXT("Manifest records proxy layer name"), Manifest.Contains(TEXT("\"proxyLayer\":\"SL_Prison_S_01_SourceProxy_Proxy\"")));
	TestTrue(TEXT("Manifest records baked layer name"), Manifest.Contains(TEXT("\"bakedLayer\":\"SL_Prison_S_01_SourceProxy_Baked\"")));
	TestTrue(TEXT("Manifest records layer plan conflict gate"), Manifest.Contains(TEXT("\"hasTagConflicts\":true")));
	TestTrue(TEXT("Manifest records Mid tier proxy selection"), Manifest.Contains(TEXT("\"tier\":\"Mid\",\"loadSource\":false,\"loadProxy\":true,\"loadBaked\":false")));
	TestTrue(TEXT("Manifest records Low tier baked selection"), Manifest.Contains(TEXT("\"tier\":\"Low\",\"loadSource\":false,\"loadProxy\":false,\"loadBaked\":true")));
	TestTrue(TEXT("Manifest contains source/proxy layer readiness"), Manifest.Contains(TEXT("\"sourceProxyLayerReadiness\":")));
	TestTrue(TEXT("Manifest records layer readiness entry count"), Manifest.Contains(TEXT("\"entryCount\":2")));
	TestTrue(TEXT("Manifest records layer readiness conflict count"), Manifest.Contains(TEXT("\"conflictEntryCount\":1")));
	TestTrue(TEXT("Manifest records layer readiness actual match count"), Manifest.Contains(TEXT("\"actualLayerMatchCount\":1")));
	TestTrue(TEXT("Manifest records layer readiness actual not required count"), Manifest.Contains(TEXT("\"notRequiredActualLayerCount\":1")));
	TestTrue(TEXT("Manifest records conflict assignment role"), Manifest.Contains(TEXT("\"layerRole\":\"Conflict\"")));
	TestTrue(TEXT("Manifest records source expected layer assignment"), Manifest.Contains(TEXT("\"expectedLayer\":\"L1_CommonLevel_Prison_S_01\"")));
	TestTrue(TEXT("Manifest records actual layer assignment"), Manifest.Contains(TEXT("\"actualLayers\":[\"L1_CommonLevel_Prison_S_01\",\"/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01\"]")));
	TestTrue(TEXT("Manifest records actual streaming level"), Manifest.Contains(TEXT("\"actualStreamingLevel\":\"L1_CommonLevel_Prison_S_01\"")));
	TestTrue(TEXT("Manifest records matched layer status"), Manifest.Contains(TEXT("\"layerValidationStatus\":\"MatchedExpectedLayer\"")));
	TestTrue(TEXT("Manifest contains source/proxy asset readiness"), Manifest.Contains(TEXT("\"sourceProxyAssetReadiness\":")));
	TestTrue(TEXT("Manifest records generated proxy fallback count"), Manifest.Contains(TEXT("\"generatedProxyFallbackCount\":1")));
	TestTrue(TEXT("Manifest records asset readiness conflict count"), Manifest.Contains(TEXT("\"conflictCount\":1")));
	TestTrue(TEXT("Manifest records default source LOD index"), Manifest.Contains(TEXT("\"sourceLODIndex\":0")));
	TestTrue(TEXT("Manifest records default proxy LOD index"), Manifest.Contains(TEXT("\"proxyLODIndex\":1")));
	TestTrue(TEXT("Manifest records generated proxy fallback status"), Manifest.Contains(TEXT("\"readinessStatus\":\"ReadyGeneratedProxy\"")));
	TestTrue(TEXT("Manifest records source asset path"), Manifest.Contains(TEXT("\"sourceAsset\":\"/Game/Art/Env/SM_Wall_A.SM_Wall_A\"")));
	TestTrue(TEXT("Manifest records proxy asset path"), Manifest.Contains(TEXT("\"proxyAsset\":\"/Game/Generated/MaterialBatch/Mid/Prison_S_01/SM_BatchProxy_Prison_S_01\"")));
	TestTrue(TEXT("Manifest contains source/proxy asset config set"), Manifest.Contains(TEXT("\"sourceProxyAssetConfigSet\":")));
	TestTrue(TEXT("Manifest records generated fallback config count"), Manifest.Contains(TEXT("\"generatedFallbackConfigCount\":1")));
	TestTrue(TEXT("Manifest records optional explicit proxy config count"), Manifest.Contains(TEXT("\"authoredProxyConfigCount\":0")));
	TestTrue(TEXT("Manifest records config source"), Manifest.Contains(TEXT("\"configSource\":\"GeneratedFallback\"")));
	TestTrue(TEXT("Manifest records generated proxy asset path"), Manifest.Contains(TEXT("\"generatedProxyAsset\":\"/Game/Generated/MaterialBatch/Mid/Prison_S_01/SM_BatchProxy_Prison_S_01\"")));
	TestTrue(TEXT("Manifest records config interaction policy"), Manifest.Contains(TEXT("\"interactionPolicy\":\"StaticBatchCandidate\"")));
	TestTrue(TEXT("Manifest records static decal bake count"), Manifest.Contains(TEXT("\"bakeStaticDecalActorCount\":1")));
	TestTrue(TEXT("Manifest records tag diagnostic warning"), Manifest.Contains(TEXT("Source/Proxy exclusivity conflict")));
	TestTrue(TEXT("Manifest records entry EnvBatch tags"), Manifest.Contains(TEXT("\"envBatchTags\":[\"EnvBatch.Source.LOD0\",\"EnvBatch.BakeStaticDecal.Wall\"]")));
	TestTrue(TEXT("Manifest records entry actual layers"), Manifest.Contains(TEXT("\"actualLayers\":[\"L1_CommonLevel_Prison_S_01\",\"/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01\"]")));
	TestTrue(TEXT("Manifest contains proxy package"), Manifest.Contains(TEXT("\"proxyMesh\":\"/Game/Generated/MaterialBatch/Mid/Prison_S_01/SM_BatchProxy_Prison_S_01\"")));
	TestTrue(TEXT("Manifest contains VT atlas package"), Manifest.Contains(TEXT("\"vtAtlas\":\"/Game/Generated/MaterialBatch/Mid/Prison_S_01/VT_Atlas_Prison_S_01\"")));
	TestTrue(TEXT("Manifest contains batch parent material package"), Manifest.Contains(TEXT("\"batchParentMaterial\":\"/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas.M_Env_Baked_VTAtlas\"")));
	TestTrue(TEXT("Manifest contains batch material contract"), Manifest.Contains(TEXT("\"batchMaterialContract\":")));
	TestTrue(TEXT("Manifest records property texture parameter"), Manifest.Contains(TEXT("\"propertyTextureParameter\":\"_PropTexture\"")));
	TestTrue(TEXT("Manifest contains next material index"), Manifest.Contains(TEXT("\"nextBatchMaterialIndex\":2")));
	TestTrue(TEXT("Manifest contains candidate material range"), Manifest.Contains(TEXT("\"firstBatchMaterialIndex\":0")));
	TestTrue(TEXT("Manifest contains rejected entry reason"), Manifest.Contains(TEXT("\"rejectReason\":\"NotStaticMeshComponent\"")));
	TestTrue(TEXT("Manifest contains geometry merge plan"), Manifest.Contains(TEXT("\"geometryMergePlan\":")));
	TestTrue(TEXT("Manifest records proxy mesh merge target"), Manifest.Contains(TEXT("\"targetProxyMesh\":\"/Game/Generated/MaterialBatch/Mid/Prison_S_01/SM_BatchProxy_Prison_S_01\"")));
	TestTrue(TEXT("Manifest records material index channel"), Manifest.Contains(TEXT("\"materialIndexChannel\":\"TexCoord7.x\"")));
	TestTrue(TEXT("Manifest records merge source count"), Manifest.Contains(TEXT("\"sourceCount\":1")));
	TestTrue(TEXT("Manifest records merge source static mesh"), Manifest.Contains(TEXT("\"staticMesh\":\"/Game/Art/Env/SM_Wall_A.SM_Wall_A\"")));
	TestTrue(TEXT("Manifest records merge source transform"), Manifest.Contains(TEXT("\"worldLocation\":{\"x\":100")));
	TestTrue(TEXT("Manifest records merge source rotation"), Manifest.Contains(TEXT("\"worldRotation\":{\"pitch\":0,\"yaw\":90,\"roll\":0}")));
	TestTrue(TEXT("Manifest records material slot remap"), Manifest.Contains(TEXT("\"materialSlotRemap\":[{\"sourceMaterialSlotIndex\":0,\"batchMaterialIndex\":0},{\"sourceMaterialSlotIndex\":1,\"batchMaterialIndex\":1}]")));
	TestTrue(TEXT("Manifest contains material rows array"), Manifest.Contains(TEXT("\"materialRows\":")));
	TestTrue(TEXT("Manifest contains first material row index"), Manifest.Contains(TEXT("\"batchMaterialIndex\":0")));
	TestTrue(TEXT("Manifest records material slot index zero"), Manifest.Contains(TEXT("\"materialSlotIndex\":0")));
	TestTrue(TEXT("Manifest records material slot index one"), Manifest.Contains(TEXT("\"materialSlotIndex\":1")));
	TestTrue(TEXT("Manifest links material rows to source entry"), Manifest.Contains(TEXT("\"sourceEntryIndex\":0")));
	TestTrue(TEXT("Manifest records material slot name"), Manifest.Contains(TEXT("\"materialSlotName\":\"WallBase\"")));
	TestTrue(TEXT("Manifest records material path"), Manifest.Contains(TEXT("\"material\":\"/Game/Art/Materials/MI_WallTrim.MI_WallTrim\"")));
	TestFalse(TEXT("Manifest does not create material rows for rejected entries"), Manifest.Contains(TEXT("\"sourceEntryIndex\":1,\"materialSlotIndex\":0")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanTextureChannelsTest,
	"DevKitEditor.MaterialBatch.BuildPlan.ClassifiesAndReportsTextureChannels",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanTextureChannelsTest::RunTest(const FString& Parameters)
{
	TestEqual(
		TEXT("BaseMap is classified as BaseColor"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("BaseMap")),
		FString(TEXT("BaseColor")));
	TestEqual(
		TEXT("Albedo is classified as BaseColor"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("Albedo")),
		FString(TEXT("BaseColor")));
	TestEqual(
		TEXT("M_Env_Building unique color map is classified as BaseColor"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T Unique Color Map")),
		FString(TEXT("BaseColor")));
	TestEqual(
		TEXT("Project T_Color parameter is classified as BaseColor"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_Color")),
		FString(TEXT("BaseColor")));
	TestEqual(
		TEXT("M_Env_Building array albedo is classified as BaseColor"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_Array_A")),
		FString(TEXT("BaseColor")));
	TestEqual(
		TEXT("NormalMap is classified as Normal"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("NormalMap")),
		FString(TEXT("Normal")));
	TestEqual(
		TEXT("M_Env_Building array normal is classified as Normal"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_Array_N")),
		FString(TEXT("Normal")));
	TestEqual(
		TEXT("Project T_Normal parameter is classified as Normal"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_Normal")),
		FString(TEXT("Normal")));
	TestEqual(
		TEXT("PackedORM is classified as ORM"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("PackedORM")),
		FString(TEXT("ORM")));
	TestEqual(
		TEXT("M_Env_Building unique MRA map is classified as ORM"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T Unique MRA Map")),
		FString(TEXT("ORM")));
	TestEqual(
		TEXT("M_Env_Building array material map is classified as ORM"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_Array_M")),
		FString(TEXT("ORM")));
	TestEqual(
		TEXT("Project T_MRA parameter is classified as ORM"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_MRA")),
		FString(TEXT("ORM")));
	TestEqual(
		TEXT("EmissionMap is classified as Emissive"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("EmissionMap")),
		FString(TEXT("Emissive")));
	TestEqual(
		TEXT("Project T_Mask parameter is classified as Mask"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("T_Mask")),
		FString(TEXT("Mask")));
	TestEqual(
		TEXT("Material light info texture is classified separately"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("Tex_LightInfo")),
		FString(TEXT("LightInfo")));
	TestEqual(
		TEXT("Unknown texture parameters stay visible as Unknown"),
		FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(TEXT("DetailNoise")),
		FString(TEXT("Unknown")));

	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison_S_01");
	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry CandidateEntry;
	CandidateEntry.SourceKind = TEXT("MapComponent");
	CandidateEntry.SourcePath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActorName = TEXT("Wall_A");
	CandidateEntry.ComponentName = TEXT("StaticMeshComponent0");
	CandidateEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	CandidateEntry.MaterialSlotCount = 1;
	CandidateEntry.MaterialSlotNames = { TEXT("WallBase") };
	CandidateEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase") };
	CandidateEntry.EnvBatchTags = { TEXT("EnvBatch.Source.LOD0") };
	CandidateEntry.LodCount = 2;
	CandidateEntry.bCandidate = true;
	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { CandidateEntry });

	FMaterialBatchBuildTextureChannelPlan Channel;
	Channel.ChannelName = TEXT("BaseColor");
	Channel.ParameterName = TEXT("BaseMap");
	Channel.TexturePath = TEXT("/Game/Art/Textures/T_Wall_D.T_Wall_D");
	Channel.TextureClass = TEXT("Texture2D");
	Channel.bFoundTexture = true;
	Channel.TextureWidth = 1024;
	Channel.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(Channel);

	FMaterialBatchBuildTextureChannelPlan NormalChannel;
	NormalChannel.ChannelName = TEXT("Normal");
	NormalChannel.ParameterName = TEXT("NormalMap");
	NormalChannel.TexturePath = TEXT("/Game/Art/Textures/T_Wall_N.T_Wall_N");
	NormalChannel.TextureClass = TEXT("Texture2D");
	NormalChannel.bFoundTexture = true;
	NormalChannel.TextureWidth = 1024;
	NormalChannel.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(NormalChannel);

	FMaterialBatchBuildTextureChannelPlan ExistingArrayChannel;
	ExistingArrayChannel.ChannelName = TEXT("BaseColor");
	ExistingArrayChannel.ParameterName = TEXT("T_Array_A");
	ExistingArrayChannel.TexturePath = TEXT("/Game/Art/Textures/T_Array_A.T_Array_A");
	ExistingArrayChannel.TextureClass = TEXT("Texture2DArray");
	ExistingArrayChannel.bFoundTexture = true;
	ExistingArrayChannel.TextureWidth = 1024;
	ExistingArrayChannel.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(ExistingArrayChannel);

	FMaterialBatchBuildTextureChannelPlan LightInfoChannel;
	LightInfoChannel.ChannelName = TEXT("LightInfo");
	LightInfoChannel.ParameterName = TEXT("Tex_LightInfo");
	LightInfoChannel.TexturePath = TEXT("/Game/Art/Textures/T_LightInfo.T_LightInfo");
	LightInfoChannel.TextureClass = TEXT("CanvasRenderTarget2D");
	LightInfoChannel.bFoundTexture = true;
	LightInfoChannel.TextureWidth = 256;
	LightInfoChannel.TextureHeight = 4;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(LightInfoChannel);

	const FString Manifest = FMaterialBatchBuildPlanBuilder::BuildJsonManifest(Plan);
	TestTrue(TEXT("Manifest writes texture channel arrays"), Manifest.Contains(TEXT("\"textureChannels\":")));
	TestTrue(TEXT("Manifest writes channel name"), Manifest.Contains(TEXT("\"channel\":\"BaseColor\"")));
	TestTrue(TEXT("Manifest writes parameter name"), Manifest.Contains(TEXT("\"parameter\":\"BaseMap\"")));
	TestTrue(TEXT("Manifest writes texture path"), Manifest.Contains(TEXT("\"texture\":\"/Game/Art/Textures/T_Wall_D.T_Wall_D\"")));
	TestTrue(TEXT("Manifest writes texture class"), Manifest.Contains(TEXT("\"textureClass\":\"Texture2D\"")));
	TestTrue(TEXT("Manifest writes texture found state"), Manifest.Contains(TEXT("\"found\":true")));
	TestTrue(TEXT("Manifest writes texture width"), Manifest.Contains(TEXT("\"width\":1024")));
	TestTrue(TEXT("Manifest writes texture height"), Manifest.Contains(TEXT("\"height\":1024")));
	TestTrue(TEXT("Manifest marks ordinary Texture2D eligible for array build"), Manifest.Contains(TEXT("\"arrayBuildEligible\":true")));
	TestTrue(TEXT("Manifest records eligible reason"), Manifest.Contains(TEXT("\"arrayBuildReason\":\"Texture2D\"")));
	TestTrue(TEXT("Manifest marks existing Texture2DArray ineligible for array build"), Manifest.Contains(TEXT("\"arrayBuildReason\":\"ExistingTexture2DArrayInput\"")));
	TestTrue(TEXT("Manifest keeps material light texture out of ordinary array build"), Manifest.Contains(TEXT("\"arrayBuildReason\":\"UnsupportedTextureClass:CanvasRenderTarget2D\"")));
	TestTrue(TEXT("Manifest writes texture array slice plans"), Manifest.Contains(TEXT("\"textureArrays\":")));
	TestTrue(TEXT("Manifest writes VT atlas plan"), Manifest.Contains(TEXT("\"vtAtlas\":")));
	TestTrue(TEXT("VT atlas plan records deterministic layout"), Manifest.Contains(TEXT("\"layoutPolicy\":\"DeterministicGridByUniqueTexture\"")));
	TestTrue(TEXT("VT atlas plan records output width"), Manifest.Contains(TEXT("\"width\":2048")));
	TestTrue(TEXT("VT atlas plan records output height"), Manifest.Contains(TEXT("\"height\":1024")));
	TestTrue(TEXT("VT atlas plan records grid columns"), Manifest.Contains(TEXT("\"columns\":2")));
	TestTrue(TEXT("VT atlas plan records grid rows"), Manifest.Contains(TEXT("\"rows\":1")));
	TestTrue(TEXT("VT atlas plan records UDIM-style virtual texture layout"), Manifest.Contains(TEXT("\"virtualTextureLayout\":\"UDIMStyleGrid\"")));
	TestTrue(TEXT("VT atlas plan records root tile padding"), Manifest.Contains(TEXT("\"tilePaddingPixels\":8")));
	TestTrue(TEXT("VT atlas plan records atlas entries"), Manifest.Contains(TEXT("\"entries\":[{\"atlasEntryIndex\":0,\"channel\":\"BaseColor\"")));
	TestTrue(TEXT("VT atlas plan records UDIM tile number"), Manifest.Contains(TEXT("\"udimNumber\":1001")));
	TestTrue(TEXT("VT atlas plan records tile U coordinate"), Manifest.Contains(TEXT("\"tileU\":0")));
	TestTrue(TEXT("VT atlas plan records tile V coordinate"), Manifest.Contains(TEXT("\"tileV\":0")));
	TestTrue(TEXT("VT atlas plan records UV remap status"), Manifest.Contains(TEXT("\"uvRemapStatus\":\"PlannedForMergedProxyUVRemap\"")));
	TestTrue(TEXT("VT atlas plan records UV rect minimum"), Manifest.Contains(TEXT("\"uvRectMin\":{\"x\":0")));
	TestTrue(TEXT("VT atlas plan records duplicate residency risk"), Manifest.Contains(TEXT("\"duplicateResidencyRisk\":true")));
	TestTrue(TEXT("Manifest records residency risk plan"), Manifest.Contains(TEXT("\"residencyRiskPlan\":")));
	TestTrue(TEXT("Manifest records residency gate"), Manifest.Contains(TEXT("\"residencyGate\":\"BlockedUntilSourceProxyUnloaded\"")));
	TestTrue(TEXT("Manifest blocks Texture2DArray fallback in VT production path"), Manifest.Contains(TEXT("\"allowTextureArrayFallbackInProduction\":false")));
	TestTrue(TEXT("Manifest requires source/proxy unload for mixed residency"), Manifest.Contains(TEXT("\"requiresSourceProxyUnload\":true")));
	TestTrue(TEXT("Texture array plan records base color channel"), Manifest.Contains(TEXT("\"baseColor\":")));
	TestTrue(TEXT("Texture array plan records normal channel"), Manifest.Contains(TEXT("\"normal\":")));
	TestTrue(TEXT("Texture array plan records slice index zero"), Manifest.Contains(TEXT("\"sliceIndex\":0")));
	TestTrue(TEXT("Texture array plan records base color source texture"), Manifest.Contains(TEXT("\"texture\":\"/Game/Art/Textures/T_Wall_D.T_Wall_D\"")));
	TestTrue(TEXT("Texture array plan records normal source texture"), Manifest.Contains(TEXT("\"texture\":\"/Game/Art/Textures/T_Wall_N.T_Wall_N\"")));
	TestFalse(TEXT("Existing Texture2DArray input is not repacked as a generated slice"), Manifest.Contains(TEXT("\"texture\":\"/Game/Art/Textures/T_Array_A.T_Array_A\",\"textureClass\":\"Texture2DArray\",\"sliceIndex\"")));
	TestTrue(TEXT("Manifest writes property rows for batch material lookup"), Manifest.Contains(TEXT("\"propertyRows\":")));
	TestTrue(TEXT("Manifest writes property texture layout"), Manifest.Contains(TEXT("\"propertyTextureLayout\":")));
	TestTrue(TEXT("Property texture layout records row key"), Manifest.Contains(TEXT("\"rowKey\":\"batchMaterialIndex\"")));
	TestTrue(TEXT("Property texture layout records base color column"), Manifest.Contains(TEXT("\"name\":\"BaseColorSlice\"")));
	TestTrue(TEXT("Property texture layout records normal column source field"), Manifest.Contains(TEXT("\"sourceField\":\"normalSlice\"")));
	TestTrue(TEXT("Property texture layout records integer texture format contract"), Manifest.Contains(TEXT("\"valueType\":\"int32\"")));
	TestTrue(TEXT("Property texture layout records BaseColor VT UV float columns"), Manifest.Contains(TEXT("\"sourceField\":\"baseVTUVMaxX\"")));
	TestTrue(TEXT("Property texture layout records Normal VT UV float columns"), Manifest.Contains(TEXT("\"sourceField\":\"normalVTUVMaxX\"")));
	TestTrue(TEXT("Property texture layout records ORM VT UV float columns"), Manifest.Contains(TEXT("\"sourceField\":\"ormVTUVMaxX\"")));
	TestTrue(TEXT("Property texture layout records float texture format contract"), Manifest.Contains(TEXT("\"valueType\":\"float\"")));
	TestTrue(TEXT("Property row records source batch material index"), Manifest.Contains(TEXT("\"batchMaterialIndex\":0")));
	TestTrue(TEXT("Property row records base color slice"), Manifest.Contains(TEXT("\"baseColorSlice\":0")));
	TestTrue(TEXT("Property row records source texture for VT atlas lookup"), Manifest.Contains(TEXT("\"sourceTexture\":\"/Game/Art/Textures/T_Wall_D.T_Wall_D\"")));
	TestTrue(TEXT("Property row records VT atlas UV rect"), Manifest.Contains(TEXT("\"uvRectMax\":{\"x\":")));
	TestTrue(TEXT("Property row records normal source texture for VT atlas lookup"), Manifest.Contains(TEXT("\"normalSourceTexture\":\"/Game/Art/Textures/T_Wall_N.T_Wall_N\"")));
	TestTrue(TEXT("Property row records normal VT atlas UV rect"), Manifest.Contains(TEXT("\"normalUVRectMax\":{\"x\":")));
	TestTrue(TEXT("Property row records normal slice"), Manifest.Contains(TEXT("\"normalSlice\":0")));
	TestTrue(TEXT("Property row records absent ORM slice"), Manifest.Contains(TEXT("\"ormSlice\":-1")));

	const TArray<FString> ReportLines = FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(Plan);
	const FString Report = FString::Join(ReportLines, TEXT("\n"));
	TestTrue(TEXT("Report includes texture array build eligibility section"), ReportLines.Contains(TEXT("## Texture2DArray Build Eligibility")));
	TestTrue(
		TEXT("Report records eligible base color texture"),
		ReportLines.Contains(TEXT("| 0 | BaseColor | `BaseMap` | `/Game/Art/Textures/T_Wall_D.T_Wall_D` | Texture2D | 1024 | 1024 | Yes | Texture2D |")));
	TestTrue(
		TEXT("Report records existing array as ineligible"),
		ReportLines.Contains(TEXT("| 0 | BaseColor | `T_Array_A` | `/Game/Art/Textures/T_Array_A.T_Array_A` | Texture2DArray | 1024 | 1024 | No | ExistingTexture2DArrayInput |")));
	TestTrue(TEXT("Report includes texture array slice section"), ReportLines.Contains(TEXT("## Planned Texture2DArray Slices")));
	TestTrue(TEXT("Report includes VT atlas section"), ReportLines.Contains(TEXT("## Planned VT Atlas Entries")));
	TestTrue(TEXT("Report includes VT atlas planned size"), ReportLines.Contains(TEXT("- Planned atlas size: 2048x1024")));
	TestTrue(TEXT("Report includes residency risk section"), ReportLines.Contains(TEXT("## VT/Non-VT Residency Risk Plan")));
	TestTrue(TEXT("Report records residency gate"), ReportLines.Contains(TEXT("- Residency gate: `BlockedUntilSourceProxyUnloaded`")));
	TestTrue(TEXT("Report records production fallback policy"), ReportLines.Contains(TEXT("- Texture2DArray fallback allowed in production: No")));
	TestTrue(
		TEXT("Report includes VT atlas base color entry"),
		Report.Contains(TEXT("| 0 | BaseColor | `/Game/Art/Textures/T_Wall_D.T_Wall_D` | Texture2D | 1024 | 1024 | 1001 | 0,0 | 8 | PlannedForMergedProxyUVRemap | 0.0000,0.0000 | 0.5000,1.0000 |")));
	TestTrue(
		TEXT("Report includes base color array slice allocation"),
		ReportLines.Contains(TEXT("| BaseColor | 0 | `/Game/Art/Textures/T_Wall_D.T_Wall_D` | Texture2D |")));
	TestTrue(
		TEXT("Report includes normal array slice allocation"),
		ReportLines.Contains(TEXT("| Normal | 0 | `/Game/Art/Textures/T_Wall_N.T_Wall_N` | Texture2D |")));
	TestTrue(TEXT("Report includes property texture row section"), ReportLines.Contains(TEXT("## Planned Property Texture Rows")));
	TestTrue(
		TEXT("Report includes property row slice allocation"),
		ReportLines.Contains(TEXT("| 0 | 0 | 0 | -1 | -1 | -1 | 0.0000,0.0000 | 0.5000,1.0000 | 0.5000,0.0000 | 1.0000,1.0000 | 0.0000,0.0000 | 1.0000,1.0000 | `/Game/Art/Textures/T_Wall_D.T_Wall_D` | `/Game/Art/Textures/T_Wall_N.T_Wall_N` | `` | `/Game/Art/Textures/T_LightInfo.T_LightInfo` | `/Game/Art/Materials/MI_WallBase.MI_WallBase` |")));
	TestTrue(TEXT("Report includes property texture layout section"), ReportLines.Contains(TEXT("## Property Texture Layout")));
	TestTrue(
		TEXT("Report includes base color layout column"),
		ReportLines.Contains(TEXT("| 0 | BaseColorSlice | baseColorSlice | int32 | -1 | Texture2DArray slice index for BaseColor |")));
	TestTrue(
		TEXT("Report includes VT UV layout column"),
		ReportLines.Contains(TEXT("| 7 | BaseVTUVMaxX | baseVTUVMaxX | float | 1 | VT atlas UV rect maximum X for BaseColor |")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanTextureArrayPayloadsTest,
	"DevKitEditor.MaterialBatch.BuildPlan.BuildsTextureArrayPayloads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanTextureArrayPayloadsTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry FirstEntry;
	FirstEntry.SourceKind = TEXT("MapComponent");
	FirstEntry.ActorName = TEXT("Wall_A");
	FirstEntry.ComponentName = TEXT("StaticMeshComponent0");
	FirstEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	FirstEntry.MaterialSlotCount = 1;
	FirstEntry.MaterialSlotNames = { TEXT("WallBase") };
	FirstEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase") };
	FirstEntry.LodCount = 2;
	FirstEntry.bCandidate = true;

	FMaterialBatchBuildPlannedEntry SecondEntry = FirstEntry;
	SecondEntry.ActorName = TEXT("Wall_B");
	SecondEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_B.SM_Wall_B");
	SecondEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_WallTrim.MI_WallTrim") };

	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { FirstEntry, SecondEntry });

	FMaterialBatchBuildTextureChannelPlan SharedBaseColor;
	SharedBaseColor.ChannelName = TEXT("BaseColor");
	SharedBaseColor.ParameterName = TEXT("BaseMap");
	SharedBaseColor.TexturePath = TEXT("/Game/Art/Textures/T_Shared_D.T_Shared_D");
	SharedBaseColor.TextureClass = TEXT("Texture2D");
	SharedBaseColor.bFoundTexture = true;
	SharedBaseColor.TextureWidth = 1024;
	SharedBaseColor.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(SharedBaseColor);
	Plan.PlannedMaterialRows[1].TextureChannels.Add(SharedBaseColor);

	FMaterialBatchBuildTextureChannelPlan Normal;
	Normal.ChannelName = TEXT("Normal");
	Normal.ParameterName = TEXT("NormalMap");
	Normal.TexturePath = TEXT("/Game/Art/Textures/T_Wall_N.T_Wall_N");
	Normal.TextureClass = TEXT("Texture2D");
	Normal.bFoundTexture = true;
	Normal.TextureWidth = 1024;
	Normal.TextureHeight = 1024;
	Plan.PlannedMaterialRows[1].TextureChannels.Add(Normal);

	FMaterialBatchBuildTextureChannelPlan Mask;
	Mask.ChannelName = TEXT("Mask");
	Mask.ParameterName = TEXT("OpacityMask");
	Mask.TexturePath = TEXT("/Game/Art/Textures/T_Wall_Mask.T_Wall_Mask");
	Mask.TextureClass = TEXT("Texture2D");
	Mask.bFoundTexture = true;
	Mask.TextureWidth = 1024;
	Mask.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(Mask);

	const TArray<FMaterialBatchBuildTextureArrayPayload> Payloads =
		FMaterialBatchBuildPlanBuilder::BuildTextureArrayPayloads(Plan);
	const FMaterialBatchBuildVTAtlasPayload VTAtlasPayload =
		FMaterialBatchBuildPlanBuilder::BuildVTAtlasPayload(Plan);

	TestEqual(TEXT("Texture array payload count"), Payloads.Num(), 3);
	TestEqual(TEXT("BaseColor payload channel"), Payloads[0].ChannelName, FString(TEXT("BaseColor")));
	TestEqual(TEXT("BaseColor payload package"), Payloads[0].PackagePath, Plan.BaseColorArrayPackage);
	TestEqual(TEXT("BaseColor shared texture only creates one slice"), Payloads[0].SourceTexturePaths.Num(), 1);
	TestEqual(TEXT("BaseColor payload records source width"), Payloads[0].Width, 1024);
	TestEqual(TEXT("BaseColor payload records source height"), Payloads[0].Height, 1024);
	TestEqual(TEXT("Normal payload channel"), Payloads[1].ChannelName, FString(TEXT("Normal")));
	TestEqual(TEXT("Normal payload package"), Payloads[1].PackagePath, Plan.NormalArrayPackage);
	TestEqual(TEXT("Mask payload channel"), Payloads[2].ChannelName, FString(TEXT("Mask")));
	TestEqual(TEXT("Mask payload package"), Payloads[2].PackagePath, Plan.MaskArrayPackage);
	TestEqual(TEXT("Mask payload source"), Payloads[2].SourceTexturePaths[0], FString(TEXT("/Game/Art/Textures/T_Wall_Mask.T_Wall_Mask")));
	TestEqual(TEXT("VT atlas payload package"), VTAtlasPayload.PackagePath, Plan.VTAtlasPackage);
	TestEqual(TEXT("VT atlas payload entry count"), VTAtlasPayload.Entries.Num(), 3);
	TestEqual(TEXT("VT atlas payload grid columns"), VTAtlasPayload.Columns, 2);
	TestEqual(TEXT("VT atlas payload grid rows"), VTAtlasPayload.Rows, 2);
	TestEqual(TEXT("VT atlas payload records UDIM-style layout"), VTAtlasPayload.VirtualTextureLayout, FString(TEXT("UDIMStyleGrid")));
	TestEqual(TEXT("VT atlas payload records tile padding"), VTAtlasPayload.TilePaddingPixels, 8);
	TestEqual(TEXT("VT atlas payload first UDIM number"), VTAtlasPayload.Entries[0].UdimNumber, 1001);
	TestEqual(TEXT("VT atlas payload second tile U"), VTAtlasPayload.Entries[1].TileU, 1);
	TestEqual(TEXT("VT atlas payload third tile V"), VTAtlasPayload.Entries[2].TileV, 1);
	TestEqual(TEXT("VT atlas payload width"), VTAtlasPayload.Width, 2048);
	TestEqual(TEXT("VT atlas payload height"), VTAtlasPayload.Height, 2048);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanProxyMeshPayloadTest,
	"DevKitEditor.MaterialBatch.BuildPlan.BuildsProxyMeshPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanProxyMeshPayloadTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry CandidateEntry;
	CandidateEntry.SourceKind = TEXT("MapComponent");
	CandidateEntry.SourcePath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActorName = TEXT("Wall_A");
	CandidateEntry.ComponentName = TEXT("StaticMeshComponent0");
	CandidateEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	CandidateEntry.bHasWorldTransform = true;
	CandidateEntry.WorldLocation = FVector(100.0, 200.0, 300.0);
	CandidateEntry.WorldRotation = FRotator(0.0, 90.0, 0.0);
	CandidateEntry.WorldScale = FVector(1.0, 2.0, 1.0);
	CandidateEntry.MaterialSlotCount = 2;
	CandidateEntry.MaterialSlotNames = { TEXT("WallBase"), TEXT("WallTrim") };
	CandidateEntry.MaterialPaths = {
		TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase"),
		TEXT("/Game/Art/Materials/MI_WallTrim.MI_WallTrim")
	};
	CandidateEntry.LodCount = 2;
	CandidateEntry.bCandidate = true;

	FMaterialBatchBuildPlannedEntry RejectedEntry = CandidateEntry;
	RejectedEntry.ActorName = TEXT("Door_A");
	RejectedEntry.AssetPath = TEXT("/Game/Art/Env/SM_Door_A.SM_Door_A");
	RejectedEntry.bCandidate = false;
	RejectedEntry.RejectReason = TEXT("InteractiveTag");

	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { CandidateEntry, RejectedEntry });

	const FMaterialBatchBuildProxyMeshPayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildProxyMeshPayload(Plan);

	TestEqual(TEXT("Proxy payload records target package"), Payload.PackagePath, Plan.ProxyMeshPackage);
	TestEqual(TEXT("Proxy payload uses UV7.x as material index channel"), Payload.MaterialIndexChannel, FString(TEXT("TexCoord7.x")));
	TestEqual(TEXT("Proxy payload only includes candidate sources"), Payload.Sources.Num(), 1);
	TestEqual(TEXT("Proxy source records planned entry index"), Payload.Sources[0].SourceEntryIndex, 0);
	TestEqual(TEXT("Proxy source records static mesh path"), Payload.Sources[0].StaticMeshPath, FString(TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A")));
	TestTrue(TEXT("Proxy source keeps world transform flag"), Payload.Sources[0].bHasWorldTransform);
	TestEqual(TEXT("Proxy source records location"), Payload.Sources[0].WorldLocation, FVector(100.0, 200.0, 300.0));
	TestEqual(TEXT("Proxy source records rotation"), Payload.Sources[0].WorldRotation, FRotator(0.0, 90.0, 0.0));
	TestEqual(TEXT("Proxy source records scale"), Payload.Sources[0].WorldScale, FVector(1.0, 2.0, 1.0));
	TestEqual(TEXT("Proxy source remaps one material slot per batch index"), Payload.Sources[0].MaterialSlotToBatchMaterialIndex.Num(), 2);
	TestEqual(TEXT("First material slot maps to first batch index"), Payload.Sources[0].MaterialSlotToBatchMaterialIndex[0], 0);
	TestEqual(TEXT("Second material slot maps to second batch index"), Payload.Sources[0].MaterialSlotToBatchMaterialIndex[1], 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanBatchMaterialPayloadTest,
	"DevKitEditor.MaterialBatch.BuildPlan.BuildsBatchMaterialPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanBatchMaterialPayloadTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");

	const FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);
	const FMaterialBatchBuildBatchMaterialPayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildBatchMaterialPayload(Plan);

	TestEqual(TEXT("Batch material payload records target package"), Payload.PackagePath, Plan.BatchMaterialInstancePackage);
	TestEqual(
		TEXT("Batch material payload uses the final batch parent material"),
		Payload.ParentMaterialPath,
		FString(TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas.M_Env_Baked_VTAtlas")));
	TestEqual(TEXT("Batch material payload binds VT atlas plus property texture"), Payload.TextureBindings.Num(), 2);
	TestEqual(TEXT("VT atlas parameter"), Payload.TextureBindings[0].ParameterName, FString(TEXT("VT_Atlas")));
	TestEqual(TEXT("VT atlas package"), Payload.TextureBindings[0].TexturePackagePath, Plan.VTAtlasPackage);
	TestEqual(TEXT("Property texture parameter"), Payload.TextureBindings[1].ParameterName, FString(TEXT("_PropTexture")));
	TestEqual(TEXT("Property texture package"), Payload.TextureBindings[1].TexturePackagePath, Plan.PropertyTexturePackage);
	TestEqual(TEXT("Batch material payload binds row-count scalar parameters"), Payload.ScalarBindings.Num(), 2);
	TestEqual(TEXT("Batch row-count scalar parameter"), Payload.ScalarBindings[0].ParameterName, FString(TEXT("BatchRowCount")));
	TestEqual(TEXT("Batch row-count scalar default for empty dry-run plan is clamped"), Payload.ScalarBindings[0].Value, 1.0f);
	TestEqual(TEXT("Property column-count scalar parameter"), Payload.ScalarBindings[1].ParameterName, FString(TEXT("PropertyColumnCount")));
	TestEqual(TEXT("Property column-count scalar value"), Payload.ScalarBindings[1].Value, 17.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanMappingDataAssetTest,
	"DevKitEditor.MaterialBatch.BuildPlan.PopulatesMappingDataAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanMappingDataAssetTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.RootPath = TEXT("/Game/Art");
	Options.MapPath = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");
	Options.TextureBackend = TEXT("VTAtlas");
	Options.SurfaceKind = TEXT("Wall");
	Options.BakePolicy = TEXT("StaticBake");
	Options.SourceProxyExclusivityGroup = TEXT("Prison S 01 SourceProxy");
	Options.OutputRoot = TEXT("/Game/Generated/MaterialBatch");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry CandidateEntry;
	CandidateEntry.SourceKind = TEXT("MapComponent");
	CandidateEntry.SourcePath = Options.MapPath;
	CandidateEntry.ActorName = TEXT("Wall_A");
	CandidateEntry.ComponentName = TEXT("StaticMeshComponent0");
	CandidateEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	CandidateEntry.bHasWorldTransform = true;
	CandidateEntry.WorldLocation = FVector(100.0, 200.0, 300.0);
	CandidateEntry.WorldRotation = FRotator(0.0, 90.0, 0.0);
	CandidateEntry.WorldScale = FVector(1.0, 2.0, 1.0);
	CandidateEntry.MaterialSlotCount = 1;
	CandidateEntry.MaterialSlotNames = { TEXT("WallBase") };
	CandidateEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase") };
	CandidateEntry.EnvBatchTags = { TEXT("EnvBatch.Source.LOD0") };
	CandidateEntry.ActualStreamingLevelName = TEXT("L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActualLevelPackageName = TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01");
	CandidateEntry.ActualLayerNames = {
		TEXT("L1_CommonLevel_Prison_S_01"),
		TEXT("/Game/Art/Map/GameLevel_L1/Prison/L1_CommonLevel_Prison_S_01")
	};
	CandidateEntry.LodCount = 2;
	CandidateEntry.bCandidate = true;
	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { CandidateEntry });

	FMaterialBatchBuildTextureChannelPlan BaseColorChannel;
	BaseColorChannel.ChannelName = TEXT("BaseColor");
	BaseColorChannel.ParameterName = TEXT("BaseMap");
	BaseColorChannel.TexturePath = TEXT("/Game/Art/Textures/T_Wall_D.T_Wall_D");
	BaseColorChannel.TextureClass = TEXT("Texture2D");
	BaseColorChannel.bFoundTexture = true;
	BaseColorChannel.TextureWidth = 1024;
	BaseColorChannel.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(BaseColorChannel);

	UMaterialBatchMappingDataAsset* MappingData = NewObject<UMaterialBatchMappingDataAsset>();
	TestNotNull(TEXT("Mapping data asset is constructible"), MappingData);

	FMaterialBatchBuildPlanBuilder::PopulateMappingDataAsset(Plan, *MappingData);

	TestEqual(TEXT("Mapping asset records schema"), MappingData->Schema, FString(TEXT("DevKit.MaterialBatchMappingData.v1")));
	TestEqual(TEXT("Mapping asset records sanitized cluster"), MappingData->ClusterName, FString(TEXT("Prison_S_01")));
	TestEqual(TEXT("Mapping asset records proxy package"), MappingData->ProxyMeshPackage, Plan.ProxyMeshPackage);
	TestEqual(TEXT("Mapping asset stores batch parent material"), MappingData->BatchParentMaterialPackage, Plan.BatchParentMaterialPackage);
	TestEqual(TEXT("Mapping asset stores texture backend"), MappingData->TextureBackend, FString(TEXT("VTAtlas")));
	TestEqual(TEXT("Mapping asset stores VT atlas package"), MappingData->VTAtlasPackage, Plan.VTAtlasPackage);
	TestEqual(TEXT("Mapping asset stores surface kind"), MappingData->SurfaceKind, FString(TEXT("Wall")));
	TestEqual(TEXT("Mapping asset stores bake policy"), MappingData->BakePolicy, FString(TEXT("StaticBake")));
	TestEqual(TEXT("Mapping asset stores source/proxy exclusivity group"), MappingData->SourceProxyExclusivityGroup, FString(TEXT("Prison_S_01_SourceProxy")));
	TestEqual(TEXT("Mapping asset stores layer backend"), MappingData->SourceProxyLayerPlan.LayerBackend, FString(TEXT("StreamingLevel")));
	TestEqual(TEXT("Mapping asset stores source layer name"), MappingData->SourceProxyLayerPlan.SourceLayerName, FString(TEXT("L1_CommonLevel_Prison_S_01")));
	TestEqual(TEXT("Mapping asset stores proxy layer name"), MappingData->SourceProxyLayerPlan.ProxyLayerName, FString(TEXT("SL_Prison_S_01_SourceProxy_Proxy")));
	TestEqual(TEXT("Mapping asset stores baked layer name"), MappingData->SourceProxyLayerPlan.BakedLayerName, FString(TEXT("SL_Prison_S_01_SourceProxy_Baked")));
	TestEqual(TEXT("Mapping asset stores four tier layer selections"), MappingData->SourceProxyLayerPlan.TierSelections.Num(), 4);
	TestTrue(TEXT("Mapping asset stores Mid proxy layer selection"), MappingData->SourceProxyLayerPlan.TierSelections[2].bLoadProxyLayer);
	TestTrue(TEXT("Mapping asset stores Low baked layer selection"), MappingData->SourceProxyLayerPlan.TierSelections[3].bLoadBakedLayer);
	TestEqual(TEXT("Mapping asset stores residency gate"), MappingData->ResidencyRiskPlan.ResidencyGate, FString(TEXT("BlockedUntilSourceProxyUnloaded")));
	TestTrue(TEXT("Mapping asset stores VT atlas main path"), MappingData->ResidencyRiskPlan.bVTAtlasMainPath);
	TestFalse(TEXT("Mapping asset blocks Texture2DArray fallback in VT production path"), MappingData->ResidencyRiskPlan.bAllowTextureArrayFallbackInProduction);
	TestTrue(TEXT("Mapping asset stores source/proxy unload requirement"), MappingData->ResidencyRiskPlan.bRequiresSourceProxyUnload);
	TestEqual(TEXT("Mapping asset stores layer readiness entry count"), MappingData->SourceProxyLayerReadiness.EntryCount, 1);
	TestEqual(TEXT("Mapping asset stores layer readiness ready count"), MappingData->SourceProxyLayerReadiness.ReadyEntryCount, 1);
	TestEqual(TEXT("Mapping asset stores layer readiness assignments"), MappingData->SourceProxyLayerReadiness.Assignments.Num(), 1);
	TestEqual(TEXT("Mapping asset stores actual layer match count"), MappingData->SourceProxyLayerReadiness.ActualLayerMatchCount, 1);
	TestEqual(TEXT("Mapping asset stores missing actual layer count"), MappingData->SourceProxyLayerReadiness.MissingActualLayerCount, 0);
	TestEqual(TEXT("Mapping asset stores source layer readiness role"), MappingData->SourceProxyLayerReadiness.Assignments[0].LayerRole, FString(TEXT("Source")));
	TestEqual(TEXT("Mapping asset stores expected source layer"), MappingData->SourceProxyLayerReadiness.Assignments[0].ExpectedLayerName, FString(TEXT("L1_CommonLevel_Prison_S_01")));
	TestEqual(TEXT("Mapping asset stores actual source layer"), MappingData->SourceProxyLayerReadiness.Assignments[0].ActualLayerNames[0], FString(TEXT("L1_CommonLevel_Prison_S_01")));
	TestEqual(TEXT("Mapping asset stores actual streaming level"), MappingData->SourceProxyLayerReadiness.Assignments[0].ActualStreamingLevelName, FString(TEXT("L1_CommonLevel_Prison_S_01")));
	TestTrue(TEXT("Mapping asset stores expected layer match"), MappingData->SourceProxyLayerReadiness.Assignments[0].bMatchesExpectedLayer);
	TestEqual(TEXT("Mapping asset stores layer validation status"), MappingData->SourceProxyLayerReadiness.Assignments[0].LayerValidationStatus, FString(TEXT("MatchedExpectedLayer")));
	TestEqual(TEXT("Mapping asset stores asset readiness entry count"), MappingData->SourceProxyAssetReadiness.EntryCount, 1);
	TestEqual(TEXT("Mapping asset stores ready source/proxy pairs"), MappingData->SourceProxyAssetReadiness.ReadyPairCount, 1);
	TestEqual(TEXT("Mapping asset stores generated proxy fallback count"), MappingData->SourceProxyAssetReadiness.GeneratedProxyFallbackCount, 1);
	TestEqual(TEXT("Mapping asset stores source/proxy asset assignments"), MappingData->SourceProxyAssetReadiness.Assignments.Num(), 1);
	TestEqual(TEXT("Mapping asset stores source asset path"), MappingData->SourceProxyAssetReadiness.Assignments[0].SourceAssetPath, FString(TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A")));
	TestEqual(TEXT("Mapping asset stores proxy asset path"), MappingData->SourceProxyAssetReadiness.Assignments[0].ProxyAssetPath, Plan.ProxyMeshPackage);
	TestEqual(TEXT("Mapping asset stores default source LOD0"), MappingData->SourceProxyAssetReadiness.Assignments[0].SourceLODIndex, 0);
	TestEqual(TEXT("Mapping asset stores default proxy LOD1"), MappingData->SourceProxyAssetReadiness.Assignments[0].ProxyLODIndex, 1);
	TestTrue(TEXT("Mapping asset marks generated proxy pairing ready"), MappingData->SourceProxyAssetReadiness.Assignments[0].bReadyForAssetPairing);
	TestEqual(TEXT("Mapping asset stores asset readiness status"), MappingData->SourceProxyAssetReadiness.Assignments[0].ReadinessStatus, FString(TEXT("ReadyGeneratedProxy")));
	TestEqual(TEXT("Mapping asset stores source/proxy config count"), MappingData->SourceProxyAssetConfigSet.ConfigCount, 1);
	TestEqual(TEXT("Mapping asset stores ready source/proxy config count"), MappingData->SourceProxyAssetConfigSet.ReadyConfigCount, 1);
	TestEqual(TEXT("Mapping asset stores generated fallback config count"), MappingData->SourceProxyAssetConfigSet.GeneratedFallbackConfigCount, 1);
	TestEqual(TEXT("Mapping asset stores source/proxy configs"), MappingData->SourceProxyAssetConfigSet.Configs.Num(), 1);
	TestEqual(TEXT("Mapping asset stores source/proxy config key"), MappingData->SourceProxyAssetConfigSet.Configs[0].ObjectKey, FString(TEXT("Wall_A::StaticMeshComponent0")));
	TestEqual(TEXT("Mapping asset stores source/proxy config source"), MappingData->SourceProxyAssetConfigSet.Configs[0].ConfigSource, FString(TEXT("GeneratedFallback")));
	TestEqual(TEXT("Mapping asset stores source/proxy config interaction policy"), MappingData->SourceProxyAssetConfigSet.Configs[0].InteractionPolicy, FString(TEXT("StaticBatchCandidate")));
	TestEqual(TEXT("Mapping asset stores source/proxy config generated asset"), MappingData->SourceProxyAssetConfigSet.Configs[0].GeneratedProxyAssetPath, Plan.ProxyMeshPackage);
	TestTrue(TEXT("Mapping asset marks generated proxy config fallback"), MappingData->SourceProxyAssetConfigSet.Configs[0].bUsesGeneratedProxyFallback);
	TestTrue(TEXT("Mapping asset records duplicate residency risk when VT atlas and array fallback both have source textures"), MappingData->bDuplicateResidencyRisk);
	TestEqual(TEXT("Mapping asset records material rows"), MappingData->MaterialRows.Num(), 1);
	TestEqual(TEXT("Mapping asset stores batch material index"), MappingData->MaterialRows[0].BatchMaterialIndex, 0);
	TestEqual(TEXT("Mapping asset stores source material path"), MappingData->MaterialRows[0].MaterialPath, FString(TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase")));
	TestEqual(TEXT("Mapping asset stores property rows"), MappingData->PropertyRows.Num(), 1);
	TestEqual(TEXT("Mapping asset stores base color slice"), MappingData->PropertyRows[0].BaseColorSlice, 0);
	TestEqual(TEXT("Mapping asset stores property row surface kind"), MappingData->PropertyRows[0].SurfaceKind, FString(TEXT("Wall")));
	TestEqual(TEXT("Mapping asset stores property row bake policy"), MappingData->PropertyRows[0].BakePolicy, FString(TEXT("StaticBake")));
	TestEqual(TEXT("Mapping asset stores property row material quality flags"), MappingData->PropertyRows[0].MaterialQualityFlags, FString(TEXT("Mid")));
	TestEqual(TEXT("Mapping asset stores property texture layout columns"), MappingData->PropertyTextureColumns.Num(), 17);
	TestEqual(TEXT("Mapping asset stores first layout column"), MappingData->PropertyTextureColumns[0].SourceField, FString(TEXT("baseColorSlice")));
	TestEqual(TEXT("Mapping asset stores VT UV layout column"), MappingData->PropertyTextureColumns[7].SourceField, FString(TEXT("baseVTUVMaxX")));
	TestEqual(TEXT("Mapping asset stores ORM VT UV layout column"), MappingData->PropertyTextureColumns[15].SourceField, FString(TEXT("ormVTUVMaxX")));
	TestEqual(TEXT("Mapping asset stores texture array slice"), MappingData->TextureArraySlices.Num(), 1);
	TestEqual(TEXT("Mapping asset stores source texture path"), MappingData->TextureArraySlices[0].TexturePath, FString(TEXT("/Game/Art/Textures/T_Wall_D.T_Wall_D")));
	TestEqual(TEXT("Mapping asset stores VT atlas entries"), MappingData->VTAtlasEntries.Num(), 1);
	TestEqual(TEXT("Mapping asset stores VT atlas source texture"), MappingData->VTAtlasEntries[0].TexturePath, FString(TEXT("/Game/Art/Textures/T_Wall_D.T_Wall_D")));
	TestEqual(TEXT("Mapping asset stores UDIM-style layout"), MappingData->VTAtlasEntries[0].VirtualTextureLayout, FString(TEXT("UDIMStyleGrid")));
	TestEqual(TEXT("Mapping asset stores UDIM number"), MappingData->VTAtlasEntries[0].UdimNumber, 1001);
	TestEqual(TEXT("Mapping asset stores tile U"), MappingData->VTAtlasEntries[0].TileU, 0);
	TestEqual(TEXT("Mapping asset stores tile V"), MappingData->VTAtlasEntries[0].TileV, 0);
	TestEqual(TEXT("Mapping asset stores tile padding"), MappingData->VTAtlasEntries[0].TilePaddingPixels, 8);
	TestEqual(TEXT("Mapping asset stores UV remap status"), MappingData->VTAtlasEntries[0].UVRemapStatus, FString(TEXT("PlannedForMergedProxyUVRemap")));
	TestEqual(TEXT("Mapping asset stores VT atlas UV minimum"), MappingData->VTAtlasEntries[0].UVRectMin, FVector2D::ZeroVector);
	TestEqual(TEXT("Mapping asset stores property row source texture"), MappingData->PropertyRows[0].SourceTexturePath, FString(TEXT("/Game/Art/Textures/T_Wall_D.T_Wall_D")));
	TestEqual(TEXT("Mapping asset stores property row VT UV minimum"), MappingData->PropertyRows[0].UVRectMin, FVector2D::ZeroVector);
	TestEqual(TEXT("Mapping asset stores property row VT UV maximum"), MappingData->PropertyRows[0].UVRectMax, FVector2D(1.f, 1.f));
	TestTrue(TEXT("Mapping asset estimates VT pool usage"), MappingData->EstimatedVTPoolMB > 0.f);
	TestTrue(TEXT("Mapping asset estimates non-VT streaming usage"), MappingData->EstimatedStreamingPoolMB > 0.f);
	TestEqual(TEXT("Mapping asset stores geometry sources"), MappingData->GeometrySources.Num(), 1);
	TestEqual(TEXT("Mapping asset stores material index channel"), MappingData->MaterialIndexChannel, FString(TEXT("TexCoord7.x")));
	TestEqual(TEXT("Mapping asset stores property texture parameter"), MappingData->PropertyTextureParameterName, FString(TEXT("_PropTexture")));
	TestEqual(TEXT("Mapping asset stores geometry source mesh"), MappingData->GeometrySources[0].StaticMeshPath, FString(TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A")));
	TestEqual(TEXT("Mapping asset stores geometry source location"), MappingData->GeometrySources[0].WorldLocation, FVector(100.0, 200.0, 300.0));
	TestEqual(TEXT("Mapping asset stores material slot remap"), MappingData->GeometrySources[0].MaterialSlotRemap.Num(), 1);
	TestEqual(TEXT("Mapping asset stores remap batch index"), MappingData->GeometrySources[0].MaterialSlotRemap[0].BatchMaterialIndex, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchBuildPlanPropertyTexturePayloadTest,
	"DevKitEditor.MaterialBatch.BuildPlan.BuildsPropertyTexturePayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchBuildPlanPropertyTexturePayloadTest::RunTest(const FString& Parameters)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("Prison S 01");
	Options.TierName = TEXT("Mid");

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);

	FMaterialBatchBuildPlannedEntry FirstEntry;
	FirstEntry.SourceKind = TEXT("MapComponent");
	FirstEntry.ActorName = TEXT("Wall_A");
	FirstEntry.ComponentName = TEXT("StaticMeshComponent0");
	FirstEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_A.SM_Wall_A");
	FirstEntry.MaterialSlotCount = 1;
	FirstEntry.MaterialSlotNames = { TEXT("WallBase") };
	FirstEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_WallBase.MI_WallBase") };
	FirstEntry.LodCount = 2;
	FirstEntry.bCandidate = true;

	FMaterialBatchBuildPlannedEntry SecondEntry = FirstEntry;
	SecondEntry.ActorName = TEXT("Wall_B");
	SecondEntry.AssetPath = TEXT("/Game/Art/Env/SM_Wall_B.SM_Wall_B");
	SecondEntry.MaterialPaths = { TEXT("/Game/Art/Materials/MI_WallTrim.MI_WallTrim") };

	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, { FirstEntry, SecondEntry });

	FMaterialBatchBuildTextureChannelPlan SharedBaseColor;
	SharedBaseColor.ChannelName = TEXT("BaseColor");
	SharedBaseColor.ParameterName = TEXT("BaseMap");
	SharedBaseColor.TexturePath = TEXT("/Game/Art/Textures/T_Shared_D.T_Shared_D");
	SharedBaseColor.TextureClass = TEXT("Texture2D");
	SharedBaseColor.bFoundTexture = true;
	SharedBaseColor.TextureWidth = 1024;
	SharedBaseColor.TextureHeight = 1024;
	Plan.PlannedMaterialRows[0].TextureChannels.Add(SharedBaseColor);
	Plan.PlannedMaterialRows[1].TextureChannels.Add(SharedBaseColor);

	FMaterialBatchBuildTextureChannelPlan Normal;
	Normal.ChannelName = TEXT("Normal");
	Normal.ParameterName = TEXT("NormalMap");
	Normal.TexturePath = TEXT("/Game/Art/Textures/T_Wall_N.T_Wall_N");
	Normal.TextureClass = TEXT("Texture2D");
	Normal.bFoundTexture = true;
	Normal.TextureWidth = 1024;
	Normal.TextureHeight = 1024;
	Plan.PlannedMaterialRows[1].TextureChannels.Add(Normal);

	const FMaterialBatchBuildPropertyTexturePayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildPropertyTexturePayload(Plan);

	TestEqual(TEXT("Property texture width matches layout column count"), Payload.Width, 17);
	TestEqual(TEXT("Property texture height matches material row count"), Payload.Height, 2);
	TestEqual(TEXT("Property texture stores one RGBA16F pixel per property"), Payload.Pixels.Num(), 34);
	TestEqual(TEXT("First material base color slice is zero"), static_cast<int32>(Payload.Pixels[0].R.GetFloat()), 0);
	TestEqual(TEXT("First material missing normal stays -1"), static_cast<int32>(Payload.Pixels[1].R.GetFloat()), -1);
	TestEqual(TEXT("First material VT UV min X is zero"), Payload.Pixels[5].R.GetFloat(), 0.0f);
	TestEqual(TEXT("First material VT UV max X is one half"), Payload.Pixels[7].R.GetFloat(), 0.5f);
	TestEqual(TEXT("First material missing normal VT UV max X defaults to one"), Payload.Pixels[11].R.GetFloat(), 1.0f);
	TestEqual(TEXT("Second material shares base color slice zero"), static_cast<int32>(Payload.Pixels[17].R.GetFloat()), 0);
	TestEqual(TEXT("Second material normal slice is zero"), static_cast<int32>(Payload.Pixels[18].R.GetFloat()), 0);
	TestEqual(TEXT("Second material normal VT UV min X points into atlas"), Payload.Pixels[26].R.GetFloat(), 0.5f);
	TestEqual(TEXT("Second material normal VT UV max X points into atlas"), Payload.Pixels[28].R.GetFloat(), 1.0f);
	TestFalse(TEXT("Generated property texture is linear data, not sRGB"), Payload.bSRGB);
	TestEqual(TEXT("Generated property texture uses stable source format"), Payload.SourceFormat, ETextureSourceFormat::TSF_RGBA16F);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchMaterialAuditReportTest,
	"DevKitEditor.MaterialBatch.MaterialAudit.BuildsMarkdownReport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchMaterialAuditReportTest::RunTest(const FString& Parameters)
{
	FMaterialBatchMaterialAuditResult Result;
	Result.MaterialPath = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building");
	Result.MaterialName = TEXT("M_Env_Building");
	Result.MaterialClass = TEXT("Material");
	Result.bLoaded = true;
	Result.BlendMode = TEXT("Opaque");
	Result.ShadingModel = TEXT("DefaultLit");
	Result.bTwoSided = false;
	Result.bMasked = false;
	Result.bUsesWorldPositionOffset = false;

	FMaterialBatchMaterialTextureParameter TextureParameter;
	TextureParameter.ChannelName = TEXT("BaseColor");
	TextureParameter.ParameterName = TEXT("BaseMap");
	TextureParameter.TexturePath = TEXT("/Game/Art/Textures/T_Building_D.T_Building_D");
	TextureParameter.TextureClass = TEXT("Texture2D");
	TextureParameter.bFoundTexture = true;
	Result.TextureParameters.Add(TextureParameter);

	FMaterialBatchMaterialScalarParameter ScalarParameter;
	ScalarParameter.ParameterName = TEXT("MaterialLightSampleCount");
	ScalarParameter.Value = 4.0;
	ScalarParameter.bFoundValue = true;
	Result.ScalarParameters.Add(ScalarParameter);

	const TArray<FString> ReportLines = FMaterialBatchMaterialAuditBuilder::BuildMarkdownReport(Result);
	TestTrue(TEXT("Report includes material audit heading"), ReportLines.Contains(TEXT("# Material Batch Material Audit")));
	TestTrue(TEXT("Report includes target material"), ReportLines.Contains(TEXT("- Material: `/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building`")));
	TestTrue(TEXT("Report includes texture parameter table"), ReportLines.Contains(TEXT("## Texture Parameters")));
	TestTrue(
		TEXT("Report includes classified texture parameter"),
		ReportLines.Contains(TEXT("| BaseColor | `BaseMap` | `/Game/Art/Textures/T_Building_D.T_Building_D` | Texture2D | Yes |")));
	TestTrue(
		TEXT("Report includes scalar parameter"),
		ReportLines.Contains(TEXT("| `MaterialLightSampleCount` | 4.000 | Yes |")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchMaterialAuditLoadsTargetMaterialTest,
	"DevKitEditor.MaterialBatch.MaterialAudit.LoadsTargetEnvBuildingMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchMaterialAuditLoadsTargetMaterialTest::RunTest(const FString& Parameters)
{
	const FString TargetMaterial = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Building.M_Env_Building");
	const FMaterialBatchMaterialAuditResult Result = FMaterialBatchMaterialAuditBuilder::AuditMaterial(TargetMaterial);

	TestTrue(TEXT("Target environment material loads"), Result.bLoaded);
	TestEqual(TEXT("Audit preserves requested material path"), Result.MaterialPath, TargetMaterial);
	TestTrue(TEXT("Audit records a material class"), !Result.MaterialClass.IsEmpty());

	const TArray<FString> ReportLines = FMaterialBatchMaterialAuditBuilder::BuildMarkdownReport(Result);
	TestTrue(TEXT("Report includes target material path"), ReportLines.Contains(FString::Printf(TEXT("- Material: `%s`"), *TargetMaterial)));
	TestTrue(TEXT("Report includes texture parameters section"), ReportLines.Contains(TEXT("## Texture Parameters")));
	TestTrue(TEXT("Report includes batch compatibility section"), ReportLines.Contains(TEXT("## Batch Compatibility Notes")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaterialBatchParentMaterialAssetContractTest,
	"DevKitEditor.MaterialBatch.ParentMaterial.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMaterialBatchParentMaterialAssetContractTest::RunTest(const FString& Parameters)
{
	const FString TargetMaterial = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas.M_Env_Baked_VTAtlas");
	FMaterialBatchBuildPlanOptions Options;
	Options.ClusterName = TEXT("ParentMaterialContract");
	const FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);
	TestEqual(TEXT("Batch parent material interface path stays stable"), Plan.BatchParentMaterialPackage, TargetMaterial);

	UMaterial* Material = LoadObject<UMaterial>(nullptr, *TargetMaterial);
	if (!Material)
	{
		AddInfo(TEXT("Batch parent material asset is not generated in the current interface-only material phase; skipping node graph checks until the asset generation phase runs."));
		return true;
	}

	TestNotNull(TEXT("Generated batch parent material loads"), Material);
	if (!Material->GetEditorOnlyData())
	{
		return false;
	}

	bool bHasTexCoord7 = false;
	bool bHasBaseColorArray = false;
	bool bHasNormalArray = false;
	bool bHasOrmArray = false;
	bool bHasVTAtlas = false;
	bool bHasPropertyTexture = false;
	bool bHasBatchRowCount = false;
	bool bHasPropertyColumnCount = false;
	bool bHasTextureArraySample = false;
	bool bHasVTAtlasSample = false;
	bool bHasPropertyTextureSample = false;

	for (const TObjectPtr<UMaterialExpression>& ExpressionPtr : Material->GetEditorOnlyData()->ExpressionCollection.Expressions)
	{
		const UMaterialExpression* Expression = ExpressionPtr.Get();
		if (const UMaterialExpressionTextureCoordinate* TexCoord = Cast<UMaterialExpressionTextureCoordinate>(Expression))
		{
			bHasTexCoord7 |= TexCoord->CoordinateIndex == 7;
		}
		else if (const UMaterialExpressionTextureObjectParameter* TextureParameter = Cast<UMaterialExpressionTextureObjectParameter>(Expression))
		{
			bHasBaseColorArray |= TextureParameter->ParameterName == TEXT("T_Array_A");
			bHasNormalArray |= TextureParameter->ParameterName == TEXT("T_Array_N");
			bHasOrmArray |= TextureParameter->ParameterName == TEXT("T_Array_M");
			bHasVTAtlas |= TextureParameter->ParameterName == TEXT("VT_Atlas");
			bHasPropertyTexture |= TextureParameter->ParameterName == TEXT("_PropTexture");
		}
		else if (const UMaterialExpressionScalarParameter* ScalarParameter = Cast<UMaterialExpressionScalarParameter>(Expression))
		{
			bHasBatchRowCount |= ScalarParameter->ParameterName == TEXT("BatchRowCount");
			bHasPropertyColumnCount |= ScalarParameter->ParameterName == TEXT("PropertyColumnCount");
		}
		else if (const UMaterialExpressionCustom* CustomNode = Cast<UMaterialExpressionCustom>(Expression))
		{
			bHasTextureArraySample |= CustomNode->Code.Contains(TEXT("Texture2DArraySample"));
			bHasVTAtlasSample |= CustomNode->Code.Contains(TEXT("Texture2DSample(VT_Atlas"));
			bHasPropertyTextureSample |= CustomNode->Code.Contains(TEXT("Texture2DSample(_PropTexture"));
		}
	}

	TestTrue(TEXT("Batch parent reads TexCoord7.x material index input"), bHasTexCoord7);
	TestTrue(TEXT("Batch parent exposes T_Array_A"), bHasBaseColorArray);
	TestTrue(TEXT("Batch parent exposes T_Array_N"), bHasNormalArray);
	TestTrue(TEXT("Batch parent exposes T_Array_M"), bHasOrmArray);
	TestTrue(TEXT("Batch parent exposes VT_Atlas"), bHasVTAtlas);
	TestTrue(TEXT("Batch parent exposes _PropTexture"), bHasPropertyTexture);
	TestTrue(TEXT("Batch parent exposes BatchRowCount"), bHasBatchRowCount);
	TestTrue(TEXT("Batch parent exposes PropertyColumnCount"), bHasPropertyColumnCount);
	TestFalse(TEXT("Batch parent VT path does not sample Texture2DArray"), bHasTextureArraySample);
	TestTrue(TEXT("Batch parent custom code samples VT atlas"), bHasVTAtlasSample);
	TestTrue(TEXT("Batch parent custom code samples property texture"), bHasPropertyTextureSample);
	TestNotNull(TEXT("Batch parent connects BaseColor"), Material->GetEditorOnlyData()->BaseColor.Expression);
	TestNotNull(TEXT("Batch parent connects Normal"), Material->GetEditorOnlyData()->Normal.Expression);
	TestNotNull(TEXT("Batch parent connects Roughness"), Material->GetEditorOnlyData()->Roughness.Expression);

	return true;
}

#endif
