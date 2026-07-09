#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureDefines.h"
#include "Math/Float16Color.h"

struct FMaterialBatchBuildPlanOptions
{
	FString RootPath = TEXT("/Game/Art");
	FString MapPath;
	FString DataLayerName; // Legacy compatibility only; current Dev01 environment batching uses StreamingLevel.
	FString LayerBackend = TEXT("StreamingLevel");
	FString ClusterName = TEXT("Default");
	FString TierName = TEXT("Mid");
	FString TextureBackend = TEXT("VTAtlas");
	FString SurfaceKind = TEXT("MixedStatic");
	FString BakePolicy = TEXT("StaticBake");
	FString SourceProxyExclusivityGroup;
	FString RulesPath;
	FString OutputRoot = TEXT("/Game/Generated/MaterialBatch");
	bool bDryRun = true;
	bool bReportStaticDecals = false;
	bool bValidateSourceProxyExclusivity = false;
};

struct FMaterialBatchBuildCandidateSummary
{
	FString SourceKind;
	int32 SourceFoundCount = 0;
	int32 SourceInspectedCount = 0;
	int32 BatchCandidateCount = 0;
	int32 RejectedCount = 0;
};

struct FMaterialBatchBuildTagDiagnostics
{
	int32 ActorCount = 0;
	int32 SourceActorCount = 0;
	int32 ProxyActorCount = 0;
	int32 BakedActorCount = 0;
	int32 ExcludeActorCount = 0;
	int32 SourceProxyConflictActorCount = 0;
	int32 BakeStaticDecalActorCount = 0;
	int32 RuntimeDecalActorCount = 0;
	int32 GameplayIndicatorActorCount = 0;
	TArray<FString> Warnings;
};

struct FMaterialBatchBuildTierLayerSelection
{
	FString TierName;
	bool bLoadSourceLayer = false;
	bool bLoadProxyLayer = false;
	bool bLoadBakedLayer = false;
	FString FallbackPolicy;
};

struct FMaterialBatchBuildSourceProxyLayerPlan
{
	FString ExclusivityGroup;
	FString LayerBackend = TEXT("StreamingLevel");
	FString SourceLayerName;
	FString ProxyLayerName;
	FString BakedLayerName;
	bool bRequiresMutualExclusion = true;
	bool bHasTagConflicts = false;
	int32 SourceActorCount = 0;
	int32 ProxyActorCount = 0;
	int32 BakedActorCount = 0;
	int32 ConflictActorCount = 0;
	TArray<FMaterialBatchBuildTierLayerSelection> TierSelections;
};

struct FMaterialBatchBuildSourceProxyLayerAssignment
{
	int32 SourceEntryIndex = INDEX_NONE;
	FString ActorName;
	FString ComponentName;
	FString LayerRole;
	FString ExpectedLayerName;
	TArray<FString> ActualLayerNames;
	FString ActualStreamingLevelName;
	FString ActualLevelPackageName;
	TArray<FString> ActualDataLayerNames;
	bool bReadyForLayerValidation = false;
	bool bHasActualLayerEvidence = false;
	bool bHasActualDataLayerEvidence = false;
	bool bMatchesExpectedLayer = false;
	bool bMatchesExpectedDataLayer = false;
	FString ReadinessReason;
	FString LayerValidationStatus;
	FString DataLayerValidationStatus;
	TArray<FString> EnvBatchTags;
};

struct FMaterialBatchBuildSourceProxyLayerReadiness
{
	FString LayerBackend = TEXT("StreamingLevel");
	int32 EntryCount = 0;
	int32 ReadyEntryCount = 0;
	int32 MissingLayerTagEntryCount = 0;
	int32 ConflictEntryCount = 0;
	int32 ExcludedEntryCount = 0;
	int32 ActualLayerMatchCount = 0;
	int32 MissingActualLayerCount = 0;
	int32 UnexpectedActualLayerCount = 0;
	int32 NotRequiredActualLayerCount = 0;
	TArray<FMaterialBatchBuildSourceProxyLayerAssignment> Assignments;
};

