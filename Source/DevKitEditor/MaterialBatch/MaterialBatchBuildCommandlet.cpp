#include "MaterialBatch/MaterialBatchBuildCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "EditorLevelUtils.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/Texture2DArray.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameFramework/Actor.h"
#include "MaterialBatch/MaterialBatchAuditHelpers.h"
#include "MaterialBatch/MaterialBatchBuildPlan.h"
#include "MaterialBatch/MaterialBatchCandidateRules.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Modules/ModuleManager.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
#include "System/MaterialBatchMappingDataAsset.h"

namespace
{
const TCHAR* MaterialBatchBuildReportFileName = TEXT("MaterialBatchBuildReport.md");
const TCHAR* MaterialBatchBuildManifestFileName = TEXT("MaterialBatchBuildManifest.json");

FString GetParamValue(const FString& Params, const TCHAR* Key, const FString& DefaultValue)
{
	FString Value;
	if (FParse::Value(*Params, Key, Value))
	{
		return Value;
	}
	return DefaultValue;
}

int32 GetParamInt(const FString& Params, const TCHAR* Key, int32 DefaultValue)
{
	int32 Value = DefaultValue;
	FParse::Value(*Params, Key, Value);
	return Value;
}

bool HasSwitch(const FString& Params, const TCHAR* SwitchName)
{
	return FParse::Param(*Params, SwitchName);
}

FString GetMaterialPath(const UMaterialInterface* Material)
{
	return Material ? Material->GetPathName() : TEXT("(no material)");
}

FString BuildObjectPathFromPackagePath(const FString& PackagePath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
}

bool SaveMappingDataAsset(const FMaterialBatchBuildPlan& Plan, FString& OutObjectPath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(Plan.MappingDataAssetPackage);
	if (AssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not derive asset name from mapping package: %s"), *Plan.MappingDataAssetPackage);
		return false;
	}

	const FString ObjectPath = BuildObjectPathFromPackagePath(Plan.MappingDataAssetPackage);
	UMaterialBatchMappingDataAsset* MappingData = Cast<UMaterialBatchMappingDataAsset>(
		StaticLoadObject(UMaterialBatchMappingDataAsset::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = MappingData ? MappingData->GetOutermost() : CreatePackage(*Plan.MappingDataAssetPackage);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not create mapping package: %s"), *Plan.MappingDataAssetPackage);
		return false;
	}

	if (!MappingData)
	{
		MappingData = NewObject<UMaterialBatchMappingDataAsset>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(MappingData);
	}

	MappingData->Modify();
	FMaterialBatchBuildPlanBuilder::PopulateMappingDataAsset(Plan, *MappingData);
	MappingData->MarkPackageDirty();
	Package->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not save mapping data asset: %s"), *ObjectPath);
		return false;
	}

	OutObjectPath = ObjectPath;
	return true;
}

bool SaveTextureArrayAsset(const FMaterialBatchBuildTextureArrayPayload& Payload, FString& OutObjectPath, FString& OutFailureReason)
{
	if (Payload.PackagePath.IsEmpty())
	{
		OutFailureReason = TEXT("empty package path");
		return false;
	}
	if (Payload.SourceTexturePaths.IsEmpty())
	{
		OutFailureReason = TEXT("no source texture slices");
		return false;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(Payload.PackagePath);
	if (AssetName.IsEmpty())
	{
		OutFailureReason = FString::Printf(TEXT("could not derive asset name from package `%s`"), *Payload.PackagePath);
		return false;
	}

	const FString ObjectPath = BuildObjectPathFromPackagePath(Payload.PackagePath);
	UTexture2DArray* TextureArray = Cast<UTexture2DArray>(
		StaticLoadObject(UTexture2DArray::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = TextureArray ? TextureArray->GetOutermost() : CreatePackage(*Payload.PackagePath);
	if (!Package)
	{
		OutFailureReason = FString::Printf(TEXT("could not create package `%s`"), *Payload.PackagePath);
		return false;
	}

	if (!TextureArray)
	{
		TextureArray = NewObject<UTexture2DArray>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(TextureArray);
	}

	TextureArray->Modify();
	TextureArray->SourceTextures.Reset();
	for (const FString& SourceTexturePath : Payload.SourceTexturePaths)
	{
		UTexture2D* SourceTexture = Cast<UTexture2D>(
			StaticLoadObject(UTexture2D::StaticClass(), nullptr, *SourceTexturePath, nullptr, LOAD_NoWarn));
		if (!SourceTexture)
		{
			OutFailureReason = FString::Printf(TEXT("could not load source texture `%s`"), *SourceTexturePath);
			return false;
		}
		TextureArray->SourceTextures.Add(SourceTexture);
	}

	if (!TextureArray->UpdateSourceFromSourceTextures(true))
	{
		OutFailureReason = FString::Printf(TEXT("source textures are not compatible for `%s`"), *Payload.ChannelName);
		return false;
	}

	TextureArray->AddressX = TA_Wrap;
	TextureArray->AddressY = TA_Wrap;
	TextureArray->AddressZ = TA_Clamp;
	TextureArray->MarkPackageDirty();
	Package->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true))
	{
		OutFailureReason = FString::Printf(TEXT("could not save `%s`"), *ObjectPath);
		return false;
	}

	OutObjectPath = ObjectPath;
	return true;
}

bool SaveTextureArrayAssets(
	const FMaterialBatchBuildPlan& Plan,
	TArray<FString>& OutReportLines,
	int32& OutSavedCount,
	int32& OutSkippedCount)
{
	OutSavedCount = 0;
	OutSkippedCount = 0;
	const TArray<FMaterialBatchBuildTextureArrayPayload> Payloads =
		FMaterialBatchBuildPlanBuilder::BuildTextureArrayPayloads(Plan);

	OutReportLines.Add(TEXT(""));
	OutReportLines.Add(TEXT("## Texture2DArray Assets"));
	OutReportLines.Add(TEXT(""));
	if (Payloads.IsEmpty())
	{
		OutReportLines.Add(TEXT("- Skipped: no eligible Texture2D slices were found in the current build plan."));
		return true;
	}

	bool bAllSaved = true;
	for (const FMaterialBatchBuildTextureArrayPayload& Payload : Payloads)
	{
		FString SavedObjectPath;
		FString FailureReason;
		const bool bSaved = SaveTextureArrayAsset(Payload, SavedObjectPath, FailureReason);
		if (bSaved)
		{
			++OutSavedCount;
			OutReportLines.Add(FString::Printf(
				TEXT("- Saved %s: `%s` (%d slices, %dx%d planned source size)"),
				*Payload.ChannelName,
				*SavedObjectPath,
				Payload.SourceTexturePaths.Num(),
				Payload.Width,
				Payload.Height));
		}
		else
		{
			++OutSkippedCount;
			bAllSaved = false;
			OutReportLines.Add(FString::Printf(
				TEXT("- Failed %s: %s"),
				*Payload.ChannelName,
				FailureReason.IsEmpty() ? TEXT("unknown error") : *FailureReason));
		}
	}

	return bAllSaved;
}

bool CopyTextureSourceIntoAtlasCell(
	const FMaterialBatchBuildVTAtlasPayload& Payload,
	const FMaterialBatchBuildVTAtlasEntry& Entry,
	TArray<uint8>& AtlasPixels,
	FString& OutFailureReason)
{
	if (Payload.Columns <= 0 || Payload.Rows <= 0 || Payload.Width <= 0 || Payload.Height <= 0)
	{
		OutFailureReason = TEXT("invalid atlas layout");
		return false;
	}

	UTexture2D* SourceTexture = Cast<UTexture2D>(
		StaticLoadObject(UTexture2D::StaticClass(), nullptr, *Entry.TexturePath, nullptr, LOAD_NoWarn));
	if (!SourceTexture)
	{
		OutFailureReason = FString::Printf(TEXT("could not load source texture `%s`"), *Entry.TexturePath);
		return false;
	}
	if (SourceTexture->Source.GetFormat() != ETextureSourceFormat::TSF_BGRA8)
	{
		OutFailureReason = FString::Printf(
			TEXT("source texture `%s` has unsupported source format `%d`; first VT atlas apply only supports TSF_BGRA8"),
			*Entry.TexturePath,
			static_cast<int32>(SourceTexture->Source.GetFormat()));
		return false;
	}

	const int32 SourceWidth = SourceTexture->Source.GetSizeX();
	const int32 SourceHeight = SourceTexture->Source.GetSizeY();
	if (SourceWidth <= 0 || SourceHeight <= 0)
	{
		OutFailureReason = FString::Printf(TEXT("source texture `%s` has invalid source size"), *Entry.TexturePath);
		return false;
	}

	TArray64<uint8> SourceMipData;
	if (!SourceTexture->Source.GetMipData(SourceMipData, 0))
	{
		OutFailureReason = FString::Printf(TEXT("could not read source mip data from `%s`"), *Entry.TexturePath);
		return false;
	}

	const int32 BytesPerPixel = 4;
	const int32 CellWidth = Payload.Width / Payload.Columns;
	const int32 CellHeight = Payload.Height / Payload.Rows;
	const int32 ColumnIndex = Entry.AtlasEntryIndex % Payload.Columns;
	const int32 RowIndex = Entry.AtlasEntryIndex / Payload.Columns;
	const int32 DestBaseX = ColumnIndex * CellWidth;
	const int32 DestBaseY = RowIndex * CellHeight;
	const int32 CopyWidth = FMath::Min(SourceWidth, CellWidth);
	const int32 CopyHeight = FMath::Min(SourceHeight, CellHeight);

	const int64 ExpectedSourceBytes = static_cast<int64>(SourceWidth) * static_cast<int64>(SourceHeight) * BytesPerPixel;
	if (SourceMipData.Num() < ExpectedSourceBytes)
	{
		OutFailureReason = FString::Printf(TEXT("source texture `%s` mip data is smaller than expected"), *Entry.TexturePath);
		return false;
	}

	for (int32 Y = 0; Y < CopyHeight; ++Y)
	{
		const int64 SourceOffset = static_cast<int64>(Y) * static_cast<int64>(SourceWidth) * BytesPerPixel;
		const int64 DestOffset =
			(static_cast<int64>(DestBaseY + Y) * static_cast<int64>(Payload.Width) + static_cast<int64>(DestBaseX)) * BytesPerPixel;
		FMemory::Memcpy(
			AtlasPixels.GetData() + DestOffset,
			SourceMipData.GetData() + SourceOffset,
			static_cast<SIZE_T>(CopyWidth * BytesPerPixel));
	}

	return true;
}

bool SaveVTAtlasAsset(const FMaterialBatchBuildPlan& Plan, FString& OutObjectPath, FString& OutFailureReason)
{
	const FMaterialBatchBuildVTAtlasPayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildVTAtlasPayload(Plan);
	if (Payload.PackagePath.IsEmpty())
	{
		OutFailureReason = TEXT("empty package path");
		return false;
	}
	if (Payload.Entries.IsEmpty())
	{
		OutFailureReason = TEXT("no eligible VT atlas entries");
		return false;
	}
	if (Payload.Width <= 0 || Payload.Height <= 0)
	{
		OutFailureReason = TEXT("invalid atlas dimensions");
		return false;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(Payload.PackagePath);
	if (AssetName.IsEmpty())
	{
		OutFailureReason = FString::Printf(TEXT("could not derive asset name from package `%s`"), *Payload.PackagePath);
		return false;
	}

	TArray<uint8> AtlasPixels;
	AtlasPixels.SetNumZeroed(Payload.Width * Payload.Height * 4);
	for (const FMaterialBatchBuildVTAtlasEntry& Entry : Payload.Entries)
	{
		if (!CopyTextureSourceIntoAtlasCell(Payload, Entry, AtlasPixels, OutFailureReason))
		{
			return false;
		}
	}

	const FString ObjectPath = BuildObjectPathFromPackagePath(Payload.PackagePath);
	UTexture2D* AtlasTexture = Cast<UTexture2D>(
		StaticLoadObject(UTexture2D::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = AtlasTexture ? AtlasTexture->GetOutermost() : CreatePackage(*Payload.PackagePath);
	if (!Package)
	{
		OutFailureReason = FString::Printf(TEXT("could not create package `%s`"), *Payload.PackagePath);
		return false;
	}

	if (!AtlasTexture)
	{
		AtlasTexture = NewObject<UTexture2D>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(AtlasTexture);
	}

	AtlasTexture->Modify();
	AtlasTexture->Source.Init(
		Payload.Width,
		Payload.Height,
		1,
		1,
		ETextureSourceFormat::TSF_BGRA8,
		AtlasPixels.GetData());
	AtlasTexture->SRGB = false;
	AtlasTexture->CompressionSettings = TC_Default;
	AtlasTexture->MipGenSettings = TMGS_SimpleAverage;
	AtlasTexture->NeverStream = false;
	AtlasTexture->VirtualTextureStreaming = true;
	AtlasTexture->AddressX = TA_Clamp;
	AtlasTexture->AddressY = TA_Clamp;
	AtlasTexture->UpdateResource();
	AtlasTexture->MarkPackageDirty();
	Package->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true))
	{
		OutFailureReason = FString::Printf(TEXT("could not save `%s`"), *ObjectPath);
		return false;
	}

	OutObjectPath = ObjectPath;
	return true;
}

bool SavePropertyTextureAsset(const FMaterialBatchBuildPlan& Plan, FString& OutObjectPath)
{
	const FMaterialBatchBuildPropertyTexturePayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildPropertyTexturePayload(Plan);
	if (Payload.Width <= 0 || Payload.Height <= 0 || Payload.Pixels.Num() != Payload.Width * Payload.Height)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild property texture payload is invalid: %dx%d, %d pixels."),
			Payload.Width,
			Payload.Height,
			Payload.Pixels.Num());
		return false;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(Plan.PropertyTexturePackage);
	if (AssetName.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not derive asset name from property texture package: %s"), *Plan.PropertyTexturePackage);
		return false;
	}

	const FString ObjectPath = BuildObjectPathFromPackagePath(Plan.PropertyTexturePackage);
	UTexture2D* PropertyTexture = Cast<UTexture2D>(
		StaticLoadObject(UTexture2D::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = PropertyTexture ? PropertyTexture->GetOutermost() : CreatePackage(*Plan.PropertyTexturePackage);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not create property texture package: %s"), *Plan.PropertyTexturePackage);
		return false;
	}

	if (!PropertyTexture)
	{
		PropertyTexture = NewObject<UTexture2D>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(PropertyTexture);
	}

	PropertyTexture->Modify();
	const uint8* SourceData = reinterpret_cast<const uint8*>(Payload.Pixels.GetData());
	PropertyTexture->Source.Init(Payload.Width, Payload.Height, 1, 1, Payload.SourceFormat, SourceData);
	PropertyTexture->SRGB = Payload.bSRGB;
	PropertyTexture->CompressionSettings = TC_HDR;
	PropertyTexture->MipGenSettings = TMGS_NoMipmaps;
	PropertyTexture->NeverStream = true;
	PropertyTexture->Filter = TF_Nearest;
	PropertyTexture->AddressX = TA_Clamp;
	PropertyTexture->AddressY = TA_Clamp;
	PropertyTexture->UpdateResource();
	PropertyTexture->MarkPackageDirty();
	Package->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not save property texture asset: %s"), *ObjectPath);
		return false;
	}

	OutObjectPath = ObjectPath;
	return true;
}

int32 ResolveMaterialSlotIndex(const UStaticMesh& StaticMesh, const FMeshDescription& SourceDescription, FPolygonGroupID PolygonGroupID)
{
	FStaticMeshConstAttributes Attributes(SourceDescription);
	const FName ImportedMaterialSlotName = Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupID];
	if (ImportedMaterialSlotName != NAME_None)
	{
		const int32 DirectMaterialIndex = StaticMesh.GetMaterialIndex(ImportedMaterialSlotName);
		if (DirectMaterialIndex != INDEX_NONE)
		{
			return DirectMaterialIndex;
		}

		const TArray<FStaticMaterial>& StaticMaterials = StaticMesh.GetStaticMaterials();
		for (int32 MaterialIndex = 0; MaterialIndex < StaticMaterials.Num(); ++MaterialIndex)
		{
			const FStaticMaterial& StaticMaterial = StaticMaterials[MaterialIndex];
			if (StaticMaterial.MaterialSlotName == ImportedMaterialSlotName ||
				StaticMaterial.ImportedMaterialSlotName == ImportedMaterialSlotName)
			{
				return MaterialIndex;
			}
		}
	}

	return PolygonGroupID.GetValue();
}

int32 ResolveBatchMaterialIndexForPolygonGroup(
	const FMaterialBatchBuildProxyMeshSourcePayload& SourcePayload,
	const UStaticMesh& StaticMesh,
	const FMeshDescription& SourceDescription,
	FPolygonGroupID PolygonGroupID)
{
	const int32 MaterialSlotIndex = ResolveMaterialSlotIndex(StaticMesh, SourceDescription, PolygonGroupID);
	if (SourcePayload.MaterialSlotToBatchMaterialIndex.IsValidIndex(MaterialSlotIndex))
	{
		return SourcePayload.MaterialSlotToBatchMaterialIndex[MaterialSlotIndex];
	}
	if (!SourcePayload.MaterialSlotToBatchMaterialIndex.IsEmpty())
	{
		return SourcePayload.MaterialSlotToBatchMaterialIndex[0];
	}
	return INDEX_NONE;
}

bool EnsureMaterialBatchUvChannel(FMeshDescription& MeshDescription)
{
	FStaticMeshAttributes Attributes(MeshDescription);
	Attributes.Register(true);
	TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	while (VertexInstanceUVs.GetNumChannels() < 8)
	{
		if (!FStaticMeshOperations::AddUVChannel(MeshDescription))
		{
			return false;
		}
	}
	return true;
}

bool WriteMaterialBatchIndicesToUv7(
	FMeshDescription& SourceDescription,
	const FMaterialBatchBuildProxyMeshSourcePayload& SourcePayload,
	const UStaticMesh& StaticMesh)
{
	if (!EnsureMaterialBatchUvChannel(SourceDescription))
	{
		return false;
	}

	FStaticMeshAttributes Attributes(SourceDescription);
	TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
	TArray<FVertexInstanceID> PolygonVertexInstanceIDs;
	PolygonVertexInstanceIDs.Reserve(8);

	for (const FPolygonID PolygonID : SourceDescription.Polygons().GetElementIDs())
	{
		const FPolygonGroupID PolygonGroupID = SourceDescription.GetPolygonPolygonGroup(PolygonID);
		const int32 BatchMaterialIndex = ResolveBatchMaterialIndexForPolygonGroup(
			SourcePayload,
			StaticMesh,
			SourceDescription,
			PolygonGroupID);
		if (BatchMaterialIndex == INDEX_NONE)
		{
			return false;
		}

		SourceDescription.GetPolygonVertexInstances(PolygonID, PolygonVertexInstanceIDs);
		for (const FVertexInstanceID VertexInstanceID : PolygonVertexInstanceIDs)
		{
			VertexInstanceUVs.Set(VertexInstanceID, 7, FVector2f(static_cast<float>(BatchMaterialIndex), 0.0f));
		}
	}

	return true;
}

bool SaveProxyMeshAsset(const FMaterialBatchBuildPlan& Plan, FString& OutObjectPath, FString& OutFailureReason)
{
	const FMaterialBatchBuildProxyMeshPayload Payload = FMaterialBatchBuildPlanBuilder::BuildProxyMeshPayload(Plan);
	if (Payload.PackagePath.IsEmpty())
	{
		OutFailureReason = TEXT("empty package path");
		return false;
	}
	if (Payload.Sources.IsEmpty())
	{
		OutFailureReason = TEXT("no candidate geometry sources");
		return false;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(Payload.PackagePath);
	if (AssetName.IsEmpty())
	{
		OutFailureReason = FString::Printf(TEXT("could not derive asset name from package `%s`"), *Payload.PackagePath);
		return false;
	}

	FMeshDescription MergedMeshDescription;
	FStaticMeshAttributes MergedAttributes(MergedMeshDescription);
	MergedAttributes.Register();
	const FPolygonGroupID BatchPolygonGroupID = MergedMeshDescription.CreatePolygonGroup();
	MergedAttributes.GetPolygonGroupMaterialSlotNames()[BatchPolygonGroupID] = FName(TEXT("MaterialBatch"));

	for (const FMaterialBatchBuildProxyMeshSourcePayload& SourcePayload : Payload.Sources)
	{
		UStaticMesh* SourceStaticMesh = Cast<UStaticMesh>(
			StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *SourcePayload.StaticMeshPath, nullptr, LOAD_NoWarn));
		if (!SourceStaticMesh)
		{
			OutFailureReason = FString::Printf(TEXT("could not load static mesh `%s`"), *SourcePayload.StaticMeshPath);
			return false;
		}

		FMeshDescription SourceDescription;
		if (!SourceStaticMesh->CloneMeshDescription(0, SourceDescription) || SourceDescription.IsEmpty())
		{
			OutFailureReason = FString::Printf(TEXT("could not clone LOD0 mesh description from `%s`"), *SourcePayload.StaticMeshPath);
			return false;
		}

		if (!WriteMaterialBatchIndicesToUv7(SourceDescription, SourcePayload, *SourceStaticMesh))
		{
			OutFailureReason = FString::Printf(TEXT("could not write UV7.x batch material indices for `%s`"), *SourcePayload.StaticMeshPath);
			return false;
		}

		FStaticMeshOperations::FAppendSettings AppendSettings;
		if (SourcePayload.bHasWorldTransform)
		{
			AppendSettings.MeshTransform = FTransform(
				SourcePayload.WorldRotation,
				SourcePayload.WorldLocation,
				SourcePayload.WorldScale);
		}
		AppendSettings.PolygonGroupsDelegate.BindLambda(
			[BatchPolygonGroupID](const FMeshDescription& SourceMesh, FMeshDescription& TargetMesh, PolygonGroupMap& RemapPolygonGroup)
			{
				for (const FPolygonGroupID SourcePolygonGroupID : SourceMesh.PolygonGroups().GetElementIDs())
				{
					RemapPolygonGroup.Add(SourcePolygonGroupID, BatchPolygonGroupID);
				}
			});

		FStaticMeshOperations::AppendMeshDescription(SourceDescription, MergedMeshDescription, AppendSettings);
	}

	if (MergedMeshDescription.Polygons().Num() <= 0)
	{
		OutFailureReason = TEXT("merged mesh contains no polygons");
		return false;
	}

	const FString ObjectPath = BuildObjectPathFromPackagePath(Payload.PackagePath);
	UStaticMesh* ProxyMesh = Cast<UStaticMesh>(
		StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = ProxyMesh ? ProxyMesh->GetOutermost() : CreatePackage(*Payload.PackagePath);
	if (!Package)
	{
		OutFailureReason = FString::Printf(TEXT("could not create package `%s`"), *Payload.PackagePath);
		return false;
	}

	if (!ProxyMesh)
	{
		ProxyMesh = NewObject<UStaticMesh>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(ProxyMesh);
	}

	ProxyMesh->Modify();
	UMaterialInterface* DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
	ProxyMesh->SetStaticMaterials({
		FStaticMaterial(DefaultMaterial, FName(TEXT("MaterialBatch")), FName(TEXT("MaterialBatch")))
	});

	UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
	BuildParams.bBuildSimpleCollision = false;
	BuildParams.bFastBuild = false;
	BuildParams.bAllowCpuAccess = false;
	UStaticMesh::FBuildMeshDescriptionsLODParams LodParams;
	LodParams.bUseFullPrecisionUVs = true;
	BuildParams.PerLODOverrides.Add(LodParams);

	const TArray<const FMeshDescription*> MeshDescriptions = { &MergedMeshDescription };
	if (!ProxyMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams))
	{
		OutFailureReason = FString::Printf(TEXT("could not build proxy static mesh `%s`"), *ObjectPath);
		return false;
	}

	ProxyMesh->MarkPackageDirty();
	Package->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);
	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true))
	{
		OutFailureReason = FString::Printf(TEXT("could not save `%s`"), *ObjectPath);
		return false;
	}

	OutObjectPath = ObjectPath;
	return true;
}

