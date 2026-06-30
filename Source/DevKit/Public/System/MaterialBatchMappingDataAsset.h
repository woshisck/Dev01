#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MaterialBatchMappingDataAsset.generated.h"

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingTextureChannel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ChannelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TextureClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bFoundTexture = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 Width = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 Height = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bArrayBuildEligible = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ArrayBuildReason;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingMaterialRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 BatchMaterialIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceEntryIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MaterialSlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceKind;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourcePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString StaticMeshPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaterialSlotName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaterialPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingTextureChannel> TextureChannels;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingTextureArraySlice
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ChannelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SliceIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TextureClass;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingVTAtlasEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 AtlasEntryIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ChannelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TextureClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 Width = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 Height = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString VirtualTextureLayout = TEXT("UDIMStyleGrid");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 UdimNumber = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 TileU = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 TileV = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 TilePaddingPixels = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString UVRemapStatus = TEXT("PlannedForMergedProxyUVRemap");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D UVRectMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D UVRectMax = FVector2D(1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	float EstimatedSourceMB = 0.f;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingPropertyTextureColumn
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 PropertyIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceField;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ValueType;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 DefaultIntValue = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString Description;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingPropertyRow
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 BatchMaterialIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaterialPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 BaseColorSlice = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 NormalSlice = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 OrmSlice = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 EmissiveSlice = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MaskSlice = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LightInfoTexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D UVRectMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D UVRectMax = FVector2D(1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceTexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString NormalSourceTexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D NormalUVRectMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D NormalUVRectMax = FVector2D(1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString OrmSourceTexturePath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D OrmUVRectMin = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector2D OrmUVRectMax = FVector2D(1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SurfaceKind;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BakePolicy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaterialQualityFlags;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingMaterialSlotRemap
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceMaterialSlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 BatchMaterialIndex = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingGeometrySource
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceEntryIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString StaticMeshPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bHasWorldTransform = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FRotator WorldRotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FVector WorldScale = FVector::OneVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 FirstBatchMaterialIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 BatchMaterialIndexCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingMaterialSlotRemap> MaterialSlotRemap;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingTierLayerSelection
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TierName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bLoadSourceLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bLoadProxyLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bLoadBakedLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString FallbackPolicy;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyLayerPlan
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ExclusivityGroup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceLayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerBackend = TEXT("StreamingLevel");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ProxyLayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BakedLayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bRequiresMutualExclusion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bHasTagConflicts = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceActorCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ProxyActorCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 BakedActorCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ConflictActorCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingTierLayerSelection> TierSelections;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyLayerAssignment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceEntryIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerRole;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ExpectedLayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FString> ActualLayerNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActualStreamingLevelName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActualLevelPackageName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FString> ActualDataLayerNames;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bReadyForLayerValidation = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bHasActualLayerEvidence = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bHasActualDataLayerEvidence = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bMatchesExpectedLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bMatchesExpectedDataLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ReadinessReason;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerValidationStatus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString DataLayerValidationStatus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FString> EnvBatchTags;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyLayerReadiness
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerBackend = TEXT("StreamingLevel");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 EntryCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ReadyEntryCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MissingLayerTagEntryCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ConflictEntryCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ExcludedEntryCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ActualLayerMatchCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MissingActualLayerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 UnexpectedActualLayerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 NotRequiredActualLayerCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingSourceProxyLayerAssignment> Assignments;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyAssetAssignment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceEntryIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerRole;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ProxyAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceLODIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ProxyLODIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bHasSourceAsset = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bHasProxyAsset = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bUsesGeneratedProxy = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bUsesAuthoredProxy = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bReadyForAssetPairing = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ReadinessStatus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ReadinessReason;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FString> EnvBatchTags;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyAssetReadiness
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 EntryCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ReadyPairCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MissingSourceAssetCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MissingProxyAssetCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 GeneratedProxyFallbackCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 AuthoredProxyCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 NotRequiredCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ConflictCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingSourceProxyAssetAssignment> Assignments;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyAssetConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ObjectKey;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ActorName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ComponentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerRole;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceAssetPath;

	// Legacy property name kept for saved asset compatibility. Current workflow treats this as an optional explicit proxy, not a required AuthorProxy asset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString AuthorProxyAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString GeneratedProxyAssetPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 SourceLODIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ProxyLODIndex = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SurfaceKind;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BakePolicy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString InteractionPolicy;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ConfigSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bUsesGeneratedProxyFallback = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bReadyForAssetPairing = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ReadinessStatus;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ReadinessReason;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingSourceProxyAssetConfigSet
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ConfigCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 ReadyConfigCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 GeneratedFallbackConfigCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 AuthoredProxyConfigCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	int32 MissingSourceReferenceCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingSourceProxyAssetConfig> Configs;
};

USTRUCT(BlueprintType)
struct DEVKIT_API FMaterialBatchMappingResidencyRiskPlan
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TextureBackend;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bVTAtlasMainPath = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bTextureArrayFallbackPresent = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bAllowTextureArrayFallbackInProduction = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bDuplicateResidencyRisk = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bRequiresSourceProxyUnload = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	float EstimatedVTPoolMB = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	float EstimatedStreamingPoolMB = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	float EstimatedCombinedPoolMB = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ResidencyGate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString Recommendation;
};

UCLASS(BlueprintType)
class DEVKIT_API UMaterialBatchMappingDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString Schema = TEXT("DevKit.MaterialBatchMappingData.v1");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bDryRunSource = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString RootPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MapPath;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString DataLayerName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString LayerBackend = TEXT("StreamingLevel");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ClusterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TierName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString OutputFolder;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString ProxyMeshPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BatchMaterialInstancePackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BatchParentMaterialPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString TextureBackend = TEXT("VTAtlas");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString VTAtlasPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString VTAtlasChannel = TEXT("Combined");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SurfaceKind = TEXT("MixedStatic");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BakePolicy = TEXT("StaticBake");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceProxyExclusivityGroup;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	float EstimatedStreamingPoolMB = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	float EstimatedVTPoolMB = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	bool bDuplicateResidencyRisk = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FMaterialBatchMappingSourceProxyLayerPlan SourceProxyLayerPlan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FMaterialBatchMappingSourceProxyLayerReadiness SourceProxyLayerReadiness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FMaterialBatchMappingSourceProxyAssetReadiness SourceProxyAssetReadiness;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FMaterialBatchMappingSourceProxyAssetConfigSet SourceProxyAssetConfigSet;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FMaterialBatchMappingResidencyRiskPlan ResidencyRiskPlan;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString BaseColorArrayPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString NormalArrayPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString OrmArrayPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString EmissiveArrayPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaskArrayPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString PropertyTexturePackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MappingDataAssetPackage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString SourceCoordinateSpace = TEXT("World");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaterialIndexChannel = TEXT("TexCoord7.x");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString MaterialIndexEncoding = TEXT("write batchMaterialIndex as a float per merged vertex");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString PropertyTextureParameterName = TEXT("_PropTexture");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	FString PropertyTextureRowEncoding = TEXT("sample _PropTexture at x=(propertyIndex+0.5)/width, y=(batchMaterialIndex+0.5)/height");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingMaterialRow> MaterialRows;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingTextureArraySlice> TextureArraySlices;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingVTAtlasEntry> VTAtlasEntries;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingPropertyTextureColumn> PropertyTextureColumns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingPropertyRow> PropertyRows;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingGeometrySource> GeometrySources;
};