struct FMaterialBatchBuildSourceProxyAssetAssignment
{
	int32 SourceEntryIndex = INDEX_NONE;
	FString ActorName;
	FString ComponentName;
	FString LayerRole;
	FString SourceAssetPath;
	FString ProxyAssetPath;
	int32 SourceLODIndex = 0;
	int32 ProxyLODIndex = 1;
	bool bHasSourceAsset = false;
	bool bHasProxyAsset = false;
	bool bUsesGeneratedProxy = false;
	bool bUsesAuthoredProxy = false;
	bool bReadyForAssetPairing = false;
	FString ReadinessStatus;
	FString ReadinessReason;
	TArray<FString> EnvBatchTags;
};

struct FMaterialBatchBuildSourceProxyAssetReadiness
{
	int32 EntryCount = 0;
	int32 ReadyPairCount = 0;
	int32 MissingSourceAssetCount = 0;
	int32 MissingProxyAssetCount = 0;
	int32 GeneratedProxyFallbackCount = 0;
	int32 AuthoredProxyCount = 0;
	int32 NotRequiredCount = 0;
	int32 ConflictCount = 0;
	TArray<FMaterialBatchBuildSourceProxyAssetAssignment> Assignments;
};

struct FMaterialBatchBuildSourceProxyAssetConfig
{
	FString ObjectKey;
	FString ActorName;
	FString ComponentName;
	FString LayerRole;
	FString SourceAssetPath;
	// Legacy field name kept for manifest/mapping compatibility. Current workflow treats this as an optional explicit proxy, not a required AuthorProxy asset.
	FString AuthorProxyAssetPath;
	FString GeneratedProxyAssetPath;
	int32 SourceLODIndex = 0;
	int32 ProxyLODIndex = 1;
	FString SurfaceKind;
	FString BakePolicy;
	FString InteractionPolicy;
	FString ConfigSource;
	bool bUsesGeneratedProxyFallback = false;
	bool bReadyForAssetPairing = false;
	FString ReadinessStatus;
	FString ReadinessReason;
};

struct FMaterialBatchBuildSourceProxyAssetConfigSet
{
	int32 ConfigCount = 0;
	int32 ReadyConfigCount = 0;
	int32 GeneratedFallbackConfigCount = 0;
	int32 AuthoredProxyConfigCount = 0;
	int32 MissingSourceReferenceCount = 0;
	TArray<FMaterialBatchBuildSourceProxyAssetConfig> Configs;
};

struct FMaterialBatchBuildResidencyRiskPlan
{
	FString TextureBackend;
	bool bVTAtlasMainPath = false;
	bool bTextureArrayFallbackPresent = false;
	bool bAllowTextureArrayFallbackInProduction = false;
	bool bDuplicateResidencyRisk = false;
	bool bRequiresSourceProxyUnload = false;
	float EstimatedVTPoolMB = 0.f;
	float EstimatedStreamingPoolMB = 0.f;
	float EstimatedCombinedPoolMB = 0.f;
	FString ResidencyGate;
	FString Recommendation;
};

struct FMaterialBatchBuildPlannedEntry
{
	FString SourceKind;
	FString SourcePath;
	FString ActorName;
	FString ComponentName;
	FString AssetPath;
	TArray<FString> EnvBatchTags;
	TArray<FString> ActualLayerNames;
	FString ActualStreamingLevelName;
	FString ActualLevelPackageName;
	TArray<FString> ActualDataLayerNames;
	bool bHasWorldTransform = false;
	FVector WorldLocation = FVector::ZeroVector;
	FRotator WorldRotation = FRotator::ZeroRotator;
	FVector WorldScale = FVector::OneVector;
	int32 MaterialSlotCount = 0;
	TArray<FString> MaterialSlotNames;
	TArray<FString> MaterialPaths;
	int32 LodCount = 0;
	bool bCandidate = false;
	FString RejectReason = TEXT("None");
	int32 FirstBatchMaterialIndex = INDEX_NONE;
	int32 BatchMaterialIndexCount = 0;
};