bool SaveBatchMaterialInstanceAsset(
	const FMaterialBatchBuildPlan& Plan,
	TArray<FString>& OutReportLines,
	FString& OutObjectPath,
	FString& OutFailureReason)
{
	const FMaterialBatchBuildBatchMaterialPayload Payload =
		FMaterialBatchBuildPlanBuilder::BuildBatchMaterialPayload(Plan);
	if (Payload.PackagePath.IsEmpty())
	{
		OutFailureReason = TEXT("empty package path");
		return false;
	}

	const FString AssetName = FPackageName::GetLongPackageAssetName(Payload.PackagePath);
	if (AssetName.IsEmpty())
	{
		OutFailureReason = FString::Printf(TEXT("could not derive asset name from package `%s`"), *Payload.PackagePath);
		return false;
	}

	UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(
		StaticLoadObject(UMaterialInterface::StaticClass(), nullptr, *Payload.ParentMaterialPath, nullptr, LOAD_NoWarn));
	if (!ParentMaterial)
	{
		OutFailureReason = FString::Printf(TEXT("could not load parent material `%s`"), *Payload.ParentMaterialPath);
		return false;
	}

	const FString ObjectPath = BuildObjectPathFromPackagePath(Payload.PackagePath);
	UMaterialInstanceConstant* MaterialInstance = Cast<UMaterialInstanceConstant>(
		StaticLoadObject(UMaterialInstanceConstant::StaticClass(), nullptr, *ObjectPath, nullptr, LOAD_NoWarn));
	UPackage* Package = MaterialInstance ? MaterialInstance->GetOutermost() : CreatePackage(*Payload.PackagePath);
	if (!Package)
	{
		OutFailureReason = FString::Printf(TEXT("could not create package `%s`"), *Payload.PackagePath);
		return false;
	}

	if (!MaterialInstance)
	{
		MaterialInstance = NewObject<UMaterialInstanceConstant>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(MaterialInstance);
	}

	MaterialInstance->Modify();
	MaterialInstance->SetParentEditorOnly(ParentMaterial);
	OutReportLines.Add(FString::Printf(TEXT("- Parent: `%s`"), *Payload.ParentMaterialPath));

	for (const FMaterialBatchBuildBatchMaterialTextureBinding& Binding : Payload.TextureBindings)
	{
		const FString TextureObjectPath = BuildObjectPathFromPackagePath(Binding.TexturePackagePath);
		UTexture* Texture = Cast<UTexture>(
			StaticLoadObject(UTexture::StaticClass(), nullptr, *TextureObjectPath, nullptr, LOAD_NoWarn));
		if (!Texture)
		{
			OutFailureReason = FString::Printf(TEXT("could not load generated texture `%s`"), *TextureObjectPath);
			return false;
		}

		MaterialInstance->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(FName(*Binding.ParameterName)), Texture);
		OutReportLines.Add(FString::Printf(
			TEXT("- Bound `%s`: `%s`"),
			*Binding.ParameterName,
			*TextureObjectPath));
	}

	for (const FMaterialBatchBuildBatchMaterialScalarBinding& Binding : Payload.ScalarBindings)
	{
		MaterialInstance->SetScalarParameterValueEditorOnly(
			FMaterialParameterInfo(FName(*Binding.ParameterName)),
			Binding.Value);
		OutReportLines.Add(FString::Printf(
			TEXT("- Bound `%s`: %.3f"),
			*Binding.ParameterName,
			Binding.Value));
	}

	MaterialInstance->PostEditChange();
	MaterialInstance->MarkPackageDirty();
	Package->MarkPackageDirty();

	TArray<UPackage*> PackagesToSave;
	PackagesToSave.Add(Package);

	const FString ProxyObjectPath = BuildObjectPathFromPackagePath(Plan.ProxyMeshPackage);
	if (UStaticMesh* ProxyMesh = Cast<UStaticMesh>(
		StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *ProxyObjectPath, nullptr, LOAD_NoWarn)))
	{
		ProxyMesh->Modify();
		ProxyMesh->SetMaterial(0, MaterialInstance);
		ProxyMesh->MarkPackageDirty();
		PackagesToSave.AddUnique(ProxyMesh->GetOutermost());
		OutReportLines.Add(FString::Printf(TEXT("- Bound proxy mesh slot 0: `%s`"), *ProxyObjectPath));
	}
	else
	{
		OutReportLines.Add(FString::Printf(TEXT("- Proxy mesh bind skipped: `%s` was not found."), *ProxyObjectPath));
	}

	if (!UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true))
	{
		OutFailureReason = FString::Printf(TEXT("could not save `%s`"), *ObjectPath);
		return false;
	}

	OutObjectPath = ObjectPath;
	return true;
}

