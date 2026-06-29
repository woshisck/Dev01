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
	TArray<FMaterialBatchMappingPropertyTextureColumn> PropertyTextureColumns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingPropertyRow> PropertyRows;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Material Batch")
	TArray<FMaterialBatchMappingGeometrySource> GeometrySources;
};