struct FMaterialBatchBuildTextureChannelPlan
{
	FString ChannelName = TEXT("Unknown");
	FString ParameterName;
	FString TexturePath;
	FString TextureClass;
	bool bFoundTexture = false;
	int32 TextureWidth = INDEX_NONE;
	int32 TextureHeight = INDEX_NONE;
	bool bTextureArrayBuildEligible = false;
	FString TextureArrayBuildReason = TEXT("NotEvaluated");
};

struct FMaterialBatchBuildMaterialRow
{
	int32 BatchMaterialIndex = INDEX_NONE;
	int32 SourceEntryIndex = INDEX_NONE;
	int32 MaterialSlotIndex = INDEX_NONE;
	FString SourceKind;
	FString SourcePath;
	FString ActorName;
	FString ComponentName;
	FString AssetPath;
	FString MaterialSlotName;
	FString MaterialPath;
	TArray<FMaterialBatchBuildTextureChannelPlan> TextureChannels;
};

struct FMaterialBatchBuildPlan
{
	FMaterialBatchBuildPlanOptions Options;
	FString SanitizedClusterName;
	FString SanitizedTierName;
	FString OutputFolder;
	FString ProxyMeshPackage;
	FString BatchMaterialInstancePackage;
	FString BatchParentMaterialPackage;
	FString TextureBackend;
	FString SurfaceKind;
	FString BakePolicy;
	FString SourceProxyExclusivityGroup;
	FString VTAtlasPackage;
	FString VTAtlasChannel = TEXT("Combined");
	FString BaseColorArrayPackage;
	FString NormalArrayPackage;
	FString OrmArrayPackage;
	FString EmissiveArrayPackage;
	FString MaskArrayPackage;
	FString PropertyTexturePackage;
	FString MappingDataAssetPackage;
	FString BatchPropertyTextureParameterName;
	FString BatchMaterialIndexChannel = TEXT("TexCoord7.x");
	FString BatchMaterialIndexSource;
	FString BatchPropertyTextureRowEncoding;
	TArray<FString> GeneratedPackageNames;
	FMaterialBatchBuildCandidateSummary CandidateSummary;
	FMaterialBatchBuildTagDiagnostics TagDiagnostics;
	FMaterialBatchBuildSourceProxyLayerPlan SourceProxyLayerPlan;
	FMaterialBatchBuildSourceProxyLayerReadiness SourceProxyLayerReadiness;
	FMaterialBatchBuildSourceProxyAssetReadiness SourceProxyAssetReadiness;
	FMaterialBatchBuildSourceProxyAssetConfigSet SourceProxyAssetConfigSet;
	FMaterialBatchBuildResidencyRiskPlan ResidencyRiskPlan;
	TArray<FMaterialBatchBuildPlannedEntry> PlannedEntries;
	TArray<FMaterialBatchBuildMaterialRow> PlannedMaterialRows;
	int32 NextBatchMaterialIndex = 0;
	bool bDryRun = true;
};

struct FMaterialBatchBuildPropertyTexturePayload
{
	int32 Width = 0;
	int32 Height = 0;
	bool bSRGB = false;
	ETextureSourceFormat SourceFormat = ETextureSourceFormat::TSF_RGBA16F;
	TArray<FFloat16Color> Pixels;
};

struct FMaterialBatchBuildTextureArrayPayload
{
	FString ChannelName;
	FString PackagePath;
	int32 Width = INDEX_NONE;
	int32 Height = INDEX_NONE;
	TArray<FString> SourceTexturePaths;
};

struct FMaterialBatchBuildVTAtlasEntry
{
	int32 AtlasEntryIndex = INDEX_NONE;
	FString ChannelName;
	FString TexturePath;
	FString TextureClass;
	int32 Width = INDEX_NONE;
	int32 Height = INDEX_NONE;
	FString VirtualTextureLayout = TEXT("UDIMStyleGrid");
	int32 UdimNumber = INDEX_NONE;
	int32 TileU = INDEX_NONE;
	int32 TileV = INDEX_NONE;
	int32 TilePaddingPixels = 8;
	FString UVRemapStatus = TEXT("PlannedForMergedProxyUVRemap");
	FVector2D UVRectMin = FVector2D::ZeroVector;
	FVector2D UVRectMax = FVector2D(1.f, 1.f);
	float EstimatedSourceMB = 0.f;
};