void AddStaticMeshMaterialSlots(const UStaticMesh* StaticMesh, FMaterialBatchBuildPlannedEntry& Entry)
{
	if (!StaticMesh)
	{
		return;
	}

	const TArray<FStaticMaterial>& StaticMaterials = StaticMesh->GetStaticMaterials();
	Entry.MaterialSlotNames.Reserve(StaticMaterials.Num());
	Entry.MaterialPaths.Reserve(StaticMaterials.Num());
	for (const FStaticMaterial& StaticMaterial : StaticMaterials)
	{
		Entry.MaterialSlotNames.Add(StaticMaterial.MaterialSlotName.ToString());
		Entry.MaterialPaths.Add(GetMaterialPath(StaticMaterial.MaterialInterface));
	}
}

void AddComponentMaterialSlots(
	const UStaticMeshComponent* Component,
	const UStaticMesh* StaticMesh,
	FMaterialBatchBuildPlannedEntry& Entry)
{
	if (!StaticMesh)
	{
		return;
	}

	const TArray<FStaticMaterial>& StaticMaterials = StaticMesh->GetStaticMaterials();
	Entry.MaterialSlotNames.Reserve(StaticMaterials.Num());
	Entry.MaterialPaths.Reserve(StaticMaterials.Num());
	for (int32 MaterialIndex = 0; MaterialIndex < StaticMaterials.Num(); ++MaterialIndex)
	{
		const FStaticMaterial& StaticMaterial = StaticMaterials[MaterialIndex];
		const UMaterialInterface* Material = Component ? Component->GetMaterial(MaterialIndex) : StaticMaterial.MaterialInterface.Get();
		Entry.MaterialSlotNames.Add(StaticMaterial.MaterialSlotName.ToString());
		Entry.MaterialPaths.Add(GetMaterialPath(Material ? Material : StaticMaterial.MaterialInterface.Get()));
	}
}

