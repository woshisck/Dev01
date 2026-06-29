#pragma once

#include "CoreMinimal.h"
#include "Engine/TextureDefines.h"
#include "Math/Float16Color.h"

struct FMaterialBatchBuildPlanOptions
{
	FString RootPath = TEXT("/Game/Art");
	FString MapPath;
	FString DataLayerName;
	FString ClusterName = TEXT("Default");
	FString TierName = TEXT("Medium");
	FString RulesPath;
	FString OutputRoot = TEXT("/Game/Generated/MaterialBatch");
	bool bDryRun = true;
};

struct FMaterialBatchBuildCandidateSummary
{
	FString SourceKind;
	int32 SourceFoundCount = 0;
	int32 SourceInspectedCount = 0;
	int32 BatchCandidateCount = 0;
	int32 RejectedCount = 0;
};

struct FMaterialBatchBuildPlannedEntry
{
	FString SourceKind;
	FString SourcePath;
	FString ActorName;
	FString ComponentName;
	FString AssetPath;
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

struct FMaterialBatchBuildProxyMeshSourcePayload
{
	int32 SourceEntryIndex = INDEX_NONE;
	FString ActorName;
	FString ComponentName;
	FString StaticMeshPath;
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
	static FMaterialBatchBuildPropertyTexturePayload BuildPropertyTexturePayload(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildProxyMeshPayload BuildProxyMeshPayload(const FMaterialBatchBuildPlan& Plan);
	static FMaterialBatchBuildBatchMaterialPayload BuildBatchMaterialPayload(const FMaterialBatchBuildPlan& Plan);
	static void PopulateMappingDataAsset(const FMaterialBatchBuildPlan& Plan, UMaterialBatchMappingDataAsset& MappingData);
	static TArray<FString> BuildMarkdownReport(const FMaterialBatchBuildPlan& Plan);
	static FString BuildJsonManifest(const FMaterialBatchBuildPlan& Plan);

private:
	static FString SanitizePackageSegment(const FString& Segment, const FString& Fallback);
	static FString NormalizeOutputRoot(const FString& OutputRoot);
};