struct FMaterialBatchBuildVTAtlasPayload
{
	FString PackagePath;
	FString LayoutPolicy = TEXT("DeterministicGridByUniqueTexture");
	FString VirtualTextureLayout = TEXT("UDIMStyleGrid");
	int32 Width = 0;
	int32 Height = 0;
	int32 Columns = 0;
	int32 Rows = 0;
	int32 TilePaddingPixels = 8;
	TArray<FMaterialBatchBuildVTAtlasEntry> Entries;
};

struct FMaterialBatchBuildProxyMeshSourcePayload
{
	int32 SourceEntryIndex = INDEX_NONE;
	FString ActorName;
	FString ComponentName;
	FString StaticMeshPath;
	bool bBakeInstanceVertexColors = false;
	bool bHasWorldTransform = false;
	FVector WorldLocation = FVector::ZeroVector;
	FRotator WorldRotation = FRotator::ZeroRotator;
	FVector WorldScale = FVector::OneVector;
	TArray<int32> MaterialSlotToBatchMaterialIndex;
};

struct FMaterialBatchBuildProxyMeshPayload
{
	FString PackagePath;
	FString MaterialIndexChannel = TEXT("TexCoord7.x");
	TArray<FMaterialBatchBuildProxyMeshSourcePayload> Sources;
};

struct FMaterialBatchBuildBatchMaterialTextureBinding
{
	FString ParameterName;
	FString TexturePackagePath;
};

struct FMaterialBatchBuildBatchMaterialScalarBinding
{
	FString ParameterName;
	float Value = 0.0f;
};

struct FMaterialBatchBuildBatchMaterialPayload
{
	FString PackagePath;
	FString ParentMaterialPath;
	TArray<FMaterialBatchBuildBatchMaterialTextureBinding> TextureBindings;
	TArray<FMaterialBatchBuildBatchMaterialScalarBinding> ScalarBindings;
};

class UMaterialBatchMappingDataAsset;

class FMaterialBatchBuildPlanBuilder
{
public:
	static FMaterialBatchBuildPlan CreateDryRunPlan(const FMaterialBatchBuildPlanOptions& Options);
	static void ApplyCandidateSummary(FMaterialBatchBuildPlan& Plan, const FMaterialBatchBuildCandidateSummary& Summary);
	static void ApplyPlannedEntries(FMaterialBatchBuildPlan& Plan, const TArray<FMaterialBatchBuildPlannedEntry>& Entries);
	static void ApplyTextureChannelPlans(FMaterialBatchBuildPlan& Plan);
	static FString ClassifyTextureChannelName(const FString& ParameterName);
	static TArray<FMaterialBatchBuildTextureArrayPayload> BuildTextureArrayPayloads(const FMaterialBatchBuildPlan& Plan);
	static TArray<FMaterialBatchBuildVTAtlasEntry> BuildVTAtlasEntries(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildVTAtlasPayload BuildVTAtlasPayload(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildPropertyTexturePayload BuildPropertyTexturePayload(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildProxyMeshPayload BuildProxyMeshPayload(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildSourceProxyLayerPlan BuildSourceProxyLayerPlan(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildSourceProxyLayerReadiness BuildSourceProxyLayerReadiness(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildSourceProxyAssetReadiness BuildSourceProxyAssetReadiness(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildSourceProxyAssetConfigSet BuildSourceProxyAssetConfigSet(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildResidencyRiskPlan BuildResidencyRiskPlan(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildBatchMaterialPayload BuildBatchMaterialPayload(const FMaterialBatchBuildPlan& Plan);
	static void PopulateMappingDataAsset(const FMaterialBatchBuildPlan& Plan, UMaterialBatchMappingDataAsset& MappingData);
	static TArray<FString> BuildMarkdownReport(const FMaterialBatchBuildPlan& Plan);
	static FString BuildJsonManifest(const FMaterialBatchBuildPlan& Plan);

private:
	static FString SanitizePackageSegment(const FString& Segment, const FString& Fallback);
	static FString NormalizeOutputRoot(const FString& OutputRoot);
};