TArray<FString> GetEnvBatchTags(const TArray<FName>& Tags)
{
	TArray<FString> Result;
	for (const FName& Tag : Tags)
	{
		const FString TagString = Tag.ToString();
		if (TagString.StartsWith(TEXT("EnvBatch.")))
		{
			Result.Add(TagString);
		}
	}
	Result.Sort();
	return Result;
}

FString NormalizeLayerBackend(const FString& LayerBackend)
{
	return LayerBackend.Equals(TEXT("DataLayer"), ESearchCase::IgnoreCase)
		? TEXT("DataLayer")
		: TEXT("StreamingLevel");
}

FString GetLevelPackageName(const ULevel* Level)
{
	return Level && Level->GetOutermost()
		? Level->GetOutermost()->GetName()
		: FString();
}

FString GetLevelShortName(const FString& LevelPackageName)
{
	return LevelPackageName.IsEmpty()
		? FString()
		: FPackageName::GetShortName(LevelPackageName);
}

TArray<FString> GetActorStreamingLayerNames(const AActor& Actor)
{
	TArray<FString> Result;
	const FString LevelPackageName = GetLevelPackageName(Actor.GetLevel());
	const FString LevelShortName = GetLevelShortName(LevelPackageName);
	if (!LevelShortName.IsEmpty())
	{
		Result.AddUnique(LevelShortName);
	}
	if (!LevelPackageName.IsEmpty() && LevelPackageName != LevelShortName)
	{
		Result.AddUnique(LevelPackageName);
	}
	Result.Sort();
	return Result;
}

bool HasEnvBatchTagPrefix(const TArray<FString>& Tags, const TCHAR* Prefix)
{
	for (const FString& Tag : Tags)
	{
		if (Tag.StartsWith(Prefix))
		{
			return true;
		}
	}
	return false;
}

void UpdateTagDiagnosticsForActor(
	const AActor& Actor,
	const TArray<FString>& EnvBatchTags,
	const FMaterialBatchBuildPlanOptions& Options,
	FMaterialBatchBuildTagDiagnostics& Diagnostics)
{
	if (EnvBatchTags.IsEmpty())
	{
		return;
	}

	++Diagnostics.ActorCount;
	const bool bHasSource = HasEnvBatchTagPrefix(EnvBatchTags, TEXT("EnvBatch.Source."));
	const bool bHasProxy = HasEnvBatchTagPrefix(EnvBatchTags, TEXT("EnvBatch.Proxy."));
	const bool bHasBaked = HasEnvBatchTagPrefix(EnvBatchTags, TEXT("EnvBatch.Baked."));
	const bool bHasExclude = EnvBatchTags.Contains(TEXT("EnvBatch.Exclude"));
	const bool bHasBakeStaticDecal = HasEnvBatchTagPrefix(EnvBatchTags, TEXT("EnvBatch.BakeStaticDecal."));
	const bool bHasRuntimeDecal = HasEnvBatchTagPrefix(EnvBatchTags, TEXT("EnvBatch.RuntimeDecal"));
	const bool bHasGameplayIndicator = HasEnvBatchTagPrefix(EnvBatchTags, TEXT("EnvBatch.GameplayIndicator"));

	if (bHasSource)
	{
		++Diagnostics.SourceActorCount;
	}
	if (bHasProxy)
	{
		++Diagnostics.ProxyActorCount;
	}
	if (bHasBaked)
	{
		++Diagnostics.BakedActorCount;
	}
	if (bHasExclude)
	{
		++Diagnostics.ExcludeActorCount;
	}
	if (bHasBakeStaticDecal)
	{
		++Diagnostics.BakeStaticDecalActorCount;
	}
	if (bHasRuntimeDecal)
	{
		++Diagnostics.RuntimeDecalActorCount;
	}
	if (bHasGameplayIndicator)
	{
		++Diagnostics.GameplayIndicatorActorCount;
	}

	if (Options.bValidateSourceProxyExclusivity && bHasSource && (bHasProxy || bHasBaked))
	{
		++Diagnostics.SourceProxyConflictActorCount;
		Diagnostics.Warnings.Add(FString::Printf(
			TEXT("%s has mutually exclusive EnvBatch tags in group `%s`: %s"),
			*Actor.GetActorNameOrLabel(),
			Options.SourceProxyExclusivityGroup.IsEmpty() ? *Options.ClusterName : *Options.SourceProxyExclusivityGroup,
			*FString::Join(EnvBatchTags, TEXT(", "))));
	}

	if (Options.bReportStaticDecals && bHasBakeStaticDecal && bHasRuntimeDecal)
	{
		Diagnostics.Warnings.Add(FString::Printf(
			TEXT("%s is tagged as both BakeStaticDecal and RuntimeDecal: %s"),
			*Actor.GetActorNameOrLabel(),
			*FString::Join(EnvBatchTags, TEXT(", "))));
	}
}

FMaterialBatchBuildCandidateSummary ScanAssetCandidates(
	const FString& RootPath,
	int32 MaxAssets,
	TArray<FMaterialBatchBuildPlannedEntry>& OutEntries)
{
	FMaterialBatchBuildCandidateSummary Summary;
	Summary.SourceKind = TEXT("StaticMeshAsset");

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	AssetRegistry.SearchAllAssets(true);

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*RootPath));
	Filter.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> MeshAssets;
	AssetRegistry.GetAssets(Filter, MeshAssets);
	MeshAssets.Sort([](const FAssetData& Left, const FAssetData& Right)
	{
		return Left.PackageName.LexicalLess(Right.PackageName);
	});

	Summary.SourceFoundCount = MeshAssets.Num();
	for (const FAssetData& AssetData : MeshAssets)
	{
		if (MaxAssets > 0 && Summary.SourceInspectedCount >= MaxAssets)
		{
			break;
		}
		++Summary.SourceInspectedCount;

		UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
		FMaterialBatchComponentScanInput Input;
		Input.bIsStaticMeshComponent = StaticMesh != nullptr;
		Input.bHasStaticMobility = true;
		if (StaticMesh)
		{
			Input.MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
			Input.LodCount = StaticMesh->GetNumLODs();
		}

		const FMaterialBatchCandidateDecision Decision = FMaterialBatchCandidateRules::ClassifyComponent(Input);
		FMaterialBatchBuildPlannedEntry Entry;
		Entry.SourceKind = Summary.SourceKind;
		Entry.SourcePath = RootPath;
		Entry.AssetPath = AssetData.GetObjectPathString();
		Entry.MaterialSlotCount = Input.MaterialSlotCount;
		AddStaticMeshMaterialSlots(StaticMesh, Entry);
		Entry.LodCount = Input.LodCount;
		Entry.bCandidate = Decision.bEligible;
		Entry.RejectReason = FMaterialBatchCandidateRules::RejectReasonToString(Decision.Reason);
		OutEntries.Add(Entry);

		if (Decision.bEligible)
		{
			++Summary.BatchCandidateCount;
		}
		else
		{
			++Summary.RejectedCount;
		}
	}

	return Summary;
}

FMaterialBatchBuildCandidateSummary ScanMapCandidates(
	const FString& MapPath,
	int32 MaxActors,
	const FString& RequireTagPrefix,
	const FMaterialBatchBuildPlanOptions& Options,
	FMaterialBatchBuildTagDiagnostics& OutTagDiagnostics,
	TArray<FMaterialBatchBuildPlannedEntry>& OutEntries)
{
	FMaterialBatchBuildCandidateSummary Summary;
	Summary.SourceKind = TEXT("MapComponent");

	const FString MapFilename = FMaterialBatchAuditHelpers::ResolveMapFilename(MapPath);
	UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);
	if (!World)
	{
		return Summary;
	}

	// Force-load every streaming sublevel so we can see actors and resolve their static meshes.
	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (StreamingLevel)
		{
			StreamingLevel->SetShouldBeLoaded(true);
			StreamingLevel->SetShouldBeVisible(true);
		}
	}
	World->FlushLevelStreaming(EFlushLevelStreamingType::Full);

	int32 ActorsInspected = 0;
	for (ULevel* Level : World->GetLevels())
	{
		if (!Level)
		{
			continue;
		}

		for (AActor* Actor : Level->Actors)
		{
			if (!Actor)
			{
				continue;
			}
			if (MaxActors > 0 && ActorsInspected >= MaxActors)
			{
				break;
			}
			++ActorsInspected;

			const TArray<FString> ActorEnvBatchTags = GetEnvBatchTags(Actor->Tags);
			const FString ActorLevelPackageName = GetLevelPackageName(Actor->GetLevel());
			const FString ActorStreamingLevelName = GetLevelShortName(ActorLevelPackageName);
			const TArray<FString> ActorLayerNames = GetActorStreamingLayerNames(*Actor);
			UpdateTagDiagnosticsForActor(*Actor, ActorEnvBatchTags, Options, OutTagDiagnostics);

			// Optional tag filter: only consider actors carrying the requested EnvBatch.* tag prefix.
			if (!RequireTagPrefix.IsEmpty())
			{
				bool bHasRequiredTag = false;
				for (const FName& Tag : Actor->Tags)
				{
					if (Tag.ToString().StartsWith(RequireTagPrefix))
					{
						bHasRequiredTag = true;
						break;
					}
				}
				if (!bHasRequiredTag)
				{
					continue;
				}
			}

			// Make sure deferred PostLoad has run on the actor before we inspect components.
			if (Actor->HasAnyFlags(RF_NeedPostLoad))
			{
				Actor->ConditionalPostLoad();
			}

			TArray<UStaticMeshComponent*> StaticMeshComponents;
			Actor->GetComponents<UStaticMeshComponent>(StaticMeshComponents);
			for (UStaticMeshComponent* Component : StaticMeshComponents)
			{
				++Summary.SourceFoundCount;
				if (Component && Component->HasAnyFlags(RF_NeedPostLoad))
				{
					Component->ConditionalPostLoad();
				}
				UStaticMesh* StaticMesh = Component ? Component->GetStaticMesh() : nullptr;

				TArray<FName> CombinedTags = Actor->Tags;
				if (Component)
				{
					CombinedTags.Append(Component->ComponentTags);
				}
				const TArray<FString> CombinedEnvBatchTags = GetEnvBatchTags(CombinedTags);

				FMaterialBatchComponentScanInput Input;
				Input.bIsStaticMeshComponent = StaticMesh != nullptr;
				Input.bHasStaticMobility = Component && Component->Mobility == EComponentMobility::Static;
				if (StaticMesh)
				{
					Input.MaterialSlotCount = StaticMesh->GetStaticMaterials().Num();
					Input.LodCount = StaticMesh->GetNumLODs();
				}
				Input = FMaterialBatchCandidateRules::BuildInputFromTags(Input, CombinedTags);

				const FMaterialBatchCandidateDecision Decision = FMaterialBatchCandidateRules::ClassifyComponent(Input);
				++Summary.SourceInspectedCount;

				FMaterialBatchBuildPlannedEntry Entry;
				Entry.SourceKind = Summary.SourceKind;
				Entry.SourcePath = MapPath;
				Entry.ActorName = Actor->GetActorNameOrLabel();
				Entry.ComponentName = Component ? Component->GetName() : TEXT("(null)");
				Entry.AssetPath = StaticMesh ? StaticMesh->GetPathName() : TEXT("(no static mesh)");
				Entry.EnvBatchTags = CombinedEnvBatchTags;
				Entry.ActualLayerNames = ActorLayerNames;
				Entry.ActualStreamingLevelName = ActorStreamingLevelName;
				Entry.ActualLevelPackageName = ActorLevelPackageName;
				if (Component)
				{
					const FTransform ComponentTransform = Component->GetComponentTransform();
					Entry.bHasWorldTransform = true;
					Entry.WorldLocation = ComponentTransform.GetLocation();
					Entry.WorldRotation = ComponentTransform.Rotator();
					Entry.WorldScale = ComponentTransform.GetScale3D();
				}
				Entry.MaterialSlotCount = Input.MaterialSlotCount;
				AddComponentMaterialSlots(Component, StaticMesh, Entry);
				Entry.LodCount = Input.LodCount;
				Entry.bCandidate = Decision.bEligible;
				Entry.RejectReason = FMaterialBatchCandidateRules::RejectReasonToString(Decision.Reason);
				OutEntries.Add(Entry);

				if (Decision.bEligible)
				{
					++Summary.BatchCandidateCount;
				}
				else
				{
					++Summary.RejectedCount;
				}
			}
		}

		if (MaxActors > 0 && ActorsInspected >= MaxActors)
		{
			break;
		}
	}

	return Summary;
}
}

UMaterialBatchBuildCommandlet::UMaterialBatchBuildCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMaterialBatchBuildCommandlet::Main(const FString& Params)
{
	FMaterialBatchBuildPlanOptions Options;
	Options.RootPath = GetParamValue(Params, TEXT("Root="), TEXT("/Game/Art"));
	Options.MapPath = GetParamValue(Params, TEXT("Map="), TEXT(""));
	Options.DataLayerName = GetParamValue(Params, TEXT("DataLayer="), TEXT(""));
	Options.LayerBackend = NormalizeLayerBackend(GetParamValue(Params, TEXT("LayerBackend="), TEXT("StreamingLevel")));
	Options.ClusterName = GetParamValue(Params, TEXT("Cluster="), TEXT("Default"));
	Options.TierName = GetParamValue(Params, TEXT("Tier="), TEXT("Mid"));
	Options.TextureBackend = GetParamValue(Params, TEXT("TextureBackend="), TEXT("VTAtlas"));
	Options.SurfaceKind = GetParamValue(Params, TEXT("SurfaceKind="), TEXT("MixedStatic"));
	Options.BakePolicy = GetParamValue(Params, TEXT("BakePolicy="), TEXT("StaticBake"));
	Options.SourceProxyExclusivityGroup = GetParamValue(Params, TEXT("SourceProxyExclusivityGroup="), TEXT(""));
	Options.RulesPath = GetParamValue(Params, TEXT("Rules="), TEXT(""));
	Options.OutputRoot = GetParamValue(Params, TEXT("OutputRoot="), TEXT("/Game/Generated/MaterialBatch"));
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bApplyVTAtlasOnly = HasSwitch(Params, TEXT("ApplyVTAtlasOnly"));
	const bool bApplyMappingOnly = HasSwitch(Params, TEXT("ApplyMappingOnly"));
	const bool bApplyTextureArraysOnly = HasSwitch(Params, TEXT("ApplyTextureArraysOnly"));
	const bool bApplyPropertyTextureOnly = HasSwitch(Params, TEXT("ApplyPropertyTextureOnly"));
	const bool bApplyProxyMeshOnly = HasSwitch(Params, TEXT("ApplyProxyMeshOnly"));
	const bool bApplyBatchMaterialOnly = HasSwitch(Params, TEXT("ApplyBatchMaterialOnly"));
	Options.bReportStaticDecals = HasSwitch(Params, TEXT("ReportStaticDecals"));
	Options.bValidateSourceProxyExclusivity = HasSwitch(Params, TEXT("ValidateSourceProxyExclusivity"));
	Options.bDryRun = !(bApply ||
		bApplyVTAtlasOnly ||
		bApplyMappingOnly ||
		bApplyTextureArraysOnly ||
		bApplyPropertyTextureOnly ||
		bApplyProxyMeshOnly ||
		bApplyBatchMaterialOnly);
	const int32 MaxAssets = FMath::Max(0, GetParamInt(Params, TEXT("MaxAssets="), 500));
	const int32 MaxActors = FMath::Max(0, GetParamInt(Params, TEXT("MaxActors="), 2000));
	const bool bIncludeEngine = HasSwitch(Params, TEXT("IncludeEngine"));
	const FString RequireTagPrefix = GetParamValue(Params, TEXT("RequireTag="), TEXT(""));

	if (Options.MapPath.IsEmpty() && !Options.RootPath.StartsWith(TEXT("/Game")) && !bIncludeEngine)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild root must start with /Game unless -IncludeEngine is provided."));
		return 1;
	}
	if (!Options.MapPath.IsEmpty() && !Options.MapPath.StartsWith(TEXT("/Game")) && !bIncludeEngine)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild map must start with /Game unless -IncludeEngine is provided."));
		return 1;
	}

	FMaterialBatchBuildPlan Plan = FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(Options);
	TArray<FMaterialBatchBuildPlannedEntry> PlannedEntries;
	FMaterialBatchBuildTagDiagnostics TagDiagnostics;
	const FMaterialBatchBuildCandidateSummary Summary = Options.MapPath.IsEmpty()
		? ScanAssetCandidates(Options.RootPath, MaxAssets, PlannedEntries)
		: ScanMapCandidates(Options.MapPath, MaxActors, RequireTagPrefix, Options, TagDiagnostics, PlannedEntries);
	Plan.TagDiagnostics = TagDiagnostics;
	FMaterialBatchBuildPlanBuilder::ApplyCandidateSummary(Plan, Summary);
	FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(Plan, PlannedEntries);
	FMaterialBatchBuildPlanBuilder::ApplyTextureChannelPlans(Plan);
	TArray<FString> ReportLines = FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(Plan);
	const FString JsonManifest = FMaterialBatchBuildPlanBuilder::BuildJsonManifest(Plan);

	if (!Plan.bDryRun)
	{
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Result"));
		ReportLines.Add(TEXT(""));
		if (bApply)
		{
			ReportLines.Add(TEXT("- Failed: `-Apply` is intentionally disabled until generated proxy meshes are reviewed and map replacement is implemented."));
		}
		else if (bApplyVTAtlasOnly || bApplyMappingOnly || bApplyTextureArraysOnly || bApplyPropertyTextureOnly || bApplyProxyMeshOnly || bApplyBatchMaterialOnly)
		{
			ReportLines.Add(TEXT("- Partial apply requested: writes only the explicitly requested generated assets. Map replacement generation remains disabled."));
		}
		else
		{
			ReportLines.Add(TEXT("- Apply mode requested but no supported partial apply switch was provided."));
		}
	}

	FString SavedMappingDataObjectPath;
	bool bSavedMappingData = false;
	if (!bApply && bApplyMappingOnly)
	{
		bSavedMappingData = SaveMappingDataAsset(Plan, SavedMappingDataObjectPath);
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Mapping Data Asset"));
		ReportLines.Add(TEXT(""));
		ReportLines.Add(bSavedMappingData
			? FString::Printf(TEXT("- Saved: `%s`"), *SavedMappingDataObjectPath)
			: TEXT("- Failed to save mapping data asset."));
	}

	bool bSavedTextureArrays = false;
	int32 SavedTextureArrayCount = 0;
	int32 SkippedTextureArrayCount = 0;
	if (!bApply && bApplyTextureArraysOnly)
	{
		bSavedTextureArrays = SaveTextureArrayAssets(
			Plan,
			ReportLines,
			SavedTextureArrayCount,
			SkippedTextureArrayCount);
	}

	FString SavedVTAtlasObjectPath;
	FString VTAtlasFailureReason;
	bool bSavedVTAtlas = false;
	if (!bApply && bApplyVTAtlasOnly)
	{
		bSavedVTAtlas = SaveVTAtlasAsset(Plan, SavedVTAtlasObjectPath, VTAtlasFailureReason);
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## VT Atlas Asset"));
		ReportLines.Add(TEXT(""));
		ReportLines.Add(bSavedVTAtlas
			? FString::Printf(TEXT("- Saved: `%s`"), *SavedVTAtlasObjectPath)
			: FString::Printf(TEXT("- Failed to save VT atlas asset: %s"), VTAtlasFailureReason.IsEmpty() ? TEXT("unknown error") : *VTAtlasFailureReason));
	}

	FString SavedPropertyTextureObjectPath;
	bool bSavedPropertyTexture = false;
	if (!bApply && bApplyPropertyTextureOnly)
	{
		bSavedPropertyTexture = SavePropertyTextureAsset(Plan, SavedPropertyTextureObjectPath);
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Property Texture Asset"));
		ReportLines.Add(TEXT(""));
		ReportLines.Add(bSavedPropertyTexture
			? FString::Printf(TEXT("- Saved: `%s`"), *SavedPropertyTextureObjectPath)
			: TEXT("- Failed to save property texture asset."));
	}

	FString SavedProxyMeshObjectPath;
	FString ProxyMeshFailureReason;
	bool bSavedProxyMesh = false;
	if (!bApply && bApplyProxyMeshOnly)
	{
		bSavedProxyMesh = SaveProxyMeshAsset(Plan, SavedProxyMeshObjectPath, ProxyMeshFailureReason);
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Proxy Mesh Asset"));
		ReportLines.Add(TEXT(""));
		ReportLines.Add(bSavedProxyMesh
			? FString::Printf(TEXT("- Saved: `%s`"), *SavedProxyMeshObjectPath)
			: FString::Printf(TEXT("- Failed to save proxy mesh asset: %s"), ProxyMeshFailureReason.IsEmpty() ? TEXT("unknown error") : *ProxyMeshFailureReason));
	}

	FString SavedBatchMaterialObjectPath;
	FString BatchMaterialFailureReason;
	bool bSavedBatchMaterial = false;
	if (!bApply && bApplyBatchMaterialOnly)
	{
		ReportLines.Add(TEXT(""));
		ReportLines.Add(TEXT("## Batch Material Instance"));
		ReportLines.Add(TEXT(""));
		bSavedBatchMaterial = SaveBatchMaterialInstanceAsset(
			Plan,
			ReportLines,
			SavedBatchMaterialObjectPath,
			BatchMaterialFailureReason);
		ReportLines.Add(bSavedBatchMaterial
			? FString::Printf(TEXT("- Saved: `%s`"), *SavedBatchMaterialObjectPath)
			: FString::Printf(TEXT("- Failed to save batch material instance: %s"), BatchMaterialFailureReason.IsEmpty() ? TEXT("unknown error") : *BatchMaterialFailureReason));
	}

	FString SavedReportPath;
	FString SharedReportPath;
	const bool bSaved = DevKitEditorCommandletReports::SaveReportLines(MaterialBatchBuildReportFileName, ReportLines, SavedReportPath, SharedReportPath);
	if (!bSaved)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("MaterialBatchBuild wrote: %s"), *SavedReportPath);
	UE_LOG(LogTemp, Display, TEXT("MaterialBatchBuild wrote shared report: %s"), *SharedReportPath);

	FString SavedManifestPath;
	FString SharedManifestPath;
	const bool bSavedManifest = DevKitEditorCommandletReports::SaveReportString(
		MaterialBatchBuildManifestFileName,
		JsonManifest,
		SavedManifestPath,
		SharedManifestPath);
	if (!bSavedManifest)
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchBuild could not save manifest."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("MaterialBatchBuild wrote manifest: %s"), *SavedManifestPath);
	UE_LOG(LogTemp, Display, TEXT("MaterialBatchBuild wrote shared manifest: %s"), *SharedManifestPath);
	if (bApply)
	{
		return 1;
	}
	if (bApplyVTAtlasOnly || bApplyMappingOnly || bApplyTextureArraysOnly || bApplyPropertyTextureOnly || bApplyProxyMeshOnly || bApplyBatchMaterialOnly)
	{
		const bool bVTAtlasOk = !bApplyVTAtlasOnly || bSavedVTAtlas;
		const bool bMappingOk = !bApplyMappingOnly || bSavedMappingData;
		const bool bTextureArraysOk = !bApplyTextureArraysOnly || bSavedTextureArrays;
		const bool bPropertyTextureOk = !bApplyPropertyTextureOnly || bSavedPropertyTexture;
		const bool bProxyMeshOk = !bApplyProxyMeshOnly || bSavedProxyMesh;
		const bool bBatchMaterialOk = !bApplyBatchMaterialOnly || bSavedBatchMaterial;
		return (bVTAtlasOk && bMappingOk && bTextureArraysOk && bPropertyTextureOk && bProxyMeshOk && bBatchMaterialOk) ? 0 : 1;
	}
	return Plan.bDryRun ? 0 : 1;
}
