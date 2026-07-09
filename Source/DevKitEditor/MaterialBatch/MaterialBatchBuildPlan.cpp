#include "MaterialBatch/MaterialBatchBuildPlan.h"

#include "Dom/JsonObject.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "System/MaterialBatchMappingDataAsset.h"

#include <initializer_list>

namespace
{
FString JoinPackagePath(const FString& Left, const FString& Right)
{
	FString CleanLeft = Left;
	while (CleanLeft.EndsWith(TEXT("/")))
	{
		CleanLeft.LeftChopInline(1);
	}

	FString CleanRight = Right;
	while (CleanRight.StartsWith(TEXT("/")))
	{
		CleanRight.RightChopInline(1);
	}

	return FString::Printf(TEXT("%s/%s"), *CleanLeft, *CleanRight);
}

FString GetObjectPath(const UObject* Object)
{
	return Object ? Object->GetPathName() : FString();
}

FString GetObjectClassName(const UObject* Object)
{
	return Object && Object->GetClass() ? Object->GetClass()->GetName() : FString();
}

FIntPoint GetTexture2DDimensions(const UTexture2D* Texture)
{
	if (!Texture)
	{
		return FIntPoint(INDEX_NONE, INDEX_NONE);
	}

	int32 Width = Texture->GetSizeX();
	int32 Height = Texture->GetSizeY();
	if (Width <= 0 || Height <= 0)
	{
		Width = Texture->Source.GetSizeX();
		Height = Texture->Source.GetSizeY();
	}
	return FIntPoint(Width, Height);
}

bool ContainsAnyToken(const FString& Value, std::initializer_list<const TCHAR*> Tokens)
{
	for (const TCHAR* Token : Tokens)
	{
		if (Value.Contains(Token))
		{
			return true;
		}
	}
	return false;
}

const FString& GetDefaultBatchParentMaterialPath()
{
	static const FString Path = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas.M_Env_Baked_VTAtlas");
	return Path;
}

const FString& GetDefaultBatchPropertyTextureParameterName()
{
	static const FString Name = TEXT("_PropTexture");
	return Name;
}

struct FMaterialBatchBuildPropertyRowPlan
{
	int32 BatchMaterialIndex = INDEX_NONE;
	FString MaterialPath;
	int32 BaseColorSliceIndex = INDEX_NONE;
	int32 NormalSliceIndex = INDEX_NONE;
	int32 OrmSliceIndex = INDEX_NONE;
	int32 EmissiveSliceIndex = INDEX_NONE;
	int32 MaskSliceIndex = INDEX_NONE;
	FString LightInfoTexturePath;
	FString SourceTexturePath;
	FString NormalSourceTexturePath;
	FString OrmSourceTexturePath;
	FVector2D UVRectMin = FVector2D::ZeroVector;
	FVector2D UVRectMax = FVector2D(1.f, 1.f);
	FVector2D NormalUVRectMin = FVector2D::ZeroVector;
	FVector2D NormalUVRectMax = FVector2D(1.f, 1.f);
	FVector2D OrmUVRectMin = FVector2D::ZeroVector;
	FVector2D OrmUVRectMax = FVector2D(1.f, 1.f);
};

struct FMaterialBatchBuildTextureArraySlicePlan
{
	int32 SliceIndex = INDEX_NONE;
	FString TexturePath;
	FString TextureClass;
};

struct FMaterialBatchBuildTextureArrayPlans
{
	TArray<FMaterialBatchBuildTextureArraySlicePlan> BaseColor;
	TArray<FMaterialBatchBuildTextureArraySlicePlan> Normal;
	TArray<FMaterialBatchBuildTextureArraySlicePlan> Orm;
	TArray<FMaterialBatchBuildTextureArraySlicePlan> Emissive;
	TArray<FMaterialBatchBuildTextureArraySlicePlan> Mask;
};

struct FMaterialBatchBuildPropertyTextureColumnPlan
{
	int32 PropertyIndex = INDEX_NONE;
	FString Name;
	FString SourceField;
	FString ValueType;
	int32 DefaultIntValue = INDEX_NONE;
	FString Description;
};

struct FMaterialBatchBuildGeometryMergeSourcePlan
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
	int32 FirstBatchMaterialIndex = INDEX_NONE;
	int32 BatchMaterialIndexCount = 0;
	TArray<int32> MaterialSlotToBatchMaterialIndex;
};

struct FMaterialBatchBuildTextureArrayEligibility
{
	bool bEligible = false;
	FString Reason = TEXT("NotEvaluated");
};

TArray<FMaterialBatchBuildPropertyTextureColumnPlan> GetPropertyTextureColumnPlans()
{
	return {
		{ 0, TEXT("BaseColorSlice"), TEXT("baseColorSlice"), TEXT("int32"), INDEX_NONE, TEXT("Texture2DArray slice index for BaseColor") },
		{ 1, TEXT("NormalSlice"), TEXT("normalSlice"), TEXT("int32"), INDEX_NONE, TEXT("Texture2DArray slice index for Normal") },
		{ 2, TEXT("ORMSlice"), TEXT("ormSlice"), TEXT("int32"), INDEX_NONE, TEXT("Texture2DArray slice index for ORM") },
		{ 3, TEXT("EmissiveSlice"), TEXT("emissiveSlice"), TEXT("int32"), INDEX_NONE, TEXT("Texture2DArray slice index for Emissive") },
		{ 4, TEXT("MaskSlice"), TEXT("maskSlice"), TEXT("int32"), INDEX_NONE, TEXT("Texture2DArray slice index for Mask") },
		{ 5, TEXT("BaseVTUVMinX"), TEXT("baseVTUVMinX"), TEXT("float"), 0, TEXT("VT atlas UV rect minimum X for BaseColor") },
		{ 6, TEXT("BaseVTUVMinY"), TEXT("baseVTUVMinY"), TEXT("float"), 0, TEXT("VT atlas UV rect minimum Y for BaseColor") },
		{ 7, TEXT("BaseVTUVMaxX"), TEXT("baseVTUVMaxX"), TEXT("float"), 1, TEXT("VT atlas UV rect maximum X for BaseColor") },
		{ 8, TEXT("BaseVTUVMaxY"), TEXT("baseVTUVMaxY"), TEXT("float"), 1, TEXT("VT atlas UV rect maximum Y for BaseColor") },
		{ 9, TEXT("NormalVTUVMinX"), TEXT("normalVTUVMinX"), TEXT("float"), 0, TEXT("VT atlas UV rect minimum X for Normal") },
		{ 10, TEXT("NormalVTUVMinY"), TEXT("normalVTUVMinY"), TEXT("float"), 0, TEXT("VT atlas UV rect minimum Y for Normal") },
		{ 11, TEXT("NormalVTUVMaxX"), TEXT("normalVTUVMaxX"), TEXT("float"), 1, TEXT("VT atlas UV rect maximum X for Normal") },
		{ 12, TEXT("NormalVTUVMaxY"), TEXT("normalVTUVMaxY"), TEXT("float"), 1, TEXT("VT atlas UV rect maximum Y for Normal") },
		{ 13, TEXT("ORMVTUVMinX"), TEXT("ormVTUVMinX"), TEXT("float"), 0, TEXT("VT atlas UV rect minimum X for ORM") },
		{ 14, TEXT("ORMVTUVMinY"), TEXT("ormVTUVMinY"), TEXT("float"), 0, TEXT("VT atlas UV rect minimum Y for ORM") },
		{ 15, TEXT("ORMVTUVMaxX"), TEXT("ormVTUVMaxX"), TEXT("float"), 1, TEXT("VT atlas UV rect maximum X for ORM") },
		{ 16, TEXT("ORMVTUVMaxY"), TEXT("ormVTUVMaxY"), TEXT("float"), 1, TEXT("VT atlas UV rect maximum Y for ORM") }
	};
}

bool IsGroundBatchedSourceTag(const FString& TagString)
{
	TArray<FString> Parts;
	TagString.ParseIntoArray(Parts, TEXT("."), true);
	return (Parts.Num() == 6 || Parts.Num() == 7)
		&& Parts[0] == TEXT("EnvBatch")
		&& Parts[1] == TEXT("Source")
		&& Parts[3] == TEXT("Ground")
		&& Parts[4] == TEXT("Batched");
}

bool ShouldBakeInstanceVertexColors(const TArray<FString>& EnvBatchTags)
{
	for (const FString& EnvBatchTag : EnvBatchTags)
	{
		if (IsGroundBatchedSourceTag(EnvBatchTag))
		{
			return true;
		}
	}
	return false;
}

TArray<FMaterialBatchBuildGeometryMergeSourcePlan> BuildGeometryMergeSourcePlans(
	const TArray<FMaterialBatchBuildPlannedEntry>& Entries)
{
	TArray<FMaterialBatchBuildGeometryMergeSourcePlan> SourcePlans;
	for (int32 EntryIndex = 0; EntryIndex < Entries.Num(); ++EntryIndex)
	{
		const FMaterialBatchBuildPlannedEntry& Entry = Entries[EntryIndex];
		if (!Entry.bCandidate)
		{
			continue;
		}

		FMaterialBatchBuildGeometryMergeSourcePlan SourcePlan;
		SourcePlan.SourceEntryIndex = EntryIndex;
		SourcePlan.ActorName = Entry.ActorName;
		SourcePlan.ComponentName = Entry.ComponentName;
		SourcePlan.StaticMeshPath = Entry.AssetPath;
		SourcePlan.bBakeInstanceVertexColors = ShouldBakeInstanceVertexColors(Entry.EnvBatchTags);
		SourcePlan.bHasWorldTransform = Entry.bHasWorldTransform;
		SourcePlan.WorldLocation = Entry.WorldLocation;
		SourcePlan.WorldRotation = Entry.WorldRotation;
		SourcePlan.WorldScale = Entry.WorldScale;
		SourcePlan.FirstBatchMaterialIndex = Entry.FirstBatchMaterialIndex;
		SourcePlan.BatchMaterialIndexCount = Entry.BatchMaterialIndexCount;
		SourcePlan.MaterialSlotToBatchMaterialIndex.Reserve(Entry.BatchMaterialIndexCount);
		for (int32 MaterialSlotIndex = 0; MaterialSlotIndex < Entry.BatchMaterialIndexCount; ++MaterialSlotIndex)
		{
			SourcePlan.MaterialSlotToBatchMaterialIndex.Add(Entry.FirstBatchMaterialIndex + MaterialSlotIndex);
		}
		SourcePlans.Add(SourcePlan);
	}
	return SourcePlans;
}

bool IsTextureArrayChannel(const FString& ChannelName)
{
	return ChannelName == TEXT("BaseColor") ||
		ChannelName == TEXT("Normal") ||
		ChannelName == TEXT("ORM") ||
		ChannelName == TEXT("Emissive") ||
		ChannelName == TEXT("Mask");
}

float EstimateTextureSourceMB(int32 Width, int32 Height)
{
	if (Width <= 0 || Height <= 0)
	{
		return 0.f;
	}
	return static_cast<float>(static_cast<double>(Width) * static_cast<double>(Height) * 4.0 / (1024.0 * 1024.0));
}

FMaterialBatchBuildTextureArrayEligibility EvaluateTextureArrayBuildEligibility(
	const FMaterialBatchBuildTextureChannelPlan& ChannelPlan)
{
	FMaterialBatchBuildTextureArrayEligibility Result;

	if (!ChannelPlan.bFoundTexture || ChannelPlan.TexturePath.IsEmpty())
	{
		Result.Reason = TEXT("MissingTexture");
		return Result;
	}

	if (ChannelPlan.TextureClass == TEXT("Texture2DArray"))
	{
		Result.Reason = TEXT("ExistingTexture2DArrayInput");
		return Result;
	}

	if (ChannelPlan.TextureClass != TEXT("Texture2D"))
	{
		Result.Reason = FString::Printf(TEXT("UnsupportedTextureClass:%s"), *ChannelPlan.TextureClass);
		return Result;
	}

	if (!IsTextureArrayChannel(ChannelPlan.ChannelName))
	{
		Result.Reason = FString::Printf(TEXT("UnsupportedChannel:%s"), *ChannelPlan.ChannelName);
		return Result;
	}

	if (ChannelPlan.TextureWidth <= 0 || ChannelPlan.TextureHeight <= 0)
	{
		Result.Reason = TEXT("UnknownTextureSize");
		return Result;
	}

	Result.bEligible = true;
	Result.Reason = TEXT("Texture2D");
	return Result;
}

FMaterialBatchBuildTextureChannelPlan WithEvaluatedTextureArrayEligibility(
	const FMaterialBatchBuildTextureChannelPlan& ChannelPlan)
{
	FMaterialBatchBuildTextureChannelPlan Result = ChannelPlan;
	const FMaterialBatchBuildTextureArrayEligibility Eligibility = EvaluateTextureArrayBuildEligibility(ChannelPlan);
	Result.bTextureArrayBuildEligible = Eligibility.bEligible;
	Result.TextureArrayBuildReason = Eligibility.Reason;
	return Result;
}

int32 AssignTextureSliceIndex(
	TMap<FString, int32>& TextureToSliceIndex,
	TArray<FMaterialBatchBuildTextureArraySlicePlan>& SlicePlans,
	const FMaterialBatchBuildTextureChannelPlan& ChannelPlan)
{
	const FMaterialBatchBuildTextureChannelPlan EvaluatedChannelPlan = WithEvaluatedTextureArrayEligibility(ChannelPlan);
	if (!EvaluatedChannelPlan.bTextureArrayBuildEligible)
	{
		return INDEX_NONE;
	}

	if (int32* ExistingIndex = TextureToSliceIndex.Find(EvaluatedChannelPlan.TexturePath))
	{
		return *ExistingIndex;
	}

	const int32 NewIndex = TextureToSliceIndex.Num();
	TextureToSliceIndex.Add(EvaluatedChannelPlan.TexturePath, NewIndex);
	FMaterialBatchBuildTextureArraySlicePlan SlicePlan;
	SlicePlan.SliceIndex = NewIndex;
	SlicePlan.TexturePath = EvaluatedChannelPlan.TexturePath;
	SlicePlan.TextureClass = EvaluatedChannelPlan.TextureClass;
	SlicePlans.Add(SlicePlan);
	return NewIndex;
}

TArray<FMaterialBatchBuildPropertyRowPlan> BuildPropertyRowPlans(
	const TArray<FMaterialBatchBuildMaterialRow>& MaterialRows,
	FMaterialBatchBuildTextureArrayPlans& TextureArrayPlans)
{
	TMap<FString, int32> BaseColorTextureToSliceIndex;
	TMap<FString, int32> NormalTextureToSliceIndex;
	TMap<FString, int32> OrmTextureToSliceIndex;
	TMap<FString, int32> EmissiveTextureToSliceIndex;
	TMap<FString, int32> MaskTextureToSliceIndex;
	TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows;
	PropertyRows.Reserve(MaterialRows.Num());

	for (const FMaterialBatchBuildMaterialRow& MaterialRow : MaterialRows)
	{
		FMaterialBatchBuildPropertyRowPlan PropertyRow;
		PropertyRow.BatchMaterialIndex = MaterialRow.BatchMaterialIndex;
		PropertyRow.MaterialPath = MaterialRow.MaterialPath;

		for (const FMaterialBatchBuildTextureChannelPlan& ChannelPlan : MaterialRow.TextureChannels)
		{
			if (ChannelPlan.ChannelName == TEXT("BaseColor") && PropertyRow.BaseColorSliceIndex == INDEX_NONE)
			{
				PropertyRow.BaseColorSliceIndex = AssignTextureSliceIndex(
					BaseColorTextureToSliceIndex,
					TextureArrayPlans.BaseColor,
					ChannelPlan);
			}
			else if (ChannelPlan.ChannelName == TEXT("Normal") && PropertyRow.NormalSliceIndex == INDEX_NONE)
			{
				PropertyRow.NormalSliceIndex = AssignTextureSliceIndex(
					NormalTextureToSliceIndex,
					TextureArrayPlans.Normal,
					ChannelPlan);
			}
			else if (ChannelPlan.ChannelName == TEXT("ORM") && PropertyRow.OrmSliceIndex == INDEX_NONE)
			{
				PropertyRow.OrmSliceIndex = AssignTextureSliceIndex(
					OrmTextureToSliceIndex,
					TextureArrayPlans.Orm,
					ChannelPlan);
			}
			else if (ChannelPlan.ChannelName == TEXT("Emissive") && PropertyRow.EmissiveSliceIndex == INDEX_NONE)
			{
				PropertyRow.EmissiveSliceIndex = AssignTextureSliceIndex(
					EmissiveTextureToSliceIndex,
					TextureArrayPlans.Emissive,
					ChannelPlan);
			}
			else if (ChannelPlan.ChannelName == TEXT("Mask") && PropertyRow.MaskSliceIndex == INDEX_NONE)
			{
				PropertyRow.MaskSliceIndex = AssignTextureSliceIndex(
					MaskTextureToSliceIndex,
					TextureArrayPlans.Mask,
					ChannelPlan);
			}
			else if (ChannelPlan.ChannelName == TEXT("LightInfo") && PropertyRow.LightInfoTexturePath.IsEmpty())
			{
				PropertyRow.LightInfoTexturePath = ChannelPlan.TexturePath;
			}
		}

		PropertyRows.Add(PropertyRow);
	}

	return PropertyRows;
}

TArray<FMaterialBatchBuildPropertyRowPlan> BuildPropertyRowPlans(const TArray<FMaterialBatchBuildMaterialRow>& MaterialRows)
{
	FMaterialBatchBuildTextureArrayPlans UnusedTextureArrayPlans;
	return BuildPropertyRowPlans(MaterialRows, UnusedTextureArrayPlans);
}

void AppendTextureArraySliceRows(
	TArray<FString>& Lines,
	const FString& ChannelName,
	const TArray<FMaterialBatchBuildTextureArraySlicePlan>& SlicePlans,
	bool& bHasSlices)
{
	for (const FMaterialBatchBuildTextureArraySlicePlan& SlicePlan : SlicePlans)
	{
		bHasSlices = true;
		Lines.Add(FString::Printf(
			TEXT("| %s | %d | `%s` | %s |"),
			*ChannelName,
			SlicePlan.SliceIndex,
			SlicePlan.TexturePath.IsEmpty() ? TEXT("") : *SlicePlan.TexturePath,
			SlicePlan.TextureClass.IsEmpty() ? TEXT("") : *SlicePlan.TextureClass));
	}
}

void SetTextureArrayJsonField(
	const TSharedRef<FJsonObject>& TextureArraysObject,
	const TCHAR* FieldName,
	const TArray<FMaterialBatchBuildTextureArraySlicePlan>& SlicePlans)
{
	TArray<TSharedPtr<FJsonValue>> SliceValues;
	SliceValues.Reserve(SlicePlans.Num());
	for (const FMaterialBatchBuildTextureArraySlicePlan& SlicePlan : SlicePlans)
	{
		TSharedRef<FJsonObject> SliceObject = MakeShared<FJsonObject>();
		SliceObject->SetNumberField(TEXT("sliceIndex"), SlicePlan.SliceIndex);
		SliceObject->SetStringField(TEXT("texture"), SlicePlan.TexturePath);
		SliceObject->SetStringField(TEXT("textureClass"), SlicePlan.TextureClass);
		SliceValues.Add(MakeShared<FJsonValueObject>(SliceObject));
	}
	TextureArraysObject->SetArrayField(FieldName, SliceValues);
}

const FMaterialBatchBuildVTAtlasEntry* FindVTAtlasEntry(
	const TArray<FMaterialBatchBuildVTAtlasEntry>& AtlasEntries,
	const FString& ChannelName,
	const FString& TexturePath)
{
	for (const FMaterialBatchBuildVTAtlasEntry& Entry : AtlasEntries)
	{
		if (Entry.ChannelName == ChannelName && Entry.TexturePath == TexturePath)
		{
			return &Entry;
		}
	}
	return nullptr;
}

FString GetTexturePathForSlice(
	const TArray<FMaterialBatchBuildTextureArraySlicePlan>& SlicePlans,
	int32 SliceIndex)
{
	for (const FMaterialBatchBuildTextureArraySlicePlan& SlicePlan : SlicePlans)
	{
		if (SlicePlan.SliceIndex == SliceIndex)
		{
			return SlicePlan.TexturePath;
		}
	}
	return FString();
}

void ApplyVTAtlasRectsToPropertyRows(
	const TArray<FMaterialBatchBuildVTAtlasEntry>& AtlasEntries,
	const FMaterialBatchBuildTextureArrayPlans& TextureArrayPlans,
	TArray<FMaterialBatchBuildPropertyRowPlan>& PropertyRows)
{
	for (FMaterialBatchBuildPropertyRowPlan& PropertyRow : PropertyRows)
	{
		PropertyRow.SourceTexturePath = GetTexturePathForSlice(TextureArrayPlans.BaseColor, PropertyRow.BaseColorSliceIndex);
		if (const FMaterialBatchBuildVTAtlasEntry* AtlasEntry =
			FindVTAtlasEntry(AtlasEntries, TEXT("BaseColor"), PropertyRow.SourceTexturePath))
		{
			PropertyRow.UVRectMin = AtlasEntry->UVRectMin;
			PropertyRow.UVRectMax = AtlasEntry->UVRectMax;
		}

		PropertyRow.NormalSourceTexturePath = GetTexturePathForSlice(TextureArrayPlans.Normal, PropertyRow.NormalSliceIndex);
		if (const FMaterialBatchBuildVTAtlasEntry* AtlasEntry =
			FindVTAtlasEntry(AtlasEntries, TEXT("Normal"), PropertyRow.NormalSourceTexturePath))
		{
			PropertyRow.NormalUVRectMin = AtlasEntry->UVRectMin;
			PropertyRow.NormalUVRectMax = AtlasEntry->UVRectMax;
		}

		PropertyRow.OrmSourceTexturePath = GetTexturePathForSlice(TextureArrayPlans.Orm, PropertyRow.OrmSliceIndex);
		if (const FMaterialBatchBuildVTAtlasEntry* AtlasEntry =
			FindVTAtlasEntry(AtlasEntries, TEXT("ORM"), PropertyRow.OrmSourceTexturePath))
		{
			PropertyRow.OrmUVRectMin = AtlasEntry->UVRectMin;
			PropertyRow.OrmUVRectMax = AtlasEntry->UVRectMax;
		}
	}
}

float SumVTAtlasSourceMB(const TArray<FMaterialBatchBuildVTAtlasEntry>& AtlasEntries)
{
	float TotalMB = 0.f;
	for (const FMaterialBatchBuildVTAtlasEntry& Entry : AtlasEntries)
	{
		TotalMB += Entry.EstimatedSourceMB;
	}
	return TotalMB;
}

float SumTextureArraySourceMB(const FMaterialBatchBuildPlan& Plan)
{
	TMap<FString, float> TextureMBByPath;
	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			const FMaterialBatchBuildTextureChannelPlan EvaluatedChannel =
				WithEvaluatedTextureArrayEligibility(Channel);
			if (EvaluatedChannel.bTextureArrayBuildEligible && !EvaluatedChannel.TexturePath.IsEmpty())
			{
				TextureMBByPath.FindOrAdd(EvaluatedChannel.TexturePath) =
					EstimateTextureSourceMB(EvaluatedChannel.TextureWidth, EvaluatedChannel.TextureHeight);
			}
		}
	}

	float TotalMB = 0.f;
	for (const TPair<FString, float>& Pair : TextureMBByPath)
	{
		TotalMB += Pair.Value;
	}
	return TotalMB;
}

void SetVector2DJsonField(const TSharedRef<FJsonObject>& Object, const TCHAR* FieldName, const FVector2D& Value)
{
	TSharedRef<FJsonObject> VectorObject = MakeShared<FJsonObject>();
	VectorObject->SetNumberField(TEXT("x"), Value.X);
	VectorObject->SetNumberField(TEXT("y"), Value.Y);
	Object->SetObjectField(FieldName, VectorObject);
}

void SetPropertyTextureLayoutJsonField(const TSharedRef<FJsonObject>& RootObject, int32 RowCount)
{
	TSharedRef<FJsonObject> LayoutObject = MakeShared<FJsonObject>();
	LayoutObject->SetStringField(TEXT("rowKey"), TEXT("batchMaterialIndex"));
	LayoutObject->SetNumberField(TEXT("rowCount"), RowCount);
	LayoutObject->SetStringField(TEXT("missingSliceValue"), TEXT("-1"));
	LayoutObject->SetStringField(TEXT("encoding"), TEXT("one material row per texture row; x=propertyIndex, y=batchMaterialIndex"));

	TArray<TSharedPtr<FJsonValue>> ColumnValues;
	const TArray<FMaterialBatchBuildPropertyTextureColumnPlan> ColumnPlans = GetPropertyTextureColumnPlans();
	ColumnValues.Reserve(ColumnPlans.Num());
	for (const FMaterialBatchBuildPropertyTextureColumnPlan& ColumnPlan : ColumnPlans)
	{
		TSharedRef<FJsonObject> ColumnObject = MakeShared<FJsonObject>();
		ColumnObject->SetNumberField(TEXT("propertyIndex"), ColumnPlan.PropertyIndex);
		ColumnObject->SetStringField(TEXT("name"), ColumnPlan.Name);
		ColumnObject->SetStringField(TEXT("sourceField"), ColumnPlan.SourceField);
		ColumnObject->SetStringField(TEXT("valueType"), ColumnPlan.ValueType);
		ColumnObject->SetNumberField(TEXT("defaultValue"), ColumnPlan.DefaultIntValue);
		ColumnObject->SetStringField(TEXT("description"), ColumnPlan.Description);
		ColumnValues.Add(MakeShared<FJsonValueObject>(ColumnObject));
	}
	LayoutObject->SetArrayField(TEXT("columns"), ColumnValues);
	RootObject->SetObjectField(TEXT("propertyTextureLayout"), LayoutObject);
}

TSharedRef<FJsonObject> BuildVectorJsonObject(const FVector& Value)
{
	TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetNumberField(TEXT("x"), Value.X);
	Object->SetNumberField(TEXT("y"), Value.Y);
	Object->SetNumberField(TEXT("z"), Value.Z);
	return Object;
}

TSharedRef<FJsonObject> BuildRotatorJsonObject(const FRotator& Value)
{
	TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
	Object->SetNumberField(TEXT("pitch"), Value.Pitch);
	Object->SetNumberField(TEXT("yaw"), Value.Yaw);
	Object->SetNumberField(TEXT("roll"), Value.Roll);
	return Object;
}

void SetGeometryMergePlanJsonField(
	const TSharedRef<FJsonObject>& RootObject,
	const FMaterialBatchBuildPlan& Plan,
	const TArray<FMaterialBatchBuildGeometryMergeSourcePlan>& SourcePlans)
{
	TSharedRef<FJsonObject> GeometryObject = MakeShared<FJsonObject>();
	GeometryObject->SetStringField(TEXT("targetProxyMesh"), Plan.ProxyMeshPackage);
	GeometryObject->SetStringField(TEXT("sourceCoordinateSpace"), TEXT("World"));
	GeometryObject->SetStringField(TEXT("materialIndexChannel"), TEXT("TexCoord7.x"));
	GeometryObject->SetStringField(TEXT("materialIndexEncoding"), TEXT("write batchMaterialIndex as a float per merged vertex"));
	GeometryObject->SetStringField(TEXT("mergeGranularity"), TEXT("cluster"));
	GeometryObject->SetStringField(TEXT("cluster"), Plan.SanitizedClusterName);
	GeometryObject->SetNumberField(TEXT("sourceCount"), SourcePlans.Num());

	TArray<TSharedPtr<FJsonValue>> SourceValues;
	SourceValues.Reserve(SourcePlans.Num());
	for (const FMaterialBatchBuildGeometryMergeSourcePlan& SourcePlan : SourcePlans)
	{
		TSharedRef<FJsonObject> SourceObject = MakeShared<FJsonObject>();
		SourceObject->SetNumberField(TEXT("sourceEntryIndex"), SourcePlan.SourceEntryIndex);
		SourceObject->SetStringField(TEXT("actor"), SourcePlan.ActorName);
		SourceObject->SetStringField(TEXT("component"), SourcePlan.ComponentName);
		SourceObject->SetStringField(TEXT("staticMesh"), SourcePlan.StaticMeshPath);
		SourceObject->SetBoolField(TEXT("hasWorldTransform"), SourcePlan.bHasWorldTransform);
		SourceObject->SetObjectField(TEXT("worldLocation"), BuildVectorJsonObject(SourcePlan.WorldLocation));
		SourceObject->SetObjectField(TEXT("worldRotation"), BuildRotatorJsonObject(SourcePlan.WorldRotation));
		SourceObject->SetObjectField(TEXT("worldScale"), BuildVectorJsonObject(SourcePlan.WorldScale));
		SourceObject->SetNumberField(TEXT("firstBatchMaterialIndex"), SourcePlan.FirstBatchMaterialIndex);
		SourceObject->SetNumberField(TEXT("batchMaterialIndexCount"), SourcePlan.BatchMaterialIndexCount);

		TArray<TSharedPtr<FJsonValue>> RemapValues;
		RemapValues.Reserve(SourcePlan.MaterialSlotToBatchMaterialIndex.Num());
		for (int32 MaterialSlotIndex = 0; MaterialSlotIndex < SourcePlan.MaterialSlotToBatchMaterialIndex.Num(); ++MaterialSlotIndex)
		{
			TSharedRef<FJsonObject> RemapObject = MakeShared<FJsonObject>();
			RemapObject->SetNumberField(TEXT("sourceMaterialSlotIndex"), MaterialSlotIndex);
			RemapObject->SetNumberField(TEXT("batchMaterialIndex"), SourcePlan.MaterialSlotToBatchMaterialIndex[MaterialSlotIndex]);
			RemapValues.Add(MakeShared<FJsonValueObject>(RemapObject));
		}
		SourceObject->SetArrayField(TEXT("materialSlotRemap"), RemapValues);
		SourceValues.Add(MakeShared<FJsonValueObject>(SourceObject));
	}
	GeometryObject->SetArrayField(TEXT("sources"), SourceValues);
	RootObject->SetObjectField(TEXT("geometryMergePlan"), GeometryObject);
}

FMaterialBatchBuildTierLayerSelection MakeTierLayerSelection(
	const TCHAR* TierName,
	bool bLoadSourceLayer,
	bool bLoadProxyLayer,
	bool bLoadBakedLayer,
	const TCHAR* FallbackPolicy)
{
	FMaterialBatchBuildTierLayerSelection Selection;
	Selection.TierName = TierName;
	Selection.bLoadSourceLayer = bLoadSourceLayer;
	Selection.bLoadProxyLayer = bLoadProxyLayer;
	Selection.bLoadBakedLayer = bLoadBakedLayer;
	Selection.FallbackPolicy = FallbackPolicy;
	return Selection;
}

bool HasStringPrefix(const TArray<FString>& Values, const TCHAR* Prefix)
{
	for (const FString& Value : Values)
	{
		if (Value.StartsWith(Prefix))
		{
			return true;
		}
	}
	return false;
}

bool ContainsStringExact(const TArray<FString>& Values, const FString& ExpectedValue)
{
	for (const FString& Value : Values)
	{
		if (Value.Equals(ExpectedValue, ESearchCase::CaseSensitive))
		{
			return true;
		}
	}
	return false;
}

FString NormalizeLayerBackend(const FString& LayerBackend)
{
	return LayerBackend.Equals(TEXT("DataLayer"), ESearchCase::IgnoreCase)
		? TEXT("DataLayer")
		: TEXT("StreamingLevel");
}

FString GetSourceLayerNameForStreamingLevelPlan(const FMaterialBatchBuildPlan& Plan)
{
	TArray<FString> SourceLayerNames;
	for (const FMaterialBatchBuildPlannedEntry& Entry : Plan.PlannedEntries)
	{
		if (!HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Source.")) || Entry.ActualStreamingLevelName.IsEmpty())
		{
			continue;
		}
		SourceLayerNames.AddUnique(Entry.ActualStreamingLevelName);
	}
	SourceLayerNames.Sort();
	return SourceLayerNames.Num() == 1
		? SourceLayerNames[0]
		: TEXT("CurrentStreamingLevel");
}

void SetSourceProxyLayerPlanJsonField(
	const TSharedRef<FJsonObject>& RootObject,
	const FMaterialBatchBuildSourceProxyLayerPlan& LayerPlan)
{
	TSharedRef<FJsonObject> LayerObject = MakeShared<FJsonObject>();
	LayerObject->SetStringField(TEXT("exclusivityGroup"), LayerPlan.ExclusivityGroup);
	LayerObject->SetStringField(TEXT("layerBackend"), LayerPlan.LayerBackend);
	LayerObject->SetStringField(TEXT("sourceLayer"), LayerPlan.SourceLayerName);
	LayerObject->SetStringField(TEXT("proxyLayer"), LayerPlan.ProxyLayerName);
	LayerObject->SetStringField(TEXT("bakedLayer"), LayerPlan.BakedLayerName);
	LayerObject->SetBoolField(TEXT("requiresMutualExclusion"), LayerPlan.bRequiresMutualExclusion);
	LayerObject->SetBoolField(TEXT("hasTagConflicts"), LayerPlan.bHasTagConflicts);
	LayerObject->SetNumberField(TEXT("sourceActorCount"), LayerPlan.SourceActorCount);
	LayerObject->SetNumberField(TEXT("proxyActorCount"), LayerPlan.ProxyActorCount);
	LayerObject->SetNumberField(TEXT("bakedActorCount"), LayerPlan.BakedActorCount);
	LayerObject->SetNumberField(TEXT("conflictActorCount"), LayerPlan.ConflictActorCount);

	TArray<TSharedPtr<FJsonValue>> SelectionValues;
	SelectionValues.Reserve(LayerPlan.TierSelections.Num());
	for (const FMaterialBatchBuildTierLayerSelection& Selection : LayerPlan.TierSelections)
	{
		TSharedRef<FJsonObject> SelectionObject = MakeShared<FJsonObject>();
		SelectionObject->SetStringField(TEXT("tier"), Selection.TierName);
		SelectionObject->SetBoolField(TEXT("loadSource"), Selection.bLoadSourceLayer);
		SelectionObject->SetBoolField(TEXT("loadProxy"), Selection.bLoadProxyLayer);
		SelectionObject->SetBoolField(TEXT("loadBaked"), Selection.bLoadBakedLayer);
		SelectionObject->SetStringField(TEXT("fallbackPolicy"), Selection.FallbackPolicy);
		SelectionValues.Add(MakeShared<FJsonValueObject>(SelectionObject));
	}
	LayerObject->SetArrayField(TEXT("tierSelections"), SelectionValues);
	RootObject->SetObjectField(TEXT("sourceProxyLayerPlan"), LayerObject);
}

void SetSourceProxyLayerReadinessJsonField(
	const TSharedRef<FJsonObject>& RootObject,
	const FMaterialBatchBuildSourceProxyLayerReadiness& Readiness)
{
	TSharedRef<FJsonObject> ReadinessObject = MakeShared<FJsonObject>();
	ReadinessObject->SetStringField(TEXT("layerBackend"), Readiness.LayerBackend);
	ReadinessObject->SetNumberField(TEXT("entryCount"), Readiness.EntryCount);
	ReadinessObject->SetNumberField(TEXT("readyEntryCount"), Readiness.ReadyEntryCount);
	ReadinessObject->SetNumberField(TEXT("missingLayerTagEntryCount"), Readiness.MissingLayerTagEntryCount);
	ReadinessObject->SetNumberField(TEXT("conflictEntryCount"), Readiness.ConflictEntryCount);
	ReadinessObject->SetNumberField(TEXT("excludedEntryCount"), Readiness.ExcludedEntryCount);
	ReadinessObject->SetNumberField(TEXT("actualLayerMatchCount"), Readiness.ActualLayerMatchCount);
	ReadinessObject->SetNumberField(TEXT("missingActualLayerCount"), Readiness.MissingActualLayerCount);
	ReadinessObject->SetNumberField(TEXT("unexpectedActualLayerCount"), Readiness.UnexpectedActualLayerCount);
	ReadinessObject->SetNumberField(TEXT("notRequiredActualLayerCount"), Readiness.NotRequiredActualLayerCount);

	TArray<TSharedPtr<FJsonValue>> AssignmentValues;
	AssignmentValues.Reserve(Readiness.Assignments.Num());
	for (const FMaterialBatchBuildSourceProxyLayerAssignment& Assignment : Readiness.Assignments)
	{
		TSharedRef<FJsonObject> AssignmentObject = MakeShared<FJsonObject>();
		AssignmentObject->SetNumberField(TEXT("sourceEntryIndex"), Assignment.SourceEntryIndex);
		AssignmentObject->SetStringField(TEXT("actor"), Assignment.ActorName);
		AssignmentObject->SetStringField(TEXT("component"), Assignment.ComponentName);
		AssignmentObject->SetStringField(TEXT("layerRole"), Assignment.LayerRole);
		AssignmentObject->SetStringField(TEXT("expectedLayer"), Assignment.ExpectedLayerName);
		AssignmentObject->SetBoolField(TEXT("readyForLayerValidation"), Assignment.bReadyForLayerValidation);
		AssignmentObject->SetBoolField(TEXT("hasActualLayerEvidence"), Assignment.bHasActualLayerEvidence);
		AssignmentObject->SetBoolField(TEXT("hasActualDataLayerEvidence"), Assignment.bHasActualDataLayerEvidence);
		AssignmentObject->SetBoolField(TEXT("matchesExpectedLayer"), Assignment.bMatchesExpectedLayer);
		AssignmentObject->SetBoolField(TEXT("matchesExpectedDataLayer"), Assignment.bMatchesExpectedDataLayer);
		AssignmentObject->SetStringField(TEXT("readinessReason"), Assignment.ReadinessReason);
		AssignmentObject->SetStringField(TEXT("layerValidationStatus"), Assignment.LayerValidationStatus);
		AssignmentObject->SetStringField(TEXT("dataLayerValidationStatus"), Assignment.DataLayerValidationStatus);
		AssignmentObject->SetStringField(TEXT("actualStreamingLevel"), Assignment.ActualStreamingLevelName);
		AssignmentObject->SetStringField(TEXT("actualLevelPackage"), Assignment.ActualLevelPackageName);

		TArray<TSharedPtr<FJsonValue>> ActualLayerValues;
		ActualLayerValues.Reserve(Assignment.ActualLayerNames.Num());
		for (const FString& ActualLayerName : Assignment.ActualLayerNames)
		{
			ActualLayerValues.Add(MakeShared<FJsonValueString>(ActualLayerName));
		}
		AssignmentObject->SetArrayField(TEXT("actualLayers"), ActualLayerValues);

		TArray<TSharedPtr<FJsonValue>> ActualDataLayerValues;
		ActualDataLayerValues.Reserve(Assignment.ActualDataLayerNames.Num());
		for (const FString& ActualDataLayerName : Assignment.ActualDataLayerNames)
		{
			ActualDataLayerValues.Add(MakeShared<FJsonValueString>(ActualDataLayerName));
		}
		AssignmentObject->SetArrayField(TEXT("actualDataLayers"), ActualDataLayerValues);

		TArray<TSharedPtr<FJsonValue>> TagValues;
		TagValues.Reserve(Assignment.EnvBatchTags.Num());
		for (const FString& EnvBatchTag : Assignment.EnvBatchTags)
		{
			TagValues.Add(MakeShared<FJsonValueString>(EnvBatchTag));
		}
		AssignmentObject->SetArrayField(TEXT("envBatchTags"), TagValues);
		AssignmentValues.Add(MakeShared<FJsonValueObject>(AssignmentObject));
	}
	ReadinessObject->SetArrayField(TEXT("assignments"), AssignmentValues);
	RootObject->SetObjectField(TEXT("sourceProxyLayerReadiness"), ReadinessObject);
}

void SetSourceProxyAssetReadinessJsonField(
	const TSharedRef<FJsonObject>& RootObject,
	const FMaterialBatchBuildSourceProxyAssetReadiness& Readiness)
{
	TSharedRef<FJsonObject> ReadinessObject = MakeShared<FJsonObject>();
	ReadinessObject->SetNumberField(TEXT("entryCount"), Readiness.EntryCount);
	ReadinessObject->SetNumberField(TEXT("readyPairCount"), Readiness.ReadyPairCount);
	ReadinessObject->SetNumberField(TEXT("missingSourceAssetCount"), Readiness.MissingSourceAssetCount);
	ReadinessObject->SetNumberField(TEXT("missingProxyAssetCount"), Readiness.MissingProxyAssetCount);
	ReadinessObject->SetNumberField(TEXT("generatedProxyFallbackCount"), Readiness.GeneratedProxyFallbackCount);
	ReadinessObject->SetNumberField(TEXT("authoredProxyCount"), Readiness.AuthoredProxyCount);
	ReadinessObject->SetNumberField(TEXT("notRequiredCount"), Readiness.NotRequiredCount);
	ReadinessObject->SetNumberField(TEXT("conflictCount"), Readiness.ConflictCount);

	TArray<TSharedPtr<FJsonValue>> AssignmentValues;
	AssignmentValues.Reserve(Readiness.Assignments.Num());
	for (const FMaterialBatchBuildSourceProxyAssetAssignment& Assignment : Readiness.Assignments)
	{
		TSharedRef<FJsonObject> AssignmentObject = MakeShared<FJsonObject>();
		AssignmentObject->SetNumberField(TEXT("sourceEntryIndex"), Assignment.SourceEntryIndex);
		AssignmentObject->SetStringField(TEXT("actor"), Assignment.ActorName);
		AssignmentObject->SetStringField(TEXT("component"), Assignment.ComponentName);
		AssignmentObject->SetStringField(TEXT("layerRole"), Assignment.LayerRole);
		AssignmentObject->SetStringField(TEXT("sourceAsset"), Assignment.SourceAssetPath);
		AssignmentObject->SetStringField(TEXT("proxyAsset"), Assignment.ProxyAssetPath);
		AssignmentObject->SetNumberField(TEXT("sourceLODIndex"), Assignment.SourceLODIndex);
		AssignmentObject->SetNumberField(TEXT("proxyLODIndex"), Assignment.ProxyLODIndex);
		AssignmentObject->SetBoolField(TEXT("hasSourceAsset"), Assignment.bHasSourceAsset);
		AssignmentObject->SetBoolField(TEXT("hasProxyAsset"), Assignment.bHasProxyAsset);
		AssignmentObject->SetBoolField(TEXT("usesGeneratedProxy"), Assignment.bUsesGeneratedProxy);
		AssignmentObject->SetBoolField(TEXT("usesAuthoredProxy"), Assignment.bUsesAuthoredProxy);
		AssignmentObject->SetBoolField(TEXT("readyForAssetPairing"), Assignment.bReadyForAssetPairing);
		AssignmentObject->SetStringField(TEXT("readinessStatus"), Assignment.ReadinessStatus);
		AssignmentObject->SetStringField(TEXT("readinessReason"), Assignment.ReadinessReason);

		TArray<TSharedPtr<FJsonValue>> TagValues;
		TagValues.Reserve(Assignment.EnvBatchTags.Num());
		for (const FString& EnvBatchTag : Assignment.EnvBatchTags)
		{
			TagValues.Add(MakeShared<FJsonValueString>(EnvBatchTag));
		}
		AssignmentObject->SetArrayField(TEXT("envBatchTags"), TagValues);
		AssignmentValues.Add(MakeShared<FJsonValueObject>(AssignmentObject));
	}
	ReadinessObject->SetArrayField(TEXT("assignments"), AssignmentValues);
	RootObject->SetObjectField(TEXT("sourceProxyAssetReadiness"), ReadinessObject);
}

void SetSourceProxyAssetConfigSetJsonField(
	const TSharedRef<FJsonObject>& RootObject,
	const FMaterialBatchBuildSourceProxyAssetConfigSet& ConfigSet)
{
	TSharedRef<FJsonObject> ConfigSetObject = MakeShared<FJsonObject>();
	ConfigSetObject->SetNumberField(TEXT("configCount"), ConfigSet.ConfigCount);
	ConfigSetObject->SetNumberField(TEXT("readyConfigCount"), ConfigSet.ReadyConfigCount);
	ConfigSetObject->SetNumberField(TEXT("generatedFallbackConfigCount"), ConfigSet.GeneratedFallbackConfigCount);
	ConfigSetObject->SetNumberField(TEXT("authoredProxyConfigCount"), ConfigSet.AuthoredProxyConfigCount);
	ConfigSetObject->SetNumberField(TEXT("missingSourceReferenceCount"), ConfigSet.MissingSourceReferenceCount);

	TArray<TSharedPtr<FJsonValue>> ConfigValues;
	ConfigValues.Reserve(ConfigSet.Configs.Num());
	for (const FMaterialBatchBuildSourceProxyAssetConfig& Config : ConfigSet.Configs)
	{
		TSharedRef<FJsonObject> ConfigObject = MakeShared<FJsonObject>();
		ConfigObject->SetStringField(TEXT("objectKey"), Config.ObjectKey);
		ConfigObject->SetStringField(TEXT("actor"), Config.ActorName);
		ConfigObject->SetStringField(TEXT("component"), Config.ComponentName);
		ConfigObject->SetStringField(TEXT("layerRole"), Config.LayerRole);
		ConfigObject->SetStringField(TEXT("sourceAsset"), Config.SourceAssetPath);
		ConfigObject->SetStringField(TEXT("explicitProxyAsset"), Config.AuthorProxyAssetPath);
		ConfigObject->SetStringField(TEXT("authorProxyAsset"), Config.AuthorProxyAssetPath);
		ConfigObject->SetStringField(TEXT("generatedProxyAsset"), Config.GeneratedProxyAssetPath);
		ConfigObject->SetNumberField(TEXT("sourceLODIndex"), Config.SourceLODIndex);
		ConfigObject->SetNumberField(TEXT("proxyLODIndex"), Config.ProxyLODIndex);
		ConfigObject->SetStringField(TEXT("surfaceKind"), Config.SurfaceKind);
		ConfigObject->SetStringField(TEXT("bakePolicy"), Config.BakePolicy);
		ConfigObject->SetStringField(TEXT("interactionPolicy"), Config.InteractionPolicy);
		ConfigObject->SetStringField(TEXT("configSource"), Config.ConfigSource);
		ConfigObject->SetBoolField(TEXT("usesGeneratedProxyFallback"), Config.bUsesGeneratedProxyFallback);
		ConfigObject->SetBoolField(TEXT("readyForAssetPairing"), Config.bReadyForAssetPairing);
		ConfigObject->SetStringField(TEXT("readinessStatus"), Config.ReadinessStatus);
		ConfigObject->SetStringField(TEXT("readinessReason"), Config.ReadinessReason);
		ConfigValues.Add(MakeShared<FJsonValueObject>(ConfigObject));
	}
	ConfigSetObject->SetArrayField(TEXT("configs"), ConfigValues);
	RootObject->SetObjectField(TEXT("sourceProxyAssetConfigSet"), ConfigSetObject);
}

void SetResidencyRiskPlanJsonField(
	const TSharedRef<FJsonObject>& RootObject,
	const FMaterialBatchBuildResidencyRiskPlan& ResidencyRiskPlan)
{
	TSharedRef<FJsonObject> ResidencyObject = MakeShared<FJsonObject>();
	ResidencyObject->SetStringField(TEXT("textureBackend"), ResidencyRiskPlan.TextureBackend);
	ResidencyObject->SetBoolField(TEXT("vtAtlasMainPath"), ResidencyRiskPlan.bVTAtlasMainPath);
	ResidencyObject->SetBoolField(TEXT("textureArrayFallbackPresent"), ResidencyRiskPlan.bTextureArrayFallbackPresent);
	ResidencyObject->SetBoolField(TEXT("allowTextureArrayFallbackInProduction"), ResidencyRiskPlan.bAllowTextureArrayFallbackInProduction);
	ResidencyObject->SetBoolField(TEXT("duplicateResidencyRisk"), ResidencyRiskPlan.bDuplicateResidencyRisk);
	ResidencyObject->SetBoolField(TEXT("requiresSourceProxyUnload"), ResidencyRiskPlan.bRequiresSourceProxyUnload);
	ResidencyObject->SetNumberField(TEXT("estimatedVTPoolMB"), ResidencyRiskPlan.EstimatedVTPoolMB);
	ResidencyObject->SetNumberField(TEXT("estimatedStreamingPoolMB"), ResidencyRiskPlan.EstimatedStreamingPoolMB);
	ResidencyObject->SetNumberField(TEXT("estimatedCombinedPoolMB"), ResidencyRiskPlan.EstimatedCombinedPoolMB);
	ResidencyObject->SetStringField(TEXT("residencyGate"), ResidencyRiskPlan.ResidencyGate);
	ResidencyObject->SetStringField(TEXT("recommendation"), ResidencyRiskPlan.Recommendation);
	RootObject->SetObjectField(TEXT("residencyRiskPlan"), ResidencyObject);
}
}

FMaterialBatchBuildPlan FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(const FMaterialBatchBuildPlanOptions& Options)
{
	FMaterialBatchBuildPlan Plan;
	Plan.Options = Options;
	Plan.bDryRun = Options.bDryRun;
	Plan.SanitizedClusterName = SanitizePackageSegment(Options.ClusterName, TEXT("Default"));
	Plan.SanitizedTierName = SanitizePackageSegment(Options.TierName, TEXT("Mid"));

	const FString OutputRoot = NormalizeOutputRoot(Options.OutputRoot);
	Plan.OutputFolder = JoinPackagePath(JoinPackagePath(OutputRoot, Plan.SanitizedTierName), Plan.SanitizedClusterName);

	Plan.ProxyMeshPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("SM_BatchProxy_%s"), *Plan.SanitizedClusterName));
	Plan.BatchMaterialInstancePackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("MI_Env_Batch_%s"), *Plan.SanitizedClusterName));
	Plan.BatchParentMaterialPackage = GetDefaultBatchParentMaterialPath();
	Plan.TextureBackend = Options.TextureBackend.IsEmpty() ? TEXT("VTAtlas") : Options.TextureBackend;
	Plan.SurfaceKind = Options.SurfaceKind.IsEmpty() ? TEXT("MixedStatic") : Options.SurfaceKind;
	Plan.BakePolicy = Options.BakePolicy.IsEmpty() ? TEXT("StaticBake") : Options.BakePolicy;
	Plan.SourceProxyExclusivityGroup = Options.SourceProxyExclusivityGroup.IsEmpty()
		? Plan.SanitizedClusterName
		: SanitizePackageSegment(Options.SourceProxyExclusivityGroup, Plan.SanitizedClusterName);
	Plan.VTAtlasPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("VT_Atlas_%s"), *Plan.SanitizedClusterName));
	Plan.VTAtlasChannel = TEXT("Combined");
	Plan.BaseColorArrayPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("T2DA_BaseColor_%s"), *Plan.SanitizedClusterName));
	Plan.NormalArrayPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("T2DA_Normal_%s"), *Plan.SanitizedClusterName));
	Plan.OrmArrayPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("T2DA_ORM_%s"), *Plan.SanitizedClusterName));
	Plan.EmissiveArrayPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("T2DA_Emissive_%s"), *Plan.SanitizedClusterName));
	Plan.MaskArrayPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("T2DA_Mask_%s"), *Plan.SanitizedClusterName));
	Plan.PropertyTexturePackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("T_PropTexture_%s"), *Plan.SanitizedClusterName));
	Plan.MappingDataAssetPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("DA_MaterialBatchMap_%s"), *Plan.SanitizedClusterName));
	Plan.BatchPropertyTextureParameterName = GetDefaultBatchPropertyTextureParameterName();
	Plan.BatchMaterialIndexChannel = TEXT("TexCoord7.x");
	Plan.BatchMaterialIndexSource = TEXT("floor(TexCoord7.x)");
	Plan.BatchPropertyTextureRowEncoding = TEXT("sample _PropTexture at x=(propertyIndex+0.5)/width, y=(batchMaterialIndex+0.5)/height");

	Plan.GeneratedPackageNames = {
		Plan.ProxyMeshPackage,
		Plan.BatchMaterialInstancePackage,
		Plan.VTAtlasPackage,
		Plan.BaseColorArrayPackage,
		Plan.NormalArrayPackage,
		Plan.OrmArrayPackage,
		Plan.EmissiveArrayPackage,
		Plan.MaskArrayPackage,
		Plan.PropertyTexturePackage,
		Plan.MappingDataAssetPackage
	};
	Plan.SourceProxyLayerPlan = BuildSourceProxyLayerPlan(Plan);
	Plan.SourceProxyLayerReadiness = BuildSourceProxyLayerReadiness(Plan);
	Plan.SourceProxyAssetReadiness = BuildSourceProxyAssetReadiness(Plan);
	Plan.SourceProxyAssetConfigSet = BuildSourceProxyAssetConfigSet(Plan);
	Plan.ResidencyRiskPlan = BuildResidencyRiskPlan(Plan);

	return Plan;
}

void FMaterialBatchBuildPlanBuilder::ApplyCandidateSummary(
	FMaterialBatchBuildPlan& Plan,
	const FMaterialBatchBuildCandidateSummary& Summary)
{
	Plan.CandidateSummary = Summary;
}

FMaterialBatchBuildSourceProxyLayerPlan FMaterialBatchBuildPlanBuilder::BuildSourceProxyLayerPlan(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildSourceProxyLayerPlan LayerPlan;
	LayerPlan.ExclusivityGroup = Plan.SourceProxyExclusivityGroup.IsEmpty()
		? Plan.SanitizedClusterName
		: Plan.SourceProxyExclusivityGroup;
	LayerPlan.LayerBackend = NormalizeLayerBackend(Plan.Options.LayerBackend);
	const bool bStreamingLevelBackend = LayerPlan.LayerBackend.Equals(TEXT("StreamingLevel"), ESearchCase::IgnoreCase);
	const FString LayerPrefix = bStreamingLevelBackend
		? FString::Printf(TEXT("SL_%s"), *LayerPlan.ExclusivityGroup)
		: FString::Printf(TEXT("DL_%s"), *LayerPlan.ExclusivityGroup);
	LayerPlan.SourceLayerName = bStreamingLevelBackend
		? GetSourceLayerNameForStreamingLevelPlan(Plan)
		: FString::Printf(TEXT("%s_Source"), *LayerPrefix);
	LayerPlan.ProxyLayerName = FString::Printf(TEXT("%s_Proxy"), *LayerPrefix);
	LayerPlan.BakedLayerName = FString::Printf(TEXT("%s_Baked"), *LayerPrefix);
	LayerPlan.bRequiresMutualExclusion = true;
	LayerPlan.bHasTagConflicts = Plan.TagDiagnostics.SourceProxyConflictActorCount > 0;
	LayerPlan.SourceActorCount = Plan.TagDiagnostics.SourceActorCount;
	LayerPlan.ProxyActorCount = Plan.TagDiagnostics.ProxyActorCount;
	LayerPlan.BakedActorCount = Plan.TagDiagnostics.BakedActorCount;
	LayerPlan.ConflictActorCount = Plan.TagDiagnostics.SourceProxyConflictActorCount;
	LayerPlan.TierSelections = {
		MakeTierLayerSelection(TEXT("Epic"), true, false, false, TEXT("Keep author Source layer for maximum visual parity.")),
		MakeTierLayerSelection(TEXT("High"), true, false, false, TEXT("Keep author Source layer unless the cluster is promoted to reviewed proxy mode.")),
		MakeTierLayerSelection(TEXT("Mid"), false, true, false, TEXT("Prefer generated Proxy layer with VT atlas batch material.")),
		MakeTierLayerSelection(TEXT("Low"), false, false, true, TEXT("Prefer Baked layer; fall back to Proxy layer only when baked output is unavailable."))
	};
	return LayerPlan;
}

FMaterialBatchBuildSourceProxyLayerReadiness FMaterialBatchBuildPlanBuilder::BuildSourceProxyLayerReadiness(
	const FMaterialBatchBuildPlan& Plan)
{
	const FMaterialBatchBuildSourceProxyLayerPlan LayerPlan = BuildSourceProxyLayerPlan(Plan);
	FMaterialBatchBuildSourceProxyLayerReadiness Readiness;
	Readiness.LayerBackend = LayerPlan.LayerBackend;
	Readiness.EntryCount = Plan.PlannedEntries.Num();
	Readiness.Assignments.Reserve(Plan.PlannedEntries.Num());

	for (int32 EntryIndex = 0; EntryIndex < Plan.PlannedEntries.Num(); ++EntryIndex)
	{
		const FMaterialBatchBuildPlannedEntry& Entry = Plan.PlannedEntries[EntryIndex];
		const bool bHasSource = HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Source."));
		const bool bHasProxy = HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Proxy."));
		const bool bHasBaked = HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Baked."));
		const bool bHasExclude = Entry.EnvBatchTags.Contains(TEXT("EnvBatch.Exclude"));
		const int32 ActiveLayerTagCount = (bHasSource ? 1 : 0) + (bHasProxy ? 1 : 0) + (bHasBaked ? 1 : 0);

		FMaterialBatchBuildSourceProxyLayerAssignment Assignment;
		Assignment.SourceEntryIndex = EntryIndex;
		Assignment.ActorName = Entry.ActorName;
		Assignment.ComponentName = Entry.ComponentName;
		Assignment.EnvBatchTags = Entry.EnvBatchTags;
		Assignment.ActualLayerNames = Entry.ActualLayerNames;
		Assignment.ActualStreamingLevelName = Entry.ActualStreamingLevelName;
		Assignment.ActualLevelPackageName = Entry.ActualLevelPackageName;
		Assignment.ActualDataLayerNames = Entry.ActualDataLayerNames;
		Assignment.bHasActualLayerEvidence = !Assignment.ActualLayerNames.IsEmpty();
		Assignment.bHasActualDataLayerEvidence = !Entry.ActualDataLayerNames.IsEmpty();

		if (bHasExclude)
		{
			Assignment.LayerRole = TEXT("Excluded");
			Assignment.ExpectedLayerName = TEXT("");
			Assignment.bReadyForLayerValidation = true;
			Assignment.ReadinessReason = TEXT("Excluded from batch layer validation.");
			Assignment.LayerValidationStatus = TEXT("NotRequiredExcluded");
			++Readiness.ExcludedEntryCount;
			++Readiness.ReadyEntryCount;
			++Readiness.NotRequiredActualLayerCount;
		}
		else if (ActiveLayerTagCount > 1)
		{
			Assignment.LayerRole = TEXT("Conflict");
			Assignment.ExpectedLayerName = TEXT("");
			Assignment.bReadyForLayerValidation = false;
			Assignment.ReadinessReason = TEXT("Multiple Source/Proxy/Baked tags are present; fix tags before layer validation.");
			Assignment.LayerValidationStatus = TEXT("NotRequiredTagConflict");
			++Readiness.ConflictEntryCount;
			++Readiness.NotRequiredActualLayerCount;
		}
		else if (bHasSource)
		{
			Assignment.LayerRole = TEXT("Source");
			Assignment.ExpectedLayerName = LayerPlan.LayerBackend.Equals(TEXT("StreamingLevel"), ESearchCase::IgnoreCase)
				&& !Entry.ActualStreamingLevelName.IsEmpty()
				? Entry.ActualStreamingLevelName
				: LayerPlan.SourceLayerName;
			Assignment.bReadyForLayerValidation = true;
			Assignment.ReadinessReason = TEXT("Ready to validate against Source layer.");
			++Readiness.ReadyEntryCount;
		}
		else if (bHasProxy)
		{
			Assignment.LayerRole = TEXT("Proxy");
			Assignment.ExpectedLayerName = LayerPlan.ProxyLayerName;
			Assignment.bReadyForLayerValidation = true;
			Assignment.ReadinessReason = TEXT("Ready to validate against Proxy layer.");
			++Readiness.ReadyEntryCount;
		}
		else if (bHasBaked)
		{
			Assignment.LayerRole = TEXT("Baked");
			Assignment.ExpectedLayerName = LayerPlan.BakedLayerName;
			Assignment.bReadyForLayerValidation = true;
			Assignment.ReadinessReason = TEXT("Ready to validate against Baked layer.");
			++Readiness.ReadyEntryCount;
		}
		else
		{
			Assignment.LayerRole = TEXT("Unassigned");
			Assignment.ExpectedLayerName = TEXT("");
			Assignment.bReadyForLayerValidation = false;
			Assignment.ReadinessReason = TEXT("Missing EnvBatch.Source.*, EnvBatch.Proxy.*, or EnvBatch.Baked.* tag.");
			Assignment.LayerValidationStatus = TEXT("NotRequiredMissingLayerTag");
			++Readiness.MissingLayerTagEntryCount;
			++Readiness.NotRequiredActualLayerCount;
		}

		if (Assignment.bReadyForLayerValidation && !Assignment.ExpectedLayerName.IsEmpty())
		{
			if (!Assignment.bHasActualLayerEvidence)
			{
				Assignment.LayerValidationStatus = TEXT("MissingActualLayer");
				++Readiness.MissingActualLayerCount;
			}
			else if (ContainsStringExact(Assignment.ActualLayerNames, Assignment.ExpectedLayerName))
			{
				Assignment.bMatchesExpectedLayer = true;
				Assignment.LayerValidationStatus = TEXT("MatchedExpectedLayer");
				++Readiness.ActualLayerMatchCount;
			}
			else
			{
				Assignment.LayerValidationStatus = TEXT("UnexpectedActualLayer");
				++Readiness.UnexpectedActualLayerCount;
			}
		}
		Assignment.bMatchesExpectedDataLayer = Assignment.bMatchesExpectedLayer
			&& LayerPlan.LayerBackend.Equals(TEXT("DataLayer"), ESearchCase::IgnoreCase);
		Assignment.DataLayerValidationStatus = Assignment.LayerValidationStatus;

		Readiness.Assignments.Add(Assignment);
	}

	return Readiness;
}

FMaterialBatchBuildSourceProxyAssetReadiness FMaterialBatchBuildPlanBuilder::BuildSourceProxyAssetReadiness(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildSourceProxyAssetReadiness Readiness;
	Readiness.EntryCount = Plan.PlannedEntries.Num();
	Readiness.Assignments.Reserve(Plan.PlannedEntries.Num());

	for (int32 EntryIndex = 0; EntryIndex < Plan.PlannedEntries.Num(); ++EntryIndex)
	{
		const FMaterialBatchBuildPlannedEntry& Entry = Plan.PlannedEntries[EntryIndex];
		const bool bHasSourceTag = HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Source."));
		const bool bHasProxyTag = HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Proxy."));
		const bool bHasBakedTag = HasStringPrefix(Entry.EnvBatchTags, TEXT("EnvBatch.Baked."));
		const bool bHasExcludeTag = Entry.EnvBatchTags.Contains(TEXT("EnvBatch.Exclude"));
		const int32 ActiveLayerTagCount = (bHasSourceTag ? 1 : 0) + (bHasProxyTag ? 1 : 0) + (bHasBakedTag ? 1 : 0);

		FMaterialBatchBuildSourceProxyAssetAssignment Assignment;
		Assignment.SourceEntryIndex = EntryIndex;
		Assignment.ActorName = Entry.ActorName;
		Assignment.ComponentName = Entry.ComponentName;
		Assignment.SourceLODIndex = 0;
		Assignment.ProxyLODIndex = 1;
		Assignment.EnvBatchTags = Entry.EnvBatchTags;

		if (bHasExcludeTag)
		{
			Assignment.LayerRole = TEXT("Excluded");
			Assignment.ReadinessStatus = TEXT("NotRequiredExcluded");
			Assignment.ReadinessReason = TEXT("Excluded actors do not require Source/Proxy asset pairing.");
			Assignment.bReadyForAssetPairing = true;
			++Readiness.NotRequiredCount;
		}
		else if (ActiveLayerTagCount > 1)
		{
			Assignment.LayerRole = TEXT("Conflict");
			Assignment.ReadinessStatus = TEXT("BlockedByLayerTagConflict");
			Assignment.ReadinessReason = TEXT("Multiple Source/Proxy/Baked tags are present; fix tags before asset pairing.");
			++Readiness.ConflictCount;
		}
		else if (bHasSourceTag)
		{
			Assignment.LayerRole = TEXT("Source");
			Assignment.SourceAssetPath = Entry.AssetPath;
			Assignment.ProxyAssetPath = Plan.ProxyMeshPackage;
			Assignment.bHasSourceAsset = !Assignment.SourceAssetPath.IsEmpty();
			Assignment.bHasProxyAsset = !Assignment.ProxyAssetPath.IsEmpty();
			Assignment.bUsesGeneratedProxy = true;
			Assignment.bReadyForAssetPairing = Assignment.bHasSourceAsset && Assignment.bHasProxyAsset;
			Assignment.ReadinessStatus = Assignment.bReadyForAssetPairing
				? TEXT("ReadyGeneratedProxy")
				: TEXT("MissingGeneratedProxyPair");
			Assignment.ReadinessReason = Assignment.bReadyForAssetPairing
				? TEXT("Source defaults to LOD0 and generated cluster proxy defaults to LOD1 for asset pairing.")
				: TEXT("Source entries require a source mesh and planned generated proxy package.");
			++Readiness.GeneratedProxyFallbackCount;
			if (Assignment.bReadyForAssetPairing)
			{
				++Readiness.ReadyPairCount;
			}
			else
			{
				if (!Assignment.bHasSourceAsset)
				{
					++Readiness.MissingSourceAssetCount;
				}
				if (!Assignment.bHasProxyAsset)
				{
					++Readiness.MissingProxyAssetCount;
				}
			}
		}
		else if (bHasProxyTag)
		{
			Assignment.LayerRole = TEXT("Proxy");
			Assignment.ProxyAssetPath = Entry.AssetPath;
			Assignment.bHasProxyAsset = !Assignment.ProxyAssetPath.IsEmpty();
			Assignment.bUsesAuthoredProxy = true;
			Assignment.bReadyForAssetPairing = false;
			Assignment.ReadinessStatus = TEXT("MissingSourceAssetReference");
			Assignment.ReadinessReason = TEXT("Authored proxy assets must be paired with an explicit source asset reference by import settings or the art asset manager.");
			++Readiness.AuthoredProxyCount;
			++Readiness.MissingSourceAssetCount;
			if (!Assignment.bHasProxyAsset)
			{
				++Readiness.MissingProxyAssetCount;
			}
		}
		else if (bHasBakedTag)
		{
			Assignment.LayerRole = TEXT("Baked");
			Assignment.ReadinessStatus = TEXT("NotRequiredBaked");
			Assignment.ReadinessReason = TEXT("Baked entries are validated by bake manifest and Source/Proxy/Baked layer readiness.");
			Assignment.bReadyForAssetPairing = true;
			++Readiness.NotRequiredCount;
		}
		else
		{
			Assignment.LayerRole = TEXT("Unassigned");
			Assignment.ReadinessStatus = TEXT("MissingLayerTag");
			Assignment.ReadinessReason = TEXT("Missing EnvBatch.Source.*, EnvBatch.Proxy.*, or EnvBatch.Baked.* tag.");
			++Readiness.NotRequiredCount;
		}

		Readiness.Assignments.Add(Assignment);
	}

	return Readiness;
}

FMaterialBatchBuildSourceProxyAssetConfigSet FMaterialBatchBuildPlanBuilder::BuildSourceProxyAssetConfigSet(
	const FMaterialBatchBuildPlan& Plan)
{
	const FMaterialBatchBuildSourceProxyAssetReadiness Readiness = BuildSourceProxyAssetReadiness(Plan);

	FMaterialBatchBuildSourceProxyAssetConfigSet ConfigSet;
	ConfigSet.ConfigCount = Readiness.Assignments.Num();
	ConfigSet.Configs.Reserve(Readiness.Assignments.Num());

	for (const FMaterialBatchBuildSourceProxyAssetAssignment& Assignment : Readiness.Assignments)
	{
		FMaterialBatchBuildSourceProxyAssetConfig Config;
		Config.ActorName = Assignment.ActorName;
		Config.ComponentName = Assignment.ComponentName;
		Config.LayerRole = Assignment.LayerRole;
		Config.SourceAssetPath = Assignment.SourceAssetPath;
		Config.SourceLODIndex = Assignment.SourceLODIndex;
		Config.ProxyLODIndex = Assignment.ProxyLODIndex;
		Config.SurfaceKind = Plan.SurfaceKind;
		Config.BakePolicy = Plan.BakePolicy;
		Config.bReadyForAssetPairing = Assignment.bReadyForAssetPairing;
		Config.ReadinessStatus = Assignment.ReadinessStatus;
		Config.ReadinessReason = Assignment.ReadinessReason;

		if (!Config.ActorName.IsEmpty() || !Config.ComponentName.IsEmpty())
		{
			Config.ObjectKey = FString::Printf(TEXT("%s::%s"), *Config.ActorName, *Config.ComponentName);
		}
		else
		{
			Config.ObjectKey = FString::Printf(TEXT("Entry_%d"), Assignment.SourceEntryIndex);
		}

		if (Assignment.bUsesGeneratedProxy)
		{
			Config.GeneratedProxyAssetPath = Assignment.ProxyAssetPath;
			Config.ConfigSource = TEXT("GeneratedFallback");
			Config.InteractionPolicy = TEXT("StaticBatchCandidate");
			Config.bUsesGeneratedProxyFallback = true;
			++ConfigSet.GeneratedFallbackConfigCount;
		}
		else if (Assignment.bUsesAuthoredProxy)
		{
			Config.AuthorProxyAssetPath = Assignment.ProxyAssetPath;
			Config.ConfigSource = TEXT("ImportSettingsOrArtAssetManagerRequired");
			Config.InteractionPolicy = TEXT("StaticBatchCandidate");
			++ConfigSet.AuthoredProxyConfigCount;
		}
		else if (Assignment.LayerRole.Equals(TEXT("Baked"), ESearchCase::IgnoreCase))
		{
			Config.ConfigSource = TEXT("BakeManifest");
			Config.InteractionPolicy = TEXT("BakedStaticReplacement");
		}
		else if (Assignment.LayerRole.Equals(TEXT("Excluded"), ESearchCase::IgnoreCase))
		{
			Config.ConfigSource = TEXT("EnvBatchExclude");
			Config.InteractionPolicy = TEXT("ExcludedFromBatch");
		}
		else if (Assignment.LayerRole.Equals(TEXT("Conflict"), ESearchCase::IgnoreCase))
		{
			Config.ConfigSource = TEXT("InvalidEnvBatchTags");
			Config.InteractionPolicy = TEXT("BlockedUntilTagsFixed");
		}
		else
		{
			Config.ConfigSource = TEXT("UnassignedEnvBatchTags");
			Config.InteractionPolicy = TEXT("RequiresArtReview");
		}

		if (Config.bReadyForAssetPairing)
		{
			++ConfigSet.ReadyConfigCount;
		}
		if (Config.ReadinessStatus.Equals(TEXT("MissingSourceAssetReference"), ESearchCase::IgnoreCase))
		{
			++ConfigSet.MissingSourceReferenceCount;
		}

		ConfigSet.Configs.Add(Config);
	}

	return ConfigSet;
}

FMaterialBatchBuildResidencyRiskPlan FMaterialBatchBuildPlanBuilder::BuildResidencyRiskPlan(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildResidencyRiskPlan RiskPlan;
	RiskPlan.TextureBackend = Plan.TextureBackend;
	RiskPlan.bVTAtlasMainPath = Plan.TextureBackend.Equals(TEXT("VTAtlas"), ESearchCase::IgnoreCase);
	RiskPlan.EstimatedStreamingPoolMB = SumTextureArraySourceMB(Plan);
	RiskPlan.EstimatedVTPoolMB = SumVTAtlasSourceMB(BuildVTAtlasEntries(Plan));
	RiskPlan.EstimatedCombinedPoolMB = RiskPlan.EstimatedStreamingPoolMB + RiskPlan.EstimatedVTPoolMB;
	RiskPlan.bTextureArrayFallbackPresent = RiskPlan.EstimatedStreamingPoolMB > 0.f;
	RiskPlan.bAllowTextureArrayFallbackInProduction = !RiskPlan.bVTAtlasMainPath;
	RiskPlan.bDuplicateResidencyRisk = RiskPlan.bVTAtlasMainPath
		&& RiskPlan.bTextureArrayFallbackPresent
		&& RiskPlan.EstimatedVTPoolMB > 0.f;
	RiskPlan.bRequiresSourceProxyUnload = RiskPlan.bDuplicateResidencyRisk;
	if (RiskPlan.bDuplicateResidencyRisk)
	{
		RiskPlan.ResidencyGate = TEXT("BlockedUntilSourceProxyUnloaded");
		RiskPlan.Recommendation = TEXT("Use VT_Atlas as the only production texture path for the cluster; keep Texture2DArray generation as explicit legacy fallback and unload Source actors before loading Proxy/Baked layers.");
	}
	else if (RiskPlan.bVTAtlasMainPath)
	{
		RiskPlan.ResidencyGate = TEXT("VTAtlasOnly");
		RiskPlan.Recommendation = TEXT("VT_Atlas is the production path; keep non-VT fallback assets unloaded unless an explicit fallback test is running.");
	}
	else
	{
		RiskPlan.ResidencyGate = TEXT("NonVTBackend");
		RiskPlan.Recommendation = TEXT("TextureBackend is not VTAtlas; validate non-VT streaming pool separately.");
	}
	return RiskPlan;
}

void FMaterialBatchBuildPlanBuilder::ApplyPlannedEntries(
	FMaterialBatchBuildPlan& Plan,
	const TArray<FMaterialBatchBuildPlannedEntry>& Entries)
{
	Plan.PlannedEntries = Entries;
	Plan.PlannedMaterialRows.Reset();
	Plan.NextBatchMaterialIndex = 0;

	for (int32 EntryIndex = 0; EntryIndex < Plan.PlannedEntries.Num(); ++EntryIndex)
	{
		FMaterialBatchBuildPlannedEntry& Entry = Plan.PlannedEntries[EntryIndex];
		if (Entry.bCandidate)
		{
			Entry.FirstBatchMaterialIndex = Plan.NextBatchMaterialIndex;
			Entry.BatchMaterialIndexCount = FMath::Max(1, Entry.MaterialSlotCount);
			Plan.NextBatchMaterialIndex += Entry.BatchMaterialIndexCount;
			if (Entry.RejectReason.IsEmpty())
			{
				Entry.RejectReason = TEXT("None");
			}

			for (int32 MaterialSlotIndex = 0; MaterialSlotIndex < Entry.BatchMaterialIndexCount; ++MaterialSlotIndex)
			{
				FMaterialBatchBuildMaterialRow Row;
				Row.BatchMaterialIndex = Entry.FirstBatchMaterialIndex + MaterialSlotIndex;
				Row.SourceEntryIndex = EntryIndex;
				Row.MaterialSlotIndex = MaterialSlotIndex;
				Row.SourceKind = Entry.SourceKind;
				Row.SourcePath = Entry.SourcePath;
				Row.ActorName = Entry.ActorName;
				Row.ComponentName = Entry.ComponentName;
				Row.AssetPath = Entry.AssetPath;
				Row.MaterialSlotName = Entry.MaterialSlotNames.IsValidIndex(MaterialSlotIndex)
					? Entry.MaterialSlotNames[MaterialSlotIndex]
					: FString::Printf(TEXT("MaterialSlot_%d"), MaterialSlotIndex);
				Row.MaterialPath = Entry.MaterialPaths.IsValidIndex(MaterialSlotIndex)
					? Entry.MaterialPaths[MaterialSlotIndex]
					: TEXT("(no material)");
				Plan.PlannedMaterialRows.Add(Row);
			}
		}
		else
		{
			Entry.FirstBatchMaterialIndex = INDEX_NONE;
			Entry.BatchMaterialIndexCount = 0;
			if (Entry.RejectReason.IsEmpty())
			{
				Entry.RejectReason = TEXT("Rejected");
			}
		}
	}
	Plan.SourceProxyLayerPlan = BuildSourceProxyLayerPlan(Plan);
	Plan.SourceProxyLayerReadiness = BuildSourceProxyLayerReadiness(Plan);
	Plan.SourceProxyAssetReadiness = BuildSourceProxyAssetReadiness(Plan);
	Plan.SourceProxyAssetConfigSet = BuildSourceProxyAssetConfigSet(Plan);
	Plan.ResidencyRiskPlan = BuildResidencyRiskPlan(Plan);
}

void FMaterialBatchBuildPlanBuilder::ApplyTextureChannelPlans(FMaterialBatchBuildPlan& Plan)
{
	for (FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		Row.TextureChannels.Reset();
		if (Row.MaterialPath.IsEmpty() || Row.MaterialPath == TEXT("(no material)"))
		{
			continue;
		}

		UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *Row.MaterialPath);
		if (!Material)
		{
			continue;
		}

		TArray<FMaterialParameterInfo> TextureParameterInfos;
		TArray<FGuid> TextureParameterIds;
		Material->GetAllTextureParameterInfo(TextureParameterInfos, TextureParameterIds);
		TextureParameterInfos.Sort([](const FMaterialParameterInfo& Left, const FMaterialParameterInfo& Right)
		{
			return Left.ToString() < Right.ToString();
		});

		for (const FMaterialParameterInfo& ParameterInfo : TextureParameterInfos)
		{
			UTexture* Texture = nullptr;
			const bool bFoundTexture = Material->GetTextureParameterValue(FHashedMaterialParameterInfo(ParameterInfo), Texture);

			FMaterialBatchBuildTextureChannelPlan ChannelPlan;
			ChannelPlan.ChannelName = ClassifyTextureChannelName(ParameterInfo.Name.ToString());
			ChannelPlan.ParameterName = ParameterInfo.Name.ToString();
			ChannelPlan.TexturePath = GetObjectPath(Texture);
			ChannelPlan.TextureClass = GetObjectClassName(Texture);
			ChannelPlan.bFoundTexture = bFoundTexture && Texture != nullptr;
			if (const UTexture2D* Texture2D = Cast<UTexture2D>(Texture))
			{
				const FIntPoint TextureDimensions = GetTexture2DDimensions(Texture2D);
				ChannelPlan.TextureWidth = TextureDimensions.X;
				ChannelPlan.TextureHeight = TextureDimensions.Y;
			}
			Row.TextureChannels.Add(WithEvaluatedTextureArrayEligibility(ChannelPlan));
		}
	}
	Plan.ResidencyRiskPlan = BuildResidencyRiskPlan(Plan);
}

FString FMaterialBatchBuildPlanBuilder::ClassifyTextureChannelName(const FString& ParameterName)
{
	FString Normalized = ParameterName;
	Normalized.ToLowerInline();

	if (ContainsAnyToken(Normalized, {
		TEXT("lightinfo"),
		TEXT("light_info")
	}))
	{
		return TEXT("LightInfo");
	}

	if (ContainsAnyToken(Normalized, {
		TEXT("basecolor"),
		TEXT("base_color"),
		TEXT("basemap"),
		TEXT("_basemap"),
		TEXT("_bc"),
		TEXT("albedo"),
		TEXT("_albedo"),
		TEXT("diffuse"),
		TEXT("_diffuse"),
		TEXT("_maintex"),
		TEXT("color map"),
		TEXT("t_color"),
		TEXT("_color"),
		TEXT("array_a"),
		TEXT("t_array_a")
	}))
	{
		return TEXT("BaseColor");
	}

	if (ContainsAnyToken(Normalized, {
		TEXT("normal"),
		TEXT("bump"),
		TEXT("t_normal"),
		TEXT("array_n"),
		TEXT("t_array_n")
	}))
	{
		return TEXT("Normal");
	}

	if (ContainsAnyToken(Normalized, {
		TEXT("packedorm"),
		TEXT("orm"),
		TEXT("mra"),
		TEXT("t_mra"),
		TEXT("occlusionroughnessmetallic"),
		TEXT("maskmap"),
		TEXT("_maskmap"),
		TEXT("array_m"),
		TEXT("t_array_m")
	}))
	{
		return TEXT("ORM");
	}

	if (ContainsAnyToken(Normalized, {
		TEXT("emissive"),
		TEXT("emission")
	}))
	{
		return TEXT("Emissive");
	}

	if (ContainsAnyToken(Normalized, {
		TEXT("opacitymask"),
		TEXT("t_mask"),
		TEXT("mask"),
		TEXT("alpha")
	}))
	{
		return TEXT("Mask");
	}

	return TEXT("Unknown");
}

TArray<FMaterialBatchBuildTextureArrayPayload> FMaterialBatchBuildPlanBuilder::BuildTextureArrayPayloads(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildTextureArrayPlans TextureArrayPlans;
	BuildPropertyRowPlans(Plan.PlannedMaterialRows, TextureArrayPlans);

	TMap<FString, FIntPoint> TextureDimensionsByPath;
	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			const FMaterialBatchBuildTextureChannelPlan EvaluatedChannel =
				WithEvaluatedTextureArrayEligibility(Channel);
			if (EvaluatedChannel.bTextureArrayBuildEligible && !EvaluatedChannel.TexturePath.IsEmpty())
			{
				TextureDimensionsByPath.FindOrAdd(EvaluatedChannel.TexturePath) = FIntPoint(
					EvaluatedChannel.TextureWidth,
					EvaluatedChannel.TextureHeight);
			}
		}
	}

	TArray<FMaterialBatchBuildTextureArrayPayload> Payloads;
	auto AppendPayload = [&Payloads, &TextureDimensionsByPath](
		const FString& ChannelName,
		const FString& PackagePath,
		const TArray<FMaterialBatchBuildTextureArraySlicePlan>& SlicePlans)
	{
		if (SlicePlans.IsEmpty())
		{
			return;
		}

		FMaterialBatchBuildTextureArrayPayload Payload;
		Payload.ChannelName = ChannelName;
		Payload.PackagePath = PackagePath;
		for (const FMaterialBatchBuildTextureArraySlicePlan& SlicePlan : SlicePlans)
		{
			Payload.SourceTexturePaths.Add(SlicePlan.TexturePath);
			if (const FIntPoint* Dimensions = TextureDimensionsByPath.Find(SlicePlan.TexturePath))
			{
				Payload.Width = FMath::Max(Payload.Width, Dimensions->X);
				Payload.Height = FMath::Max(Payload.Height, Dimensions->Y);
			}
		}
		Payloads.Add(Payload);
	};

	AppendPayload(TEXT("BaseColor"), Plan.BaseColorArrayPackage, TextureArrayPlans.BaseColor);
	AppendPayload(TEXT("Normal"), Plan.NormalArrayPackage, TextureArrayPlans.Normal);
	AppendPayload(TEXT("ORM"), Plan.OrmArrayPackage, TextureArrayPlans.Orm);
	AppendPayload(TEXT("Emissive"), Plan.EmissiveArrayPackage, TextureArrayPlans.Emissive);
	AppendPayload(TEXT("Mask"), Plan.MaskArrayPackage, TextureArrayPlans.Mask);
	return Payloads;
}

TArray<FMaterialBatchBuildVTAtlasEntry> FMaterialBatchBuildPlanBuilder::BuildVTAtlasEntries(
	const FMaterialBatchBuildPlan& Plan)
{
	TArray<FMaterialBatchBuildVTAtlasEntry> Entries;
	TSet<FString> SeenKeys;

	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			const FMaterialBatchBuildTextureChannelPlan EvaluatedChannel =
				WithEvaluatedTextureArrayEligibility(Channel);
			if (!EvaluatedChannel.bTextureArrayBuildEligible || EvaluatedChannel.TexturePath.IsEmpty())
			{
				continue;
			}

			const FString Key = FString::Printf(
				TEXT("%s|%s"),
				*EvaluatedChannel.ChannelName,
				*EvaluatedChannel.TexturePath);
			if (SeenKeys.Contains(Key))
			{
				continue;
			}
			SeenKeys.Add(Key);

			FMaterialBatchBuildVTAtlasEntry Entry;
			Entry.AtlasEntryIndex = Entries.Num();
			Entry.ChannelName = EvaluatedChannel.ChannelName;
			Entry.TexturePath = EvaluatedChannel.TexturePath;
			Entry.TextureClass = EvaluatedChannel.TextureClass;
			Entry.Width = EvaluatedChannel.TextureWidth;
			Entry.Height = EvaluatedChannel.TextureHeight;
			Entry.EstimatedSourceMB = EstimateTextureSourceMB(Entry.Width, Entry.Height);
			Entries.Add(Entry);
		}
	}

	const int32 EntryCount = Entries.Num();
	if (EntryCount <= 0)
	{
		return Entries;
	}

	const int32 ColumnCount = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(EntryCount))));
	const int32 RowCount = FMath::Max(1, FMath::CeilToInt(static_cast<float>(EntryCount) / static_cast<float>(ColumnCount)));
	for (FMaterialBatchBuildVTAtlasEntry& Entry : Entries)
	{
		const int32 ColumnIndex = Entry.AtlasEntryIndex % ColumnCount;
		const int32 RowIndex = Entry.AtlasEntryIndex / ColumnCount;
		Entry.VirtualTextureLayout = TEXT("UDIMStyleGrid");
		Entry.TileU = ColumnIndex;
		Entry.TileV = RowIndex;
		Entry.UdimNumber = 1001 + Entry.TileU + (Entry.TileV * 10);
		Entry.TilePaddingPixels = 8;
		Entry.UVRemapStatus = TEXT("PlannedForMergedProxyUVRemap");
		Entry.UVRectMin = FVector2D(
			static_cast<float>(ColumnIndex) / static_cast<float>(ColumnCount),
			static_cast<float>(RowIndex) / static_cast<float>(RowCount));
		Entry.UVRectMax = FVector2D(
			static_cast<float>(ColumnIndex + 1) / static_cast<float>(ColumnCount),
			static_cast<float>(RowIndex + 1) / static_cast<float>(RowCount));
	}

	return Entries;
}

FMaterialBatchBuildVTAtlasPayload FMaterialBatchBuildPlanBuilder::BuildVTAtlasPayload(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildVTAtlasPayload Payload;
	Payload.PackagePath = Plan.VTAtlasPackage;
	Payload.VirtualTextureLayout = TEXT("UDIMStyleGrid");
	Payload.TilePaddingPixels = 8;
	Payload.Entries = BuildVTAtlasEntries(Plan);
	const int32 EntryCount = Payload.Entries.Num();
	if (EntryCount <= 0)
	{
		return Payload;
	}

	Payload.Columns = FMath::Max(1, FMath::CeilToInt(FMath::Sqrt(static_cast<float>(EntryCount))));
	Payload.Rows = FMath::Max(1, FMath::CeilToInt(static_cast<float>(EntryCount) / static_cast<float>(Payload.Columns)));

	int32 CellWidth = 1;
	int32 CellHeight = 1;
	for (const FMaterialBatchBuildVTAtlasEntry& Entry : Payload.Entries)
	{
		CellWidth = FMath::Max(CellWidth, Entry.Width);
		CellHeight = FMath::Max(CellHeight, Entry.Height);
	}
	Payload.Width = CellWidth * Payload.Columns;
	Payload.Height = CellHeight * Payload.Rows;
	return Payload;
}

FMaterialBatchBuildPropertyTexturePayload FMaterialBatchBuildPlanBuilder::BuildPropertyTexturePayload(
	const FMaterialBatchBuildPlan& Plan)
{
	const TArray<FMaterialBatchBuildPropertyTextureColumnPlan> ColumnPlans = GetPropertyTextureColumnPlans();
	FMaterialBatchBuildTextureArrayPlans TextureArrayPlans;
	TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows =
		BuildPropertyRowPlans(Plan.PlannedMaterialRows, TextureArrayPlans);
	ApplyVTAtlasRectsToPropertyRows(BuildVTAtlasEntries(Plan), TextureArrayPlans, PropertyRows);

	FMaterialBatchBuildPropertyTexturePayload Payload;
	Payload.Width = ColumnPlans.Num();
	Payload.Height = PropertyRows.Num();
	Payload.bSRGB = false;
	Payload.SourceFormat = ETextureSourceFormat::TSF_RGBA16F;
	Payload.Pixels.SetNum(Payload.Width * Payload.Height);

	auto GetColumnValue = [](const FMaterialBatchBuildPropertyRowPlan& Row, const FString& SourceField) -> float
	{
		if (SourceField == TEXT("baseColorSlice"))
		{
			return static_cast<float>(Row.BaseColorSliceIndex);
		}
		if (SourceField == TEXT("normalSlice"))
		{
			return static_cast<float>(Row.NormalSliceIndex);
		}
		if (SourceField == TEXT("ormSlice"))
		{
			return static_cast<float>(Row.OrmSliceIndex);
		}
		if (SourceField == TEXT("emissiveSlice"))
		{
			return static_cast<float>(Row.EmissiveSliceIndex);
		}
		if (SourceField == TEXT("maskSlice"))
		{
			return static_cast<float>(Row.MaskSliceIndex);
		}
		if (SourceField == TEXT("baseVTUVMinX"))
		{
			return Row.UVRectMin.X;
		}
		if (SourceField == TEXT("baseVTUVMinY"))
		{
			return Row.UVRectMin.Y;
		}
		if (SourceField == TEXT("baseVTUVMaxX"))
		{
			return Row.UVRectMax.X;
		}
		if (SourceField == TEXT("baseVTUVMaxY"))
		{
			return Row.UVRectMax.Y;
		}
		if (SourceField == TEXT("normalVTUVMinX"))
		{
			return Row.NormalUVRectMin.X;
		}
		if (SourceField == TEXT("normalVTUVMinY"))
		{
			return Row.NormalUVRectMin.Y;
		}
		if (SourceField == TEXT("normalVTUVMaxX"))
		{
			return Row.NormalUVRectMax.X;
		}
		if (SourceField == TEXT("normalVTUVMaxY"))
		{
			return Row.NormalUVRectMax.Y;
		}
		if (SourceField == TEXT("ormVTUVMinX"))
		{
			return Row.OrmUVRectMin.X;
		}
		if (SourceField == TEXT("ormVTUVMinY"))
		{
			return Row.OrmUVRectMin.Y;
		}
		if (SourceField == TEXT("ormVTUVMaxX"))
		{
			return Row.OrmUVRectMax.X;
		}
		if (SourceField == TEXT("ormVTUVMaxY"))
		{
			return Row.OrmUVRectMax.Y;
		}
		return static_cast<float>(INDEX_NONE);
	};

	for (int32 RowIndex = 0; RowIndex < PropertyRows.Num(); ++RowIndex)
	{
		const FMaterialBatchBuildPropertyRowPlan& PropertyRow = PropertyRows[RowIndex];
		for (int32 ColumnIndex = 0; ColumnIndex < ColumnPlans.Num(); ++ColumnIndex)
		{
			const int32 PixelIndex = RowIndex * Payload.Width + ColumnIndex;
			const float PropertyValue = GetColumnValue(PropertyRow, ColumnPlans[ColumnIndex].SourceField);
			Payload.Pixels[PixelIndex] = FFloat16Color(FLinearColor(PropertyValue, 0.0f, 0.0f, 0.0f));
		}
	}

	return Payload;
}

FMaterialBatchBuildProxyMeshPayload FMaterialBatchBuildPlanBuilder::BuildProxyMeshPayload(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildProxyMeshPayload Payload;
	Payload.PackagePath = Plan.ProxyMeshPackage;
	Payload.MaterialIndexChannel = TEXT("TexCoord7.x");

	const TArray<FMaterialBatchBuildGeometryMergeSourcePlan> GeometrySources =
		BuildGeometryMergeSourcePlans(Plan.PlannedEntries);
	Payload.Sources.Reserve(GeometrySources.Num());
	for (const FMaterialBatchBuildGeometryMergeSourcePlan& GeometrySource : GeometrySources)
	{
		FMaterialBatchBuildProxyMeshSourcePayload SourcePayload;
		SourcePayload.SourceEntryIndex = GeometrySource.SourceEntryIndex;
		SourcePayload.ActorName = GeometrySource.ActorName;
		SourcePayload.ComponentName = GeometrySource.ComponentName;
		SourcePayload.StaticMeshPath = GeometrySource.StaticMeshPath;
		SourcePayload.bBakeInstanceVertexColors = GeometrySource.bBakeInstanceVertexColors;
		SourcePayload.bHasWorldTransform = GeometrySource.bHasWorldTransform;
		SourcePayload.WorldLocation = GeometrySource.WorldLocation;
		SourcePayload.WorldRotation = GeometrySource.WorldRotation;
		SourcePayload.WorldScale = GeometrySource.WorldScale;
		SourcePayload.MaterialSlotToBatchMaterialIndex = GeometrySource.MaterialSlotToBatchMaterialIndex;
		Payload.Sources.Add(SourcePayload);
	}

	return Payload;
}

FMaterialBatchBuildBatchMaterialPayload FMaterialBatchBuildPlanBuilder::BuildBatchMaterialPayload(
	const FMaterialBatchBuildPlan& Plan)
{
	FMaterialBatchBuildBatchMaterialPayload Payload;
	Payload.PackagePath = Plan.BatchMaterialInstancePackage;
	Payload.ParentMaterialPath = Plan.BatchParentMaterialPackage;
	if (Plan.TextureBackend.Equals(TEXT("VTAtlas"), ESearchCase::IgnoreCase))
	{
		Payload.TextureBindings = {
			{ TEXT("VT_Atlas"), Plan.VTAtlasPackage },
			{ Plan.BatchPropertyTextureParameterName, Plan.PropertyTexturePackage }
		};
	}
	else
	{
		Payload.TextureBindings = {
			{ TEXT("T_Array_A"), Plan.BaseColorArrayPackage },
			{ TEXT("T_Array_N"), Plan.NormalArrayPackage },
			{ TEXT("T_Array_M"), Plan.OrmArrayPackage },
			{ Plan.BatchPropertyTextureParameterName, Plan.PropertyTexturePackage }
		};
	}
	Payload.ScalarBindings = {
		{ TEXT("BatchRowCount"), static_cast<float>(FMath::Max(Plan.PlannedMaterialRows.Num(), 1)) },
		{ TEXT("PropertyColumnCount"), static_cast<float>(GetPropertyTextureColumnPlans().Num()) }
	};
	return Payload;
}

void FMaterialBatchBuildPlanBuilder::PopulateMappingDataAsset(
	const FMaterialBatchBuildPlan& Plan,
	UMaterialBatchMappingDataAsset& MappingData)
{
	MappingData.Schema = TEXT("DevKit.MaterialBatchMappingData.v1");
	MappingData.bDryRunSource = Plan.bDryRun;
	MappingData.RootPath = Plan.Options.RootPath;
	MappingData.MapPath = Plan.Options.MapPath;
	MappingData.DataLayerName = Plan.Options.DataLayerName;
	MappingData.LayerBackend = NormalizeLayerBackend(Plan.Options.LayerBackend);
	MappingData.ClusterName = Plan.SanitizedClusterName;
	MappingData.TierName = Plan.SanitizedTierName;
	MappingData.OutputFolder = Plan.OutputFolder;
	MappingData.ProxyMeshPackage = Plan.ProxyMeshPackage;
	MappingData.BatchMaterialInstancePackage = Plan.BatchMaterialInstancePackage;
	MappingData.BatchParentMaterialPackage = Plan.BatchParentMaterialPackage;
	MappingData.TextureBackend = Plan.TextureBackend;
	MappingData.VTAtlasPackage = Plan.VTAtlasPackage;
	MappingData.VTAtlasChannel = Plan.VTAtlasChannel;
	MappingData.SurfaceKind = Plan.SurfaceKind;
	MappingData.BakePolicy = Plan.BakePolicy;
	MappingData.SourceProxyExclusivityGroup = Plan.SourceProxyExclusivityGroup;
	const TArray<FMaterialBatchBuildVTAtlasEntry> VTAtlasEntries = BuildVTAtlasEntries(Plan);
	const FMaterialBatchBuildSourceProxyLayerPlan SourceProxyLayerPlan = BuildSourceProxyLayerPlan(Plan);
	const FMaterialBatchBuildSourceProxyLayerReadiness SourceProxyLayerReadiness = BuildSourceProxyLayerReadiness(Plan);
	const FMaterialBatchBuildSourceProxyAssetReadiness SourceProxyAssetReadiness = BuildSourceProxyAssetReadiness(Plan);
	const FMaterialBatchBuildSourceProxyAssetConfigSet SourceProxyAssetConfigSet = BuildSourceProxyAssetConfigSet(Plan);
	const FMaterialBatchBuildResidencyRiskPlan ResidencyRiskPlan = BuildResidencyRiskPlan(Plan);
	MappingData.EstimatedStreamingPoolMB = SumTextureArraySourceMB(Plan);
	MappingData.EstimatedVTPoolMB = SumVTAtlasSourceMB(VTAtlasEntries);
	MappingData.bDuplicateResidencyRisk = Plan.TextureBackend.Equals(TEXT("VTAtlas"), ESearchCase::IgnoreCase)
		&& MappingData.EstimatedStreamingPoolMB > 0.f
		&& MappingData.EstimatedVTPoolMB > 0.f;
	MappingData.ResidencyRiskPlan.TextureBackend = ResidencyRiskPlan.TextureBackend;
	MappingData.ResidencyRiskPlan.bVTAtlasMainPath = ResidencyRiskPlan.bVTAtlasMainPath;
	MappingData.ResidencyRiskPlan.bTextureArrayFallbackPresent = ResidencyRiskPlan.bTextureArrayFallbackPresent;
	MappingData.ResidencyRiskPlan.bAllowTextureArrayFallbackInProduction = ResidencyRiskPlan.bAllowTextureArrayFallbackInProduction;
	MappingData.ResidencyRiskPlan.bDuplicateResidencyRisk = ResidencyRiskPlan.bDuplicateResidencyRisk;
	MappingData.ResidencyRiskPlan.bRequiresSourceProxyUnload = ResidencyRiskPlan.bRequiresSourceProxyUnload;
	MappingData.ResidencyRiskPlan.EstimatedVTPoolMB = ResidencyRiskPlan.EstimatedVTPoolMB;
	MappingData.ResidencyRiskPlan.EstimatedStreamingPoolMB = ResidencyRiskPlan.EstimatedStreamingPoolMB;
	MappingData.ResidencyRiskPlan.EstimatedCombinedPoolMB = ResidencyRiskPlan.EstimatedCombinedPoolMB;
	MappingData.ResidencyRiskPlan.ResidencyGate = ResidencyRiskPlan.ResidencyGate;
	MappingData.ResidencyRiskPlan.Recommendation = ResidencyRiskPlan.Recommendation;
	MappingData.SourceProxyLayerPlan.ExclusivityGroup = SourceProxyLayerPlan.ExclusivityGroup;
	MappingData.SourceProxyLayerPlan.LayerBackend = SourceProxyLayerPlan.LayerBackend;
	MappingData.SourceProxyLayerPlan.SourceLayerName = SourceProxyLayerPlan.SourceLayerName;
	MappingData.SourceProxyLayerPlan.ProxyLayerName = SourceProxyLayerPlan.ProxyLayerName;
	MappingData.SourceProxyLayerPlan.BakedLayerName = SourceProxyLayerPlan.BakedLayerName;
	MappingData.SourceProxyLayerPlan.bRequiresMutualExclusion = SourceProxyLayerPlan.bRequiresMutualExclusion;
	MappingData.SourceProxyLayerPlan.bHasTagConflicts = SourceProxyLayerPlan.bHasTagConflicts;
	MappingData.SourceProxyLayerPlan.SourceActorCount = SourceProxyLayerPlan.SourceActorCount;
	MappingData.SourceProxyLayerPlan.ProxyActorCount = SourceProxyLayerPlan.ProxyActorCount;
	MappingData.SourceProxyLayerPlan.BakedActorCount = SourceProxyLayerPlan.BakedActorCount;
	MappingData.SourceProxyLayerPlan.ConflictActorCount = SourceProxyLayerPlan.ConflictActorCount;
	MappingData.SourceProxyLayerPlan.TierSelections.Reset();
	MappingData.SourceProxyLayerPlan.TierSelections.Reserve(SourceProxyLayerPlan.TierSelections.Num());
	for (const FMaterialBatchBuildTierLayerSelection& Selection : SourceProxyLayerPlan.TierSelections)
	{
		FMaterialBatchMappingTierLayerSelection MappingSelection;
		MappingSelection.TierName = Selection.TierName;
		MappingSelection.bLoadSourceLayer = Selection.bLoadSourceLayer;
		MappingSelection.bLoadProxyLayer = Selection.bLoadProxyLayer;
		MappingSelection.bLoadBakedLayer = Selection.bLoadBakedLayer;
		MappingSelection.FallbackPolicy = Selection.FallbackPolicy;
		MappingData.SourceProxyLayerPlan.TierSelections.Add(MappingSelection);
	}
	MappingData.SourceProxyLayerReadiness.EntryCount = SourceProxyLayerReadiness.EntryCount;
	MappingData.SourceProxyLayerReadiness.LayerBackend = SourceProxyLayerReadiness.LayerBackend;
	MappingData.SourceProxyLayerReadiness.ReadyEntryCount = SourceProxyLayerReadiness.ReadyEntryCount;
	MappingData.SourceProxyLayerReadiness.MissingLayerTagEntryCount = SourceProxyLayerReadiness.MissingLayerTagEntryCount;
	MappingData.SourceProxyLayerReadiness.ConflictEntryCount = SourceProxyLayerReadiness.ConflictEntryCount;
	MappingData.SourceProxyLayerReadiness.ExcludedEntryCount = SourceProxyLayerReadiness.ExcludedEntryCount;
	MappingData.SourceProxyLayerReadiness.ActualLayerMatchCount = SourceProxyLayerReadiness.ActualLayerMatchCount;
	MappingData.SourceProxyLayerReadiness.MissingActualLayerCount = SourceProxyLayerReadiness.MissingActualLayerCount;
	MappingData.SourceProxyLayerReadiness.UnexpectedActualLayerCount = SourceProxyLayerReadiness.UnexpectedActualLayerCount;
	MappingData.SourceProxyLayerReadiness.NotRequiredActualLayerCount = SourceProxyLayerReadiness.NotRequiredActualLayerCount;
	MappingData.SourceProxyLayerReadiness.Assignments.Reset();
	MappingData.SourceProxyLayerReadiness.Assignments.Reserve(SourceProxyLayerReadiness.Assignments.Num());
	for (const FMaterialBatchBuildSourceProxyLayerAssignment& Assignment : SourceProxyLayerReadiness.Assignments)
	{
		FMaterialBatchMappingSourceProxyLayerAssignment MappingAssignment;
		MappingAssignment.SourceEntryIndex = Assignment.SourceEntryIndex;
		MappingAssignment.ActorName = Assignment.ActorName;
		MappingAssignment.ComponentName = Assignment.ComponentName;
		MappingAssignment.LayerRole = Assignment.LayerRole;
		MappingAssignment.ExpectedLayerName = Assignment.ExpectedLayerName;
		MappingAssignment.ActualLayerNames = Assignment.ActualLayerNames;
		MappingAssignment.ActualStreamingLevelName = Assignment.ActualStreamingLevelName;
		MappingAssignment.ActualLevelPackageName = Assignment.ActualLevelPackageName;
		MappingAssignment.ActualDataLayerNames = Assignment.ActualDataLayerNames;
		MappingAssignment.bReadyForLayerValidation = Assignment.bReadyForLayerValidation;
		MappingAssignment.bHasActualLayerEvidence = Assignment.bHasActualLayerEvidence;
		MappingAssignment.bHasActualDataLayerEvidence = Assignment.bHasActualDataLayerEvidence;
		MappingAssignment.bMatchesExpectedLayer = Assignment.bMatchesExpectedLayer;
		MappingAssignment.bMatchesExpectedDataLayer = Assignment.bMatchesExpectedDataLayer;
		MappingAssignment.ReadinessReason = Assignment.ReadinessReason;
		MappingAssignment.LayerValidationStatus = Assignment.LayerValidationStatus;
		MappingAssignment.DataLayerValidationStatus = Assignment.DataLayerValidationStatus;
		MappingAssignment.EnvBatchTags = Assignment.EnvBatchTags;
		MappingData.SourceProxyLayerReadiness.Assignments.Add(MappingAssignment);
	}
	MappingData.SourceProxyAssetReadiness.EntryCount = SourceProxyAssetReadiness.EntryCount;
	MappingData.SourceProxyAssetReadiness.ReadyPairCount = SourceProxyAssetReadiness.ReadyPairCount;
	MappingData.SourceProxyAssetReadiness.MissingSourceAssetCount = SourceProxyAssetReadiness.MissingSourceAssetCount;
	MappingData.SourceProxyAssetReadiness.MissingProxyAssetCount = SourceProxyAssetReadiness.MissingProxyAssetCount;
	MappingData.SourceProxyAssetReadiness.GeneratedProxyFallbackCount = SourceProxyAssetReadiness.GeneratedProxyFallbackCount;
	MappingData.SourceProxyAssetReadiness.AuthoredProxyCount = SourceProxyAssetReadiness.AuthoredProxyCount;
	MappingData.SourceProxyAssetReadiness.NotRequiredCount = SourceProxyAssetReadiness.NotRequiredCount;
	MappingData.SourceProxyAssetReadiness.ConflictCount = SourceProxyAssetReadiness.ConflictCount;
	MappingData.SourceProxyAssetReadiness.Assignments.Reset();
	MappingData.SourceProxyAssetReadiness.Assignments.Reserve(SourceProxyAssetReadiness.Assignments.Num());
	for (const FMaterialBatchBuildSourceProxyAssetAssignment& Assignment : SourceProxyAssetReadiness.Assignments)
	{
		FMaterialBatchMappingSourceProxyAssetAssignment MappingAssignment;
		MappingAssignment.SourceEntryIndex = Assignment.SourceEntryIndex;
		MappingAssignment.ActorName = Assignment.ActorName;
		MappingAssignment.ComponentName = Assignment.ComponentName;
		MappingAssignment.LayerRole = Assignment.LayerRole;
		MappingAssignment.SourceAssetPath = Assignment.SourceAssetPath;
		MappingAssignment.ProxyAssetPath = Assignment.ProxyAssetPath;
		MappingAssignment.SourceLODIndex = Assignment.SourceLODIndex;
		MappingAssignment.ProxyLODIndex = Assignment.ProxyLODIndex;
		MappingAssignment.bHasSourceAsset = Assignment.bHasSourceAsset;
		MappingAssignment.bHasProxyAsset = Assignment.bHasProxyAsset;
		MappingAssignment.bUsesGeneratedProxy = Assignment.bUsesGeneratedProxy;
		MappingAssignment.bUsesAuthoredProxy = Assignment.bUsesAuthoredProxy;
		MappingAssignment.bReadyForAssetPairing = Assignment.bReadyForAssetPairing;
		MappingAssignment.ReadinessStatus = Assignment.ReadinessStatus;
		MappingAssignment.ReadinessReason = Assignment.ReadinessReason;
		MappingAssignment.EnvBatchTags = Assignment.EnvBatchTags;
		MappingData.SourceProxyAssetReadiness.Assignments.Add(MappingAssignment);
	}
	MappingData.SourceProxyAssetConfigSet.ConfigCount = SourceProxyAssetConfigSet.ConfigCount;
	MappingData.SourceProxyAssetConfigSet.ReadyConfigCount = SourceProxyAssetConfigSet.ReadyConfigCount;
	MappingData.SourceProxyAssetConfigSet.GeneratedFallbackConfigCount = SourceProxyAssetConfigSet.GeneratedFallbackConfigCount;
	MappingData.SourceProxyAssetConfigSet.AuthoredProxyConfigCount = SourceProxyAssetConfigSet.AuthoredProxyConfigCount;
	MappingData.SourceProxyAssetConfigSet.MissingSourceReferenceCount = SourceProxyAssetConfigSet.MissingSourceReferenceCount;
	MappingData.SourceProxyAssetConfigSet.Configs.Reset();
	MappingData.SourceProxyAssetConfigSet.Configs.Reserve(SourceProxyAssetConfigSet.Configs.Num());
	for (const FMaterialBatchBuildSourceProxyAssetConfig& Config : SourceProxyAssetConfigSet.Configs)
	{
		FMaterialBatchMappingSourceProxyAssetConfig MappingConfig;
		MappingConfig.ObjectKey = Config.ObjectKey;
		MappingConfig.ActorName = Config.ActorName;
		MappingConfig.ComponentName = Config.ComponentName;
		MappingConfig.LayerRole = Config.LayerRole;
		MappingConfig.SourceAssetPath = Config.SourceAssetPath;
		MappingConfig.AuthorProxyAssetPath = Config.AuthorProxyAssetPath;
		MappingConfig.GeneratedProxyAssetPath = Config.GeneratedProxyAssetPath;
		MappingConfig.SourceLODIndex = Config.SourceLODIndex;
		MappingConfig.ProxyLODIndex = Config.ProxyLODIndex;
		MappingConfig.SurfaceKind = Config.SurfaceKind;
		MappingConfig.BakePolicy = Config.BakePolicy;
		MappingConfig.InteractionPolicy = Config.InteractionPolicy;
		MappingConfig.ConfigSource = Config.ConfigSource;
		MappingConfig.bUsesGeneratedProxyFallback = Config.bUsesGeneratedProxyFallback;
		MappingConfig.bReadyForAssetPairing = Config.bReadyForAssetPairing;
		MappingConfig.ReadinessStatus = Config.ReadinessStatus;
		MappingConfig.ReadinessReason = Config.ReadinessReason;
		MappingData.SourceProxyAssetConfigSet.Configs.Add(MappingConfig);
	}
	MappingData.BaseColorArrayPackage = Plan.BaseColorArrayPackage;
	MappingData.NormalArrayPackage = Plan.NormalArrayPackage;
	MappingData.OrmArrayPackage = Plan.OrmArrayPackage;
	MappingData.EmissiveArrayPackage = Plan.EmissiveArrayPackage;
	MappingData.MaskArrayPackage = Plan.MaskArrayPackage;
	MappingData.PropertyTexturePackage = Plan.PropertyTexturePackage;
	MappingData.MappingDataAssetPackage = Plan.MappingDataAssetPackage;
	MappingData.SourceCoordinateSpace = TEXT("World");
	MappingData.MaterialIndexChannel = Plan.BatchMaterialIndexChannel;
	MappingData.MaterialIndexEncoding = TEXT("write batchMaterialIndex as a float per merged vertex");
	MappingData.PropertyTextureParameterName = Plan.BatchPropertyTextureParameterName;
	MappingData.PropertyTextureRowEncoding = Plan.BatchPropertyTextureRowEncoding;

	MappingData.MaterialRows.Reset();
	MappingData.MaterialRows.Reserve(Plan.PlannedMaterialRows.Num());
	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		FMaterialBatchMappingMaterialRow MappingRow;
		MappingRow.BatchMaterialIndex = Row.BatchMaterialIndex;
		MappingRow.SourceEntryIndex = Row.SourceEntryIndex;
		MappingRow.MaterialSlotIndex = Row.MaterialSlotIndex;
		MappingRow.SourceKind = Row.SourceKind;
		MappingRow.SourcePath = Row.SourcePath;
		MappingRow.ActorName = Row.ActorName;
		MappingRow.ComponentName = Row.ComponentName;
		MappingRow.StaticMeshPath = Row.AssetPath;
		MappingRow.MaterialSlotName = Row.MaterialSlotName;
		MappingRow.MaterialPath = Row.MaterialPath;
		MappingRow.TextureChannels.Reserve(Row.TextureChannels.Num());
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			const FMaterialBatchBuildTextureChannelPlan EvaluatedChannel =
				WithEvaluatedTextureArrayEligibility(Channel);

			FMaterialBatchMappingTextureChannel MappingChannel;
			MappingChannel.ChannelName = EvaluatedChannel.ChannelName;
			MappingChannel.ParameterName = EvaluatedChannel.ParameterName;
			MappingChannel.TexturePath = EvaluatedChannel.TexturePath;
			MappingChannel.TextureClass = EvaluatedChannel.TextureClass;
			MappingChannel.bFoundTexture = EvaluatedChannel.bFoundTexture;
			MappingChannel.Width = EvaluatedChannel.TextureWidth;
			MappingChannel.Height = EvaluatedChannel.TextureHeight;
			MappingChannel.bArrayBuildEligible = EvaluatedChannel.bTextureArrayBuildEligible;
			MappingChannel.ArrayBuildReason = EvaluatedChannel.TextureArrayBuildReason;
			MappingRow.TextureChannels.Add(MappingChannel);
		}
		MappingData.MaterialRows.Add(MappingRow);
	}

	FMaterialBatchBuildTextureArrayPlans TextureArrayPlans;
	TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows =
		BuildPropertyRowPlans(Plan.PlannedMaterialRows, TextureArrayPlans);
	ApplyVTAtlasRectsToPropertyRows(VTAtlasEntries, TextureArrayPlans, PropertyRows);

	auto AppendTextureArraySlices = [&MappingData](
		const FString& ChannelName,
		const TArray<FMaterialBatchBuildTextureArraySlicePlan>& SlicePlans)
	{
		for (const FMaterialBatchBuildTextureArraySlicePlan& SlicePlan : SlicePlans)
		{
			FMaterialBatchMappingTextureArraySlice MappingSlice;
			MappingSlice.ChannelName = ChannelName;
			MappingSlice.SliceIndex = SlicePlan.SliceIndex;
			MappingSlice.TexturePath = SlicePlan.TexturePath;
			MappingSlice.TextureClass = SlicePlan.TextureClass;
			MappingData.TextureArraySlices.Add(MappingSlice);
		}
	};

	MappingData.TextureArraySlices.Reset();
	AppendTextureArraySlices(TEXT("BaseColor"), TextureArrayPlans.BaseColor);
	AppendTextureArraySlices(TEXT("Normal"), TextureArrayPlans.Normal);
	AppendTextureArraySlices(TEXT("ORM"), TextureArrayPlans.Orm);
	AppendTextureArraySlices(TEXT("Emissive"), TextureArrayPlans.Emissive);
	AppendTextureArraySlices(TEXT("Mask"), TextureArrayPlans.Mask);

	MappingData.VTAtlasEntries.Reset();
	MappingData.VTAtlasEntries.Reserve(VTAtlasEntries.Num());
	for (const FMaterialBatchBuildVTAtlasEntry& AtlasEntry : VTAtlasEntries)
	{
		FMaterialBatchMappingVTAtlasEntry MappingAtlasEntry;
		MappingAtlasEntry.AtlasEntryIndex = AtlasEntry.AtlasEntryIndex;
		MappingAtlasEntry.ChannelName = AtlasEntry.ChannelName;
		MappingAtlasEntry.TexturePath = AtlasEntry.TexturePath;
		MappingAtlasEntry.TextureClass = AtlasEntry.TextureClass;
		MappingAtlasEntry.Width = AtlasEntry.Width;
		MappingAtlasEntry.Height = AtlasEntry.Height;
		MappingAtlasEntry.VirtualTextureLayout = AtlasEntry.VirtualTextureLayout;
		MappingAtlasEntry.UdimNumber = AtlasEntry.UdimNumber;
		MappingAtlasEntry.TileU = AtlasEntry.TileU;
		MappingAtlasEntry.TileV = AtlasEntry.TileV;
		MappingAtlasEntry.TilePaddingPixels = AtlasEntry.TilePaddingPixels;
		MappingAtlasEntry.UVRemapStatus = AtlasEntry.UVRemapStatus;
		MappingAtlasEntry.UVRectMin = AtlasEntry.UVRectMin;
		MappingAtlasEntry.UVRectMax = AtlasEntry.UVRectMax;
		MappingAtlasEntry.EstimatedSourceMB = AtlasEntry.EstimatedSourceMB;
		MappingData.VTAtlasEntries.Add(MappingAtlasEntry);
	}

	MappingData.PropertyTextureColumns.Reset();
	for (const FMaterialBatchBuildPropertyTextureColumnPlan& ColumnPlan : GetPropertyTextureColumnPlans())
	{
		FMaterialBatchMappingPropertyTextureColumn MappingColumn;
		MappingColumn.PropertyIndex = ColumnPlan.PropertyIndex;
		MappingColumn.Name = ColumnPlan.Name;
		MappingColumn.SourceField = ColumnPlan.SourceField;
		MappingColumn.ValueType = ColumnPlan.ValueType;
		MappingColumn.DefaultIntValue = ColumnPlan.DefaultIntValue;
		MappingColumn.Description = ColumnPlan.Description;
		MappingData.PropertyTextureColumns.Add(MappingColumn);
	}

	MappingData.PropertyRows.Reset();
	MappingData.PropertyRows.Reserve(PropertyRows.Num());
	for (const FMaterialBatchBuildPropertyRowPlan& PropertyRow : PropertyRows)
	{
		FMaterialBatchMappingPropertyRow MappingPropertyRow;
		MappingPropertyRow.BatchMaterialIndex = PropertyRow.BatchMaterialIndex;
		MappingPropertyRow.MaterialPath = PropertyRow.MaterialPath;
		MappingPropertyRow.BaseColorSlice = PropertyRow.BaseColorSliceIndex;
		MappingPropertyRow.NormalSlice = PropertyRow.NormalSliceIndex;
		MappingPropertyRow.OrmSlice = PropertyRow.OrmSliceIndex;
		MappingPropertyRow.EmissiveSlice = PropertyRow.EmissiveSliceIndex;
		MappingPropertyRow.MaskSlice = PropertyRow.MaskSliceIndex;
		MappingPropertyRow.LightInfoTexturePath = PropertyRow.LightInfoTexturePath;
		MappingPropertyRow.SourceTexturePath = PropertyRow.SourceTexturePath;
		MappingPropertyRow.UVRectMin = PropertyRow.UVRectMin;
		MappingPropertyRow.UVRectMax = PropertyRow.UVRectMax;
		MappingPropertyRow.NormalSourceTexturePath = PropertyRow.NormalSourceTexturePath;
		MappingPropertyRow.NormalUVRectMin = PropertyRow.NormalUVRectMin;
		MappingPropertyRow.NormalUVRectMax = PropertyRow.NormalUVRectMax;
		MappingPropertyRow.OrmSourceTexturePath = PropertyRow.OrmSourceTexturePath;
		MappingPropertyRow.OrmUVRectMin = PropertyRow.OrmUVRectMin;
		MappingPropertyRow.OrmUVRectMax = PropertyRow.OrmUVRectMax;
		MappingPropertyRow.SurfaceKind = Plan.SurfaceKind;
		MappingPropertyRow.BakePolicy = Plan.BakePolicy;
		MappingPropertyRow.MaterialQualityFlags = Plan.SanitizedTierName;
		MappingData.PropertyRows.Add(MappingPropertyRow);
	}

	const TArray<FMaterialBatchBuildGeometryMergeSourcePlan> GeometrySources =
		BuildGeometryMergeSourcePlans(Plan.PlannedEntries);
	MappingData.GeometrySources.Reset();
	MappingData.GeometrySources.Reserve(GeometrySources.Num());
	for (const FMaterialBatchBuildGeometryMergeSourcePlan& SourcePlan : GeometrySources)
	{
		FMaterialBatchMappingGeometrySource MappingSource;
		MappingSource.SourceEntryIndex = SourcePlan.SourceEntryIndex;
		MappingSource.ActorName = SourcePlan.ActorName;
		MappingSource.ComponentName = SourcePlan.ComponentName;
		MappingSource.StaticMeshPath = SourcePlan.StaticMeshPath;
		MappingSource.bHasWorldTransform = SourcePlan.bHasWorldTransform;
		MappingSource.WorldLocation = SourcePlan.WorldLocation;
		MappingSource.WorldRotation = SourcePlan.WorldRotation;
		MappingSource.WorldScale = SourcePlan.WorldScale;
		MappingSource.FirstBatchMaterialIndex = SourcePlan.FirstBatchMaterialIndex;
		MappingSource.BatchMaterialIndexCount = SourcePlan.BatchMaterialIndexCount;
		MappingSource.MaterialSlotRemap.Reserve(SourcePlan.MaterialSlotToBatchMaterialIndex.Num());
		for (int32 MaterialSlotIndex = 0; MaterialSlotIndex < SourcePlan.MaterialSlotToBatchMaterialIndex.Num(); ++MaterialSlotIndex)
		{
			FMaterialBatchMappingMaterialSlotRemap Remap;
			Remap.SourceMaterialSlotIndex = MaterialSlotIndex;
			Remap.BatchMaterialIndex = SourcePlan.MaterialSlotToBatchMaterialIndex[MaterialSlotIndex];
			MappingSource.MaterialSlotRemap.Add(Remap);
		}
		MappingData.GeometrySources.Add(MappingSource);
	}
}

TArray<FString> FMaterialBatchBuildPlanBuilder::BuildMarkdownReport(const FMaterialBatchBuildPlan& Plan)
{
	TArray<FString> Lines;
	Lines.Add(TEXT("# Material Batch Build Plan"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Root: `%s`"), Plan.Options.RootPath.IsEmpty() ? TEXT("(not set)") : *Plan.Options.RootPath));
	Lines.Add(FString::Printf(TEXT("- Map: `%s`"), Plan.Options.MapPath.IsEmpty() ? TEXT("(not set)") : *Plan.Options.MapPath));
	Lines.Add(FString::Printf(TEXT("- LayerBackend: `%s`"), *NormalizeLayerBackend(Plan.Options.LayerBackend)));
	if (!Plan.Options.DataLayerName.IsEmpty())
	{
		Lines.Add(FString::Printf(TEXT("- LegacyDataLayer: `%s`"), *Plan.Options.DataLayerName));
	}
	Lines.Add(FString::Printf(TEXT("- Cluster: `%s`"), *Plan.Options.ClusterName));
	Lines.Add(FString::Printf(TEXT("- Tier: `%s`"), *Plan.Options.TierName));
	Lines.Add(FString::Printf(TEXT("- TextureBackend: `%s`"), *Plan.TextureBackend));
	Lines.Add(FString::Printf(TEXT("- SurfaceKind: `%s`"), *Plan.SurfaceKind));
	Lines.Add(FString::Printf(TEXT("- BakePolicy: `%s`"), *Plan.BakePolicy));
	Lines.Add(FString::Printf(TEXT("- SourceProxyExclusivityGroup: `%s`"), *Plan.SourceProxyExclusivityGroup));
	Lines.Add(FString::Printf(TEXT("- VTAtlasPackage: `%s`"), *Plan.VTAtlasPackage));
	Lines.Add(FString::Printf(TEXT("- Rules: `%s`"), Plan.Options.RulesPath.IsEmpty() ? TEXT("(not set)") : *Plan.Options.RulesPath));
	Lines.Add(FString::Printf(TEXT("- OutputRoot: `%s`"), *NormalizeOutputRoot(Plan.Options.OutputRoot)));
	Lines.Add(Plan.bDryRun
		? TEXT("- Mode: dry-run; no assets are modified or generated.")
		: TEXT("- Mode: partial apply; supported generated assets may be written, while full apply and map replacement remain disabled."));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Candidate Summary"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Source kind: %s"), Plan.CandidateSummary.SourceKind.IsEmpty() ? TEXT("(not scanned)") : *Plan.CandidateSummary.SourceKind));
	Lines.Add(FString::Printf(TEXT("- Source found: %d"), Plan.CandidateSummary.SourceFoundCount));
	Lines.Add(FString::Printf(TEXT("- Source inspected: %d"), Plan.CandidateSummary.SourceInspectedCount));
	Lines.Add(FString::Printf(TEXT("- Batch candidates: %d"), Plan.CandidateSummary.BatchCandidateCount));
	Lines.Add(FString::Printf(TEXT("- Rejected: %d"), Plan.CandidateSummary.RejectedCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## EnvBatch Tag Diagnostics"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Actors with EnvBatch tags: %d"), Plan.TagDiagnostics.ActorCount));
	Lines.Add(FString::Printf(TEXT("- Source actors: %d"), Plan.TagDiagnostics.SourceActorCount));
	Lines.Add(FString::Printf(TEXT("- Proxy actors: %d"), Plan.TagDiagnostics.ProxyActorCount));
	Lines.Add(FString::Printf(TEXT("- Baked actors: %d"), Plan.TagDiagnostics.BakedActorCount));
	Lines.Add(FString::Printf(TEXT("- Excluded actors: %d"), Plan.TagDiagnostics.ExcludeActorCount));
	Lines.Add(FString::Printf(TEXT("- Source/Proxy conflicts: %d"), Plan.TagDiagnostics.SourceProxyConflictActorCount));
	Lines.Add(FString::Printf(TEXT("- Static decal bake actors: %d"), Plan.TagDiagnostics.BakeStaticDecalActorCount));
	Lines.Add(FString::Printf(TEXT("- Runtime decal actors: %d"), Plan.TagDiagnostics.RuntimeDecalActorCount));
	Lines.Add(FString::Printf(TEXT("- Gameplay indicator actors: %d"), Plan.TagDiagnostics.GameplayIndicatorActorCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Warning |"));
	Lines.Add(TEXT("| --- |"));
	if (Plan.TagDiagnostics.Warnings.IsEmpty())
	{
		Lines.Add(TEXT("| (none) |"));
	}
	else
	{
		for (const FString& Warning : Plan.TagDiagnostics.Warnings)
		{
			Lines.Add(FString::Printf(TEXT("| %s |"), Warning.IsEmpty() ? TEXT("(empty)") : *Warning));
		}
	}
	Lines.Add(TEXT(""));
	const FMaterialBatchBuildSourceProxyLayerPlan SourceProxyLayerPlan = BuildSourceProxyLayerPlan(Plan);
	Lines.Add(TEXT("## Source/Proxy/Baked Layer Plan"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Exclusivity group: `%s`"), *SourceProxyLayerPlan.ExclusivityGroup));
	Lines.Add(FString::Printf(TEXT("- Layer backend: `%s`"), *SourceProxyLayerPlan.LayerBackend));
	Lines.Add(FString::Printf(TEXT("- Source layer: `%s`"), *SourceProxyLayerPlan.SourceLayerName));
	Lines.Add(FString::Printf(TEXT("- Proxy layer: `%s`"), *SourceProxyLayerPlan.ProxyLayerName));
	Lines.Add(FString::Printf(TEXT("- Baked layer: `%s`"), *SourceProxyLayerPlan.BakedLayerName));
	Lines.Add(FString::Printf(TEXT("- Requires mutual exclusion: %s"), SourceProxyLayerPlan.bRequiresMutualExclusion ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Tag conflict gate: %s"), SourceProxyLayerPlan.bHasTagConflicts ? TEXT("Blocked") : TEXT("Clear")));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Tier | Load Source | Load Proxy | Load Baked | Fallback Policy |"));
	Lines.Add(TEXT("| --- | --- | --- | --- | --- |"));
	for (const FMaterialBatchBuildTierLayerSelection& Selection : SourceProxyLayerPlan.TierSelections)
	{
		Lines.Add(FString::Printf(
			TEXT("| %s | %s | %s | %s | %s |"),
			*Selection.TierName,
			Selection.bLoadSourceLayer ? TEXT("Yes") : TEXT("No"),
			Selection.bLoadProxyLayer ? TEXT("Yes") : TEXT("No"),
			Selection.bLoadBakedLayer ? TEXT("Yes") : TEXT("No"),
			*Selection.FallbackPolicy));
	}
	Lines.Add(TEXT(""));
	const FMaterialBatchBuildSourceProxyLayerReadiness SourceProxyLayerReadiness = BuildSourceProxyLayerReadiness(Plan);
	Lines.Add(TEXT("## Source/Proxy/Baked Layer Readiness"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Entries: %d"), SourceProxyLayerReadiness.EntryCount));
	Lines.Add(FString::Printf(TEXT("- Ready for layer validation: %d"), SourceProxyLayerReadiness.ReadyEntryCount));
	Lines.Add(FString::Printf(TEXT("- Missing layer tags: %d"), SourceProxyLayerReadiness.MissingLayerTagEntryCount));
	Lines.Add(FString::Printf(TEXT("- Layer tag conflicts: %d"), SourceProxyLayerReadiness.ConflictEntryCount));
	Lines.Add(FString::Printf(TEXT("- Excluded entries: %d"), SourceProxyLayerReadiness.ExcludedEntryCount));
	Lines.Add(FString::Printf(TEXT("- Actual layer matches: %d"), SourceProxyLayerReadiness.ActualLayerMatchCount));
	Lines.Add(FString::Printf(TEXT("- Missing actual layer evidence: %d"), SourceProxyLayerReadiness.MissingActualLayerCount));
	Lines.Add(FString::Printf(TEXT("- Unexpected actual layer assignments: %d"), SourceProxyLayerReadiness.UnexpectedActualLayerCount));
	Lines.Add(FString::Printf(TEXT("- Actual layer not required: %d"), SourceProxyLayerReadiness.NotRequiredActualLayerCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| SourceEntryIndex | Actor | Component | Role | Expected Layer | Actual Layers | StreamingLevel | Ready | Match | Layer Status | Reason | EnvBatchTags |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"));
	if (SourceProxyLayerReadiness.Assignments.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | `` | `` | Unassigned | `` |  | `` | No | No | NotScanned | Not scanned. |  |"));
	}
	else
	{
		for (const FMaterialBatchBuildSourceProxyLayerAssignment& Assignment : SourceProxyLayerReadiness.Assignments)
		{
			const FString JoinedEnvBatchTags = Assignment.EnvBatchTags.IsEmpty()
				? FString()
				: FString::Join(Assignment.EnvBatchTags, TEXT(", "));
			const FString JoinedActualLayers = Assignment.ActualLayerNames.IsEmpty()
				? FString()
				: FString::Join(Assignment.ActualLayerNames, TEXT(", "));
			Lines.Add(FString::Printf(
				TEXT("| %d | `%s` | `%s` | %s | `%s` | %s | `%s` | %s | %s | %s | %s | %s |"),
				Assignment.SourceEntryIndex,
				Assignment.ActorName.IsEmpty() ? TEXT("") : *Assignment.ActorName,
				Assignment.ComponentName.IsEmpty() ? TEXT("") : *Assignment.ComponentName,
				*Assignment.LayerRole,
				Assignment.ExpectedLayerName.IsEmpty() ? TEXT("") : *Assignment.ExpectedLayerName,
				JoinedActualLayers.IsEmpty() ? TEXT("") : *JoinedActualLayers,
				Assignment.ActualStreamingLevelName.IsEmpty() ? TEXT("") : *Assignment.ActualStreamingLevelName,
				Assignment.bReadyForLayerValidation ? TEXT("Yes") : TEXT("No"),
				Assignment.bMatchesExpectedLayer ? TEXT("Yes") : TEXT("No"),
				Assignment.LayerValidationStatus.IsEmpty() ? TEXT("") : *Assignment.LayerValidationStatus,
				Assignment.ReadinessReason.IsEmpty() ? TEXT("") : *Assignment.ReadinessReason,
				JoinedEnvBatchTags.IsEmpty() ? TEXT("") : *JoinedEnvBatchTags));
		}
	}
	Lines.Add(TEXT(""));
	const FMaterialBatchBuildSourceProxyAssetReadiness SourceProxyAssetReadiness = BuildSourceProxyAssetReadiness(Plan);
	Lines.Add(TEXT("## Source/Proxy Asset Readiness"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Entries: %d"), SourceProxyAssetReadiness.EntryCount));
	Lines.Add(FString::Printf(TEXT("- Ready source/proxy pairs: %d"), SourceProxyAssetReadiness.ReadyPairCount));
	Lines.Add(FString::Printf(TEXT("- Missing source asset references: %d"), SourceProxyAssetReadiness.MissingSourceAssetCount));
	Lines.Add(FString::Printf(TEXT("- Missing proxy asset references: %d"), SourceProxyAssetReadiness.MissingProxyAssetCount));
	Lines.Add(FString::Printf(TEXT("- Generated proxy fallbacks: %d"), SourceProxyAssetReadiness.GeneratedProxyFallbackCount));
	Lines.Add(FString::Printf(TEXT("- Authored proxy entries: %d"), SourceProxyAssetReadiness.AuthoredProxyCount));
	Lines.Add(FString::Printf(TEXT("- Not required entries: %d"), SourceProxyAssetReadiness.NotRequiredCount));
	Lines.Add(FString::Printf(TEXT("- Conflicts: %d"), SourceProxyAssetReadiness.ConflictCount));
	Lines.Add(TEXT("- Default Source LOD index: 0"));
	Lines.Add(TEXT("- Default Proxy LOD index: 1"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| SourceEntryIndex | Actor | Component | Role | Source Asset | Proxy Asset | SourceLOD | ProxyLOD | GeneratedProxy | AuthoredProxy | Ready | Status | Reason | EnvBatchTags |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | --- | --- | ---: | ---: | --- | --- | --- | --- | --- | --- |"));
	if (SourceProxyAssetReadiness.Assignments.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | `` | `` | Unassigned | `` | `` | 0 | 1 | No | No | No | NotScanned | Not scanned. |  |"));
	}
	else
	{
		for (const FMaterialBatchBuildSourceProxyAssetAssignment& Assignment : SourceProxyAssetReadiness.Assignments)
		{
			const FString JoinedEnvBatchTags = Assignment.EnvBatchTags.IsEmpty()
				? FString()
				: FString::Join(Assignment.EnvBatchTags, TEXT(", "));
			Lines.Add(FString::Printf(
				TEXT("| %d | `%s` | `%s` | %s | `%s` | `%s` | %d | %d | %s | %s | %s | %s | %s | %s |"),
				Assignment.SourceEntryIndex,
				Assignment.ActorName.IsEmpty() ? TEXT("") : *Assignment.ActorName,
				Assignment.ComponentName.IsEmpty() ? TEXT("") : *Assignment.ComponentName,
				*Assignment.LayerRole,
				Assignment.SourceAssetPath.IsEmpty() ? TEXT("") : *Assignment.SourceAssetPath,
				Assignment.ProxyAssetPath.IsEmpty() ? TEXT("") : *Assignment.ProxyAssetPath,
				Assignment.SourceLODIndex,
				Assignment.ProxyLODIndex,
				Assignment.bUsesGeneratedProxy ? TEXT("Yes") : TEXT("No"),
				Assignment.bUsesAuthoredProxy ? TEXT("Yes") : TEXT("No"),
				Assignment.bReadyForAssetPairing ? TEXT("Yes") : TEXT("No"),
				Assignment.ReadinessStatus.IsEmpty() ? TEXT("") : *Assignment.ReadinessStatus,
				Assignment.ReadinessReason.IsEmpty() ? TEXT("") : *Assignment.ReadinessReason,
				JoinedEnvBatchTags.IsEmpty() ? TEXT("") : *JoinedEnvBatchTags));
		}
	}
	Lines.Add(TEXT(""));
	const FMaterialBatchBuildSourceProxyAssetConfigSet SourceProxyAssetConfigSet = BuildSourceProxyAssetConfigSet(Plan);
	Lines.Add(TEXT("## Source/Proxy Asset Config Set"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Configs: %d"), SourceProxyAssetConfigSet.ConfigCount));
	Lines.Add(FString::Printf(TEXT("- Ready configs: %d"), SourceProxyAssetConfigSet.ReadyConfigCount));
	Lines.Add(FString::Printf(TEXT("- Generated fallback configs: %d"), SourceProxyAssetConfigSet.GeneratedFallbackConfigCount));
	Lines.Add(FString::Printf(TEXT("- Authored proxy configs: %d"), SourceProxyAssetConfigSet.AuthoredProxyConfigCount));
	Lines.Add(FString::Printf(TEXT("- Missing source reference configs: %d"), SourceProxyAssetConfigSet.MissingSourceReferenceCount));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| ObjectKey | Role | Source Asset | Explicit Proxy Asset | GeneratedProxy Asset | SourceLOD | ProxyLOD | SurfaceKind | BakePolicy | InteractionPolicy | ConfigSource | Ready | Status |"));
	Lines.Add(TEXT("| --- | --- | --- | --- | --- | ---: | ---: | --- | --- | --- | --- | --- | --- |"));
	if (SourceProxyAssetConfigSet.Configs.IsEmpty())
	{
		Lines.Add(TEXT("| Entry_-1 | Unassigned | `` | `` | `` | 0 | 1 | `` | `` | RequiresArtReview | UnassignedEnvBatchTags | No | NotScanned |"));
	}
	else
	{
		for (const FMaterialBatchBuildSourceProxyAssetConfig& Config : SourceProxyAssetConfigSet.Configs)
		{
			Lines.Add(FString::Printf(
				TEXT("| %s | %s | `%s` | `%s` | `%s` | %d | %d | %s | %s | %s | %s | %s | %s |"),
				Config.ObjectKey.IsEmpty() ? TEXT("Entry") : *Config.ObjectKey,
				Config.LayerRole.IsEmpty() ? TEXT("Unassigned") : *Config.LayerRole,
				Config.SourceAssetPath.IsEmpty() ? TEXT("") : *Config.SourceAssetPath,
				Config.AuthorProxyAssetPath.IsEmpty() ? TEXT("") : *Config.AuthorProxyAssetPath,
				Config.GeneratedProxyAssetPath.IsEmpty() ? TEXT("") : *Config.GeneratedProxyAssetPath,
				Config.SourceLODIndex,
				Config.ProxyLODIndex,
				Config.SurfaceKind.IsEmpty() ? TEXT("") : *Config.SurfaceKind,
				Config.BakePolicy.IsEmpty() ? TEXT("") : *Config.BakePolicy,
				Config.InteractionPolicy.IsEmpty() ? TEXT("") : *Config.InteractionPolicy,
				Config.ConfigSource.IsEmpty() ? TEXT("") : *Config.ConfigSource,
				Config.bReadyForAssetPairing ? TEXT("Yes") : TEXT("No"),
				Config.ReadinessStatus.IsEmpty() ? TEXT("") : *Config.ReadinessStatus));
		}
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Planned Generated Assets"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Type | Package |"));
	Lines.Add(TEXT("| --- | --- |"));
	Lines.Add(FString::Printf(TEXT("| Proxy Mesh | `%s` |"), *Plan.ProxyMeshPackage));
	Lines.Add(FString::Printf(TEXT("| Batch Material Instance | `%s` |"), *Plan.BatchMaterialInstancePackage));
	Lines.Add(FString::Printf(TEXT("| Batch Parent Material | `%s` |"), *Plan.BatchParentMaterialPackage));
	Lines.Add(FString::Printf(TEXT("| BaseColor Texture2DArray | `%s` |"), *Plan.BaseColorArrayPackage));
	Lines.Add(FString::Printf(TEXT("| Normal Texture2DArray | `%s` |"), *Plan.NormalArrayPackage));
	Lines.Add(FString::Printf(TEXT("| ORM Texture2DArray | `%s` |"), *Plan.OrmArrayPackage));
	Lines.Add(FString::Printf(TEXT("| Emissive Texture2DArray | `%s` |"), *Plan.EmissiveArrayPackage));
	Lines.Add(FString::Printf(TEXT("| Mask Texture2DArray | `%s` |"), *Plan.MaskArrayPackage));
	Lines.Add(FString::Printf(TEXT("| Property Texture | `%s` |"), *Plan.PropertyTexturePackage));
	Lines.Add(FString::Printf(TEXT("| Mapping Data Asset | `%s` |"), *Plan.MappingDataAssetPackage));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Planned Batch Entries"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Source | Actor | Component | StreamingLevel | Asset | Materials | LODs | EnvBatchTags | Status | Reason | FirstBatchMaterialIndex | BatchMaterialIndexCount |"));
	Lines.Add(TEXT("| --- | --- | --- | --- | --- | ---: | ---: | --- | --- | --- | ---: | ---: |"));
	if (Plan.PlannedEntries.IsEmpty())
	{
		Lines.Add(TEXT("| (none) | `` | `` | `` | `` | 0 | 0 |  | Pending | NotScanned | -1 | 0 |"));
	}
	else
	{
		for (const FMaterialBatchBuildPlannedEntry& Entry : Plan.PlannedEntries)
		{
			const FString JoinedEnvBatchTags = Entry.EnvBatchTags.IsEmpty()
				? FString()
				: FString::Join(Entry.EnvBatchTags, TEXT(", "));
			Lines.Add(FString::Printf(
				TEXT("| %s | `%s` | `%s` | `%s` | `%s` | %d | %d | %s | %s | %s | %d | %d |"),
				*Entry.SourceKind,
				Entry.ActorName.IsEmpty() ? TEXT("") : *Entry.ActorName,
				Entry.ComponentName.IsEmpty() ? TEXT("") : *Entry.ComponentName,
				Entry.ActualStreamingLevelName.IsEmpty() ? TEXT("") : *Entry.ActualStreamingLevelName,
				Entry.AssetPath.IsEmpty() ? TEXT("") : *Entry.AssetPath,
				Entry.MaterialSlotCount,
				Entry.LodCount,
				JoinedEnvBatchTags.IsEmpty() ? TEXT("") : *JoinedEnvBatchTags,
				Entry.bCandidate ? TEXT("Candidate") : TEXT("Rejected"),
				Entry.RejectReason.IsEmpty() ? TEXT("None") : *Entry.RejectReason,
				Entry.FirstBatchMaterialIndex,
				Entry.BatchMaterialIndexCount));
		}
	}
	Lines.Add(TEXT(""));
	const TArray<FMaterialBatchBuildGeometryMergeSourcePlan> GeometryMergeSources =
		BuildGeometryMergeSourcePlans(Plan.PlannedEntries);
	Lines.Add(TEXT("## Geometry Merge Plan"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Target proxy mesh: `%s`"), *Plan.ProxyMeshPackage));
	Lines.Add(TEXT("- Source coordinate space: `World`"));
	Lines.Add(TEXT("- Material index channel: `TexCoord7.x`"));
	Lines.Add(TEXT("- Material index encoding: `batchMaterialIndex` as one float per merged vertex"));
	Lines.Add(TEXT("- Vertex color bake: `Ground.Batched` sources copy component OverrideVertexColors into the generated proxy mesh before append."));
	Lines.Add(TEXT("- Merge granularity: `cluster`"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| SourceEntryIndex | Actor | Component | StaticMesh | VertexColorBake | Location | Rotation | Scale | FirstBatchMaterialIndex | BatchMaterialIndexCount |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | --- | --- | --- | --- | ---: | ---: |"));
	if (GeometryMergeSources.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | `` | `` | `` | No | 0,0,0 | 0,0,0 | 1,1,1 | -1 | 0 |"));
	}
	else
	{
		for (const FMaterialBatchBuildGeometryMergeSourcePlan& SourcePlan : GeometryMergeSources)
		{
			Lines.Add(FString::Printf(
				TEXT("| %d | `%s` | `%s` | `%s` | %s | %.3f,%.3f,%.3f | %.3f,%.3f,%.3f | %.3f,%.3f,%.3f | %d | %d |"),
				SourcePlan.SourceEntryIndex,
				SourcePlan.ActorName.IsEmpty() ? TEXT("") : *SourcePlan.ActorName,
				SourcePlan.ComponentName.IsEmpty() ? TEXT("") : *SourcePlan.ComponentName,
				SourcePlan.StaticMeshPath.IsEmpty() ? TEXT("") : *SourcePlan.StaticMeshPath,
				SourcePlan.bBakeInstanceVertexColors ? TEXT("Yes") : TEXT("No"),
				SourcePlan.WorldLocation.X,
				SourcePlan.WorldLocation.Y,
				SourcePlan.WorldLocation.Z,
				SourcePlan.WorldRotation.Pitch,
				SourcePlan.WorldRotation.Yaw,
				SourcePlan.WorldRotation.Roll,
				SourcePlan.WorldScale.X,
				SourcePlan.WorldScale.Y,
				SourcePlan.WorldScale.Z,
				SourcePlan.FirstBatchMaterialIndex,
				SourcePlan.BatchMaterialIndexCount));
		}
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Material Slot Remap"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| SourceEntryIndex | SourceMaterialSlotIndex | BatchMaterialIndex |"));
	Lines.Add(TEXT("| ---: | ---: | ---: |"));
	bool bHasMaterialSlotRemap = false;
	for (const FMaterialBatchBuildGeometryMergeSourcePlan& SourcePlan : GeometryMergeSources)
	{
		for (int32 MaterialSlotIndex = 0; MaterialSlotIndex < SourcePlan.MaterialSlotToBatchMaterialIndex.Num(); ++MaterialSlotIndex)
		{
			bHasMaterialSlotRemap = true;
			Lines.Add(FString::Printf(
				TEXT("| %d | %d | %d |"),
				SourcePlan.SourceEntryIndex,
				MaterialSlotIndex,
				SourcePlan.MaterialSlotToBatchMaterialIndex[MaterialSlotIndex]));
		}
	}
	if (!bHasMaterialSlotRemap)
	{
		Lines.Add(TEXT("| -1 | -1 | -1 |"));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Planned Material Rows"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| BatchMaterialIndex | SourceEntryIndex | MaterialSlotIndex | MaterialSlotName | Material | Actor | Component | Asset |"));
	Lines.Add(TEXT("| ---: | ---: | ---: | --- | --- | --- | --- | --- |"));
	if (Plan.PlannedMaterialRows.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | -1 | -1 | `` | `` | `` | `` | `` |"));
	}
	else
	{
		for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
		{
			Lines.Add(FString::Printf(
				TEXT("| %d | %d | %d | `%s` | `%s` | `%s` | `%s` | `%s` |"),
				Row.BatchMaterialIndex,
				Row.SourceEntryIndex,
				Row.MaterialSlotIndex,
				Row.MaterialSlotName.IsEmpty() ? TEXT("") : *Row.MaterialSlotName,
				Row.MaterialPath.IsEmpty() ? TEXT("") : *Row.MaterialPath,
				Row.ActorName.IsEmpty() ? TEXT("") : *Row.ActorName,
				Row.ComponentName.IsEmpty() ? TEXT("") : *Row.ComponentName,
				Row.AssetPath.IsEmpty() ? TEXT("") : *Row.AssetPath));
		}
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Planned Texture Channels"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| BatchMaterialIndex | Channel | Parameter | Texture | TextureClass | Found |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | --- | --- |"));
	bool bHasTextureChannels = false;
	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			bHasTextureChannels = true;
			Lines.Add(FString::Printf(
				TEXT("| %d | %s | `%s` | `%s` | %s | %s |"),
				Row.BatchMaterialIndex,
				Channel.ChannelName.IsEmpty() ? TEXT("Unknown") : *Channel.ChannelName,
				Channel.ParameterName.IsEmpty() ? TEXT("") : *Channel.ParameterName,
				Channel.TexturePath.IsEmpty() ? TEXT("") : *Channel.TexturePath,
				Channel.TextureClass.IsEmpty() ? TEXT("") : *Channel.TextureClass,
				Channel.bFoundTexture ? TEXT("Yes") : TEXT("No")));
		}
	}
	if (!bHasTextureChannels)
	{
		Lines.Add(TEXT("| -1 | Unknown | `` | `` |  | No |"));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Texture2DArray Build Eligibility"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| BatchMaterialIndex | Channel | Parameter | Texture | TextureClass | Width | Height | Eligible | Reason |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | --- | ---: | ---: | --- | --- |"));
	bool bHasEligibilityRows = false;
	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			const FMaterialBatchBuildTextureChannelPlan EvaluatedChannel = WithEvaluatedTextureArrayEligibility(Channel);
			bHasEligibilityRows = true;
			Lines.Add(FString::Printf(
				TEXT("| %d | %s | `%s` | `%s` | %s | %d | %d | %s | %s |"),
				Row.BatchMaterialIndex,
				EvaluatedChannel.ChannelName.IsEmpty() ? TEXT("Unknown") : *EvaluatedChannel.ChannelName,
				EvaluatedChannel.ParameterName.IsEmpty() ? TEXT("") : *EvaluatedChannel.ParameterName,
				EvaluatedChannel.TexturePath.IsEmpty() ? TEXT("") : *EvaluatedChannel.TexturePath,
				EvaluatedChannel.TextureClass.IsEmpty() ? TEXT("") : *EvaluatedChannel.TextureClass,
				EvaluatedChannel.TextureWidth,
				EvaluatedChannel.TextureHeight,
				EvaluatedChannel.bTextureArrayBuildEligible ? TEXT("Yes") : TEXT("No"),
				EvaluatedChannel.TextureArrayBuildReason.IsEmpty() ? TEXT("NotEvaluated") : *EvaluatedChannel.TextureArrayBuildReason));
		}
	}
	if (!bHasEligibilityRows)
	{
		Lines.Add(TEXT("| -1 | Unknown | `` | `` |  | -1 | -1 | No | MissingTexture |"));
	}
	Lines.Add(TEXT(""));
	FMaterialBatchBuildTextureArrayPlans TextureArrayPlans;
	TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows = BuildPropertyRowPlans(
		Plan.PlannedMaterialRows,
		TextureArrayPlans);
	Lines.Add(TEXT("## Planned Texture2DArray Slices"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Channel | SliceIndex | Texture | TextureClass |"));
	Lines.Add(TEXT("| --- | ---: | --- | --- |"));
	bool bHasArraySlices = false;
	AppendTextureArraySliceRows(Lines, TEXT("BaseColor"), TextureArrayPlans.BaseColor, bHasArraySlices);
	AppendTextureArraySliceRows(Lines, TEXT("Normal"), TextureArrayPlans.Normal, bHasArraySlices);
	AppendTextureArraySliceRows(Lines, TEXT("ORM"), TextureArrayPlans.Orm, bHasArraySlices);
	AppendTextureArraySliceRows(Lines, TEXT("Emissive"), TextureArrayPlans.Emissive, bHasArraySlices);
	AppendTextureArraySliceRows(Lines, TEXT("Mask"), TextureArrayPlans.Mask, bHasArraySlices);
	if (!bHasArraySlices)
	{
		Lines.Add(TEXT("| (none) | -1 | `` |  |"));
	}
	Lines.Add(TEXT(""));
	const TArray<FMaterialBatchBuildVTAtlasEntry> VTAtlasEntries = BuildVTAtlasEntries(Plan);
	const FMaterialBatchBuildVTAtlasPayload VTAtlasPayload = BuildVTAtlasPayload(Plan);
	ApplyVTAtlasRectsToPropertyRows(VTAtlasEntries, TextureArrayPlans, PropertyRows);
	const float EstimatedStreamingPoolMB = SumTextureArraySourceMB(Plan);
	const float EstimatedVTPoolMB = SumVTAtlasSourceMB(VTAtlasEntries);
	const bool bDuplicateResidencyRisk = Plan.TextureBackend.Equals(TEXT("VTAtlas"), ESearchCase::IgnoreCase)
		&& EstimatedStreamingPoolMB > 0.f
		&& EstimatedVTPoolMB > 0.f;
	Lines.Add(TEXT("## Planned VT Atlas Entries"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Package: `%s`"), *Plan.VTAtlasPackage));
	Lines.Add(FString::Printf(TEXT("- Layout: `%s`"), TEXT("DeterministicGridByUniqueTexture")));
	Lines.Add(FString::Printf(TEXT("- Virtual texture layout: `%s`"), *VTAtlasPayload.VirtualTextureLayout));
	Lines.Add(FString::Printf(TEXT("- Tile padding pixels: %d"), VTAtlasPayload.TilePaddingPixels));
	Lines.Add(FString::Printf(TEXT("- Estimated VT pool source MB: %.3f"), EstimatedVTPoolMB));
	Lines.Add(FString::Printf(TEXT("- Estimated non-VT streaming source MB: %.3f"), EstimatedStreamingPoolMB));
	Lines.Add(FString::Printf(TEXT("- Duplicate residency risk: %s"), bDuplicateResidencyRisk ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Planned atlas size: %dx%d"), VTAtlasPayload.Width, VTAtlasPayload.Height));
	Lines.Add(FString::Printf(TEXT("- Planned atlas grid: %d x %d"), VTAtlasPayload.Columns, VTAtlasPayload.Rows));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| Entry | Channel | Texture | TextureClass | Width | Height | UDIM | Tile | Padding | UVRemap | UVMin | UVMax | EstimatedSourceMB |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | ---: | ---: | ---: | --- | ---: | --- | --- | --- | ---: |"));
	if (VTAtlasEntries.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | Unknown | `` |  | -1 | -1 | -1 | -1,-1 | 0 |  | 0,0 | 1,1 | 0.000 |"));
	}
	else
	{
		for (const FMaterialBatchBuildVTAtlasEntry& AtlasEntry : VTAtlasEntries)
		{
			Lines.Add(FString::Printf(
				TEXT("| %d | %s | `%s` | %s | %d | %d | %d | %d,%d | %d | %s | %.4f,%.4f | %.4f,%.4f | %.3f |"),
				AtlasEntry.AtlasEntryIndex,
				*AtlasEntry.ChannelName,
				AtlasEntry.TexturePath.IsEmpty() ? TEXT("") : *AtlasEntry.TexturePath,
				AtlasEntry.TextureClass.IsEmpty() ? TEXT("") : *AtlasEntry.TextureClass,
				AtlasEntry.Width,
				AtlasEntry.Height,
				AtlasEntry.UdimNumber,
				AtlasEntry.TileU,
				AtlasEntry.TileV,
				AtlasEntry.TilePaddingPixels,
				*AtlasEntry.UVRemapStatus,
				AtlasEntry.UVRectMin.X,
				AtlasEntry.UVRectMin.Y,
				AtlasEntry.UVRectMax.X,
				AtlasEntry.UVRectMax.Y,
				AtlasEntry.EstimatedSourceMB));
		}
	}
	Lines.Add(TEXT(""));
	const FMaterialBatchBuildResidencyRiskPlan ResidencyRiskPlan = BuildResidencyRiskPlan(Plan);
	Lines.Add(TEXT("## VT/Non-VT Residency Risk Plan"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Texture backend: `%s`"), *ResidencyRiskPlan.TextureBackend));
	Lines.Add(FString::Printf(TEXT("- VT Atlas main path: %s"), ResidencyRiskPlan.bVTAtlasMainPath ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Texture2DArray fallback present: %s"), ResidencyRiskPlan.bTextureArrayFallbackPresent ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Texture2DArray fallback allowed in production: %s"), ResidencyRiskPlan.bAllowTextureArrayFallbackInProduction ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Requires Source/Proxy unload before shipping: %s"), ResidencyRiskPlan.bRequiresSourceProxyUnload ? TEXT("Yes") : TEXT("No")));
	Lines.Add(FString::Printf(TEXT("- Estimated VT physical pool source MB: %.3f"), ResidencyRiskPlan.EstimatedVTPoolMB));
	Lines.Add(FString::Printf(TEXT("- Estimated non-VT streaming pool source MB: %.3f"), ResidencyRiskPlan.EstimatedStreamingPoolMB));
	Lines.Add(FString::Printf(TEXT("- Estimated combined source MB if mixed: %.3f"), ResidencyRiskPlan.EstimatedCombinedPoolMB));
	Lines.Add(FString::Printf(TEXT("- Residency gate: `%s`"), *ResidencyRiskPlan.ResidencyGate));
	Lines.Add(FString::Printf(TEXT("- Recommendation: %s"), *ResidencyRiskPlan.Recommendation));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Property Texture Layout"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Row key: `batchMaterialIndex`"));
	Lines.Add(FString::Printf(TEXT("- Row count: %d"), PropertyRows.Num()));
	Lines.Add(TEXT("- Encoding: `x=propertyIndex, y=batchMaterialIndex`"));
	Lines.Add(TEXT("- Missing slice value: `-1`"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| PropertyIndex | Name | SourceField | ValueType | DefaultValue | Description |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | ---: | --- |"));
	for (const FMaterialBatchBuildPropertyTextureColumnPlan& ColumnPlan : GetPropertyTextureColumnPlans())
	{
		Lines.Add(FString::Printf(
			TEXT("| %d | %s | %s | %s | %d | %s |"),
			ColumnPlan.PropertyIndex,
			*ColumnPlan.Name,
			*ColumnPlan.SourceField,
			*ColumnPlan.ValueType,
			ColumnPlan.DefaultIntValue,
			*ColumnPlan.Description));
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Planned Property Texture Rows"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| BatchMaterialIndex | BaseColorSlice | NormalSlice | ORMSlice | EmissiveSlice | MaskSlice | BaseVTUVMin | BaseVTUVMax | NormalVTUVMin | NormalVTUVMax | ORMVTUVMin | ORMVTUVMax | BaseTexture | NormalTexture | ORMTexture | LightInfoTexture | Material |"));
	Lines.Add(TEXT("| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |"));
	if (PropertyRows.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | -1 | -1 | -1 | -1 | -1 | 0,0 | 1,1 | 0,0 | 1,1 | 0,0 | 1,1 | `` | `` | `` | `` | `` |"));
	}
	else
	{
		for (const FMaterialBatchBuildPropertyRowPlan& PropertyRow : PropertyRows)
		{
			Lines.Add(FString::Printf(
				TEXT("| %d | %d | %d | %d | %d | %d | %.4f,%.4f | %.4f,%.4f | %.4f,%.4f | %.4f,%.4f | %.4f,%.4f | %.4f,%.4f | `%s` | `%s` | `%s` | `%s` | `%s` |"),
				PropertyRow.BatchMaterialIndex,
				PropertyRow.BaseColorSliceIndex,
				PropertyRow.NormalSliceIndex,
				PropertyRow.OrmSliceIndex,
				PropertyRow.EmissiveSliceIndex,
				PropertyRow.MaskSliceIndex,
				PropertyRow.UVRectMin.X,
				PropertyRow.UVRectMin.Y,
				PropertyRow.UVRectMax.X,
				PropertyRow.UVRectMax.Y,
				PropertyRow.NormalUVRectMin.X,
				PropertyRow.NormalUVRectMin.Y,
				PropertyRow.NormalUVRectMax.X,
				PropertyRow.NormalUVRectMax.Y,
				PropertyRow.OrmUVRectMin.X,
				PropertyRow.OrmUVRectMin.Y,
				PropertyRow.OrmUVRectMax.X,
				PropertyRow.OrmUVRectMax.Y,
				PropertyRow.SourceTexturePath.IsEmpty() ? TEXT("") : *PropertyRow.SourceTexturePath,
				PropertyRow.NormalSourceTexturePath.IsEmpty() ? TEXT("") : *PropertyRow.NormalSourceTexturePath,
				PropertyRow.OrmSourceTexturePath.IsEmpty() ? TEXT("") : *PropertyRow.OrmSourceTexturePath,
				PropertyRow.LightInfoTexturePath.IsEmpty() ? TEXT("") : *PropertyRow.LightInfoTexturePath,
				PropertyRow.MaterialPath.IsEmpty() ? TEXT("") : *PropertyRow.MaterialPath));
		}
	}
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Batch Material Parent Contract"));
	Lines.Add(TEXT(""));
	Lines.Add(FString::Printf(TEXT("- Parent material: `%s`"), *Plan.BatchParentMaterialPackage));
	Lines.Add(FString::Printf(TEXT("- Material index channel: `%s`"), *Plan.BatchMaterialIndexChannel));
	Lines.Add(FString::Printf(TEXT("- Material index source: `%s`"), *Plan.BatchMaterialIndexSource));
	Lines.Add(FString::Printf(TEXT("- Property texture parameter: `%s`"), *Plan.BatchPropertyTextureParameterName));
	Lines.Add(FString::Printf(TEXT("- Property texture row encoding: `%s`"), *Plan.BatchPropertyTextureRowEncoding));
	Lines.Add(TEXT("- Required VT Atlas parameter: `VT_Atlas`."));
	Lines.Add(TEXT("- Texture2DArray parameters `T_Array_A`, `T_Array_N`, `T_Array_M` remain fallback-only for legacy generated assets."));
	Lines.Add(TEXT("- The parent material must use the property texture row values as VT atlas UV rect data; binding source preview textures is not sufficient for production batching."));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Current Scope"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Dry-run mode creates a deterministic build plan only."));
	Lines.Add(TEXT("- `-ApplyMappingOnly` writes the mapping data asset."));
	Lines.Add(TEXT("- `-ApplyVTAtlasOnly` writes a generated virtual-texture-enabled Texture2D atlas from eligible TSF_BGRA8 source textures."));
	Lines.Add(TEXT("- `-ApplyTextureArraysOnly` writes Texture2DArray assets from eligible slice plans."));
	Lines.Add(TEXT("- `-ApplyPropertyTextureOnly` writes the RGBA16F property texture asset."));
	Lines.Add(TEXT("- `-ApplyProxyMeshOnly` writes a LOD0 merged proxy StaticMesh with UV7.x batchMaterialIndex data."));
	Lines.Add(TEXT("- `-ApplyBatchMaterialOnly` writes a generated material instance and assigns it to the proxy mesh material slot when the proxy mesh exists."));
	Lines.Add(TEXT("- Full `-Apply` and map actor replacement remain disabled until generated proxy meshes are reviewed."));
	Lines.Add(TEXT("- Use `MaterialBatchAudit` first to confirm candidate actors and reject reasons."));
	Lines.Add(TEXT("- Generated assets must stay under `Content/Generated/MaterialBatch/...`."));
	return Lines;
}

FString FMaterialBatchBuildPlanBuilder::BuildJsonManifest(const FMaterialBatchBuildPlan& Plan)
{
	TSharedRef<FJsonObject> RootObject = MakeShared<FJsonObject>();
	RootObject->SetStringField(TEXT("schema"), TEXT("DevKit.MaterialBatchBuildPlan.v1"));
	RootObject->SetBoolField(TEXT("dryRun"), Plan.bDryRun);
	RootObject->SetStringField(TEXT("root"), Plan.Options.RootPath);
	RootObject->SetStringField(TEXT("map"), Plan.Options.MapPath);
	RootObject->SetStringField(TEXT("dataLayer"), Plan.Options.DataLayerName);
	RootObject->SetStringField(TEXT("layerBackend"), NormalizeLayerBackend(Plan.Options.LayerBackend));
	RootObject->SetStringField(TEXT("cluster"), Plan.SanitizedClusterName);
	RootObject->SetStringField(TEXT("tier"), Plan.SanitizedTierName);
	RootObject->SetStringField(TEXT("textureBackend"), Plan.TextureBackend);
	RootObject->SetStringField(TEXT("surfaceKind"), Plan.SurfaceKind);
	RootObject->SetStringField(TEXT("bakePolicy"), Plan.BakePolicy);
	RootObject->SetStringField(TEXT("sourceProxyExclusivityGroup"), Plan.SourceProxyExclusivityGroup);
	RootObject->SetStringField(TEXT("rules"), Plan.Options.RulesPath);
	RootObject->SetStringField(TEXT("outputFolder"), Plan.OutputFolder);
	RootObject->SetNumberField(TEXT("nextBatchMaterialIndex"), Plan.NextBatchMaterialIndex);

	TSharedRef<FJsonObject> SummaryObject = MakeShared<FJsonObject>();
	SummaryObject->SetStringField(TEXT("sourceKind"), Plan.CandidateSummary.SourceKind);
	SummaryObject->SetNumberField(TEXT("sourceFound"), Plan.CandidateSummary.SourceFoundCount);
	SummaryObject->SetNumberField(TEXT("sourceInspected"), Plan.CandidateSummary.SourceInspectedCount);
	SummaryObject->SetNumberField(TEXT("batchCandidates"), Plan.CandidateSummary.BatchCandidateCount);
	SummaryObject->SetNumberField(TEXT("rejected"), Plan.CandidateSummary.RejectedCount);
	RootObject->SetObjectField(TEXT("summary"), SummaryObject);

	TSharedRef<FJsonObject> TagDiagnosticsObject = MakeShared<FJsonObject>();
	TagDiagnosticsObject->SetNumberField(TEXT("actorCount"), Plan.TagDiagnostics.ActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("sourceActorCount"), Plan.TagDiagnostics.SourceActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("proxyActorCount"), Plan.TagDiagnostics.ProxyActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("bakedActorCount"), Plan.TagDiagnostics.BakedActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("excludeActorCount"), Plan.TagDiagnostics.ExcludeActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("sourceProxyConflictActorCount"), Plan.TagDiagnostics.SourceProxyConflictActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("bakeStaticDecalActorCount"), Plan.TagDiagnostics.BakeStaticDecalActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("runtimeDecalActorCount"), Plan.TagDiagnostics.RuntimeDecalActorCount);
	TagDiagnosticsObject->SetNumberField(TEXT("gameplayIndicatorActorCount"), Plan.TagDiagnostics.GameplayIndicatorActorCount);
	TArray<TSharedPtr<FJsonValue>> TagWarningValues;
	TagWarningValues.Reserve(Plan.TagDiagnostics.Warnings.Num());
	for (const FString& Warning : Plan.TagDiagnostics.Warnings)
	{
		TagWarningValues.Add(MakeShared<FJsonValueString>(Warning));
	}
	TagDiagnosticsObject->SetArrayField(TEXT("warnings"), TagWarningValues);
	RootObject->SetObjectField(TEXT("tagDiagnostics"), TagDiagnosticsObject);
	SetSourceProxyLayerPlanJsonField(RootObject, BuildSourceProxyLayerPlan(Plan));
	SetSourceProxyLayerReadinessJsonField(RootObject, BuildSourceProxyLayerReadiness(Plan));
	SetSourceProxyAssetReadinessJsonField(RootObject, BuildSourceProxyAssetReadiness(Plan));
	SetSourceProxyAssetConfigSetJsonField(RootObject, BuildSourceProxyAssetConfigSet(Plan));

	TSharedRef<FJsonObject> PackagesObject = MakeShared<FJsonObject>();
	PackagesObject->SetStringField(TEXT("proxyMesh"), Plan.ProxyMeshPackage);
	PackagesObject->SetStringField(TEXT("batchMaterialInstance"), Plan.BatchMaterialInstancePackage);
	PackagesObject->SetStringField(TEXT("batchParentMaterial"), Plan.BatchParentMaterialPackage);
	PackagesObject->SetStringField(TEXT("vtAtlas"), Plan.VTAtlasPackage);
	PackagesObject->SetStringField(TEXT("baseColorArray"), Plan.BaseColorArrayPackage);
	PackagesObject->SetStringField(TEXT("normalArray"), Plan.NormalArrayPackage);
	PackagesObject->SetStringField(TEXT("ormArray"), Plan.OrmArrayPackage);
	PackagesObject->SetStringField(TEXT("emissiveArray"), Plan.EmissiveArrayPackage);
	PackagesObject->SetStringField(TEXT("maskArray"), Plan.MaskArrayPackage);
	PackagesObject->SetStringField(TEXT("propertyTexture"), Plan.PropertyTexturePackage);
	PackagesObject->SetStringField(TEXT("mappingDataAsset"), Plan.MappingDataAssetPackage);
	RootObject->SetObjectField(TEXT("packages"), PackagesObject);

	TSharedRef<FJsonObject> BatchMaterialContractObject = MakeShared<FJsonObject>();
	BatchMaterialContractObject->SetStringField(TEXT("parentMaterial"), Plan.BatchParentMaterialPackage);
	BatchMaterialContractObject->SetStringField(TEXT("materialIndexChannel"), Plan.BatchMaterialIndexChannel);
	BatchMaterialContractObject->SetStringField(TEXT("materialIndexSource"), Plan.BatchMaterialIndexSource);
	BatchMaterialContractObject->SetStringField(TEXT("propertyTextureParameter"), Plan.BatchPropertyTextureParameterName);
	BatchMaterialContractObject->SetStringField(TEXT("propertyTextureRowEncoding"), Plan.BatchPropertyTextureRowEncoding);
	BatchMaterialContractObject->SetStringField(TEXT("textureBackend"), Plan.TextureBackend);
	BatchMaterialContractObject->SetStringField(TEXT("vtAtlasParameter"), TEXT("VT_Atlas"));
	TArray<TSharedPtr<FJsonValue>> RequiredArrayParameters;
	for (const TCHAR* ParameterName : { TEXT("T_Array_A"), TEXT("T_Array_N"), TEXT("T_Array_M") })
	{
		RequiredArrayParameters.Add(MakeShared<FJsonValueString>(ParameterName));
	}
	BatchMaterialContractObject->SetArrayField(TEXT("fallbackTexture2DArrayParameters"), RequiredArrayParameters);
	RootObject->SetObjectField(TEXT("batchMaterialContract"), BatchMaterialContractObject);

	TArray<TSharedPtr<FJsonValue>> EntryValues;
	EntryValues.Reserve(Plan.PlannedEntries.Num());
	for (const FMaterialBatchBuildPlannedEntry& Entry : Plan.PlannedEntries)
	{
		TSharedRef<FJsonObject> EntryObject = MakeShared<FJsonObject>();
		EntryObject->SetStringField(TEXT("sourceKind"), Entry.SourceKind);
		EntryObject->SetStringField(TEXT("sourcePath"), Entry.SourcePath);
		EntryObject->SetStringField(TEXT("actor"), Entry.ActorName);
		EntryObject->SetStringField(TEXT("component"), Entry.ComponentName);
		EntryObject->SetStringField(TEXT("asset"), Entry.AssetPath);
		EntryObject->SetStringField(TEXT("actualStreamingLevel"), Entry.ActualStreamingLevelName);
		EntryObject->SetStringField(TEXT("actualLevelPackage"), Entry.ActualLevelPackageName);
		EntryObject->SetNumberField(TEXT("materialSlots"), Entry.MaterialSlotCount);
		EntryObject->SetNumberField(TEXT("lods"), Entry.LodCount);
		TArray<TSharedPtr<FJsonValue>> EnvBatchTagValues;
		EnvBatchTagValues.Reserve(Entry.EnvBatchTags.Num());
		for (const FString& EnvBatchTag : Entry.EnvBatchTags)
		{
			EnvBatchTagValues.Add(MakeShared<FJsonValueString>(EnvBatchTag));
		}
		EntryObject->SetArrayField(TEXT("envBatchTags"), EnvBatchTagValues);
		TArray<TSharedPtr<FJsonValue>> ActualLayerValues;
		ActualLayerValues.Reserve(Entry.ActualLayerNames.Num());
		for (const FString& ActualLayerName : Entry.ActualLayerNames)
		{
			ActualLayerValues.Add(MakeShared<FJsonValueString>(ActualLayerName));
		}
		EntryObject->SetArrayField(TEXT("actualLayers"), ActualLayerValues);
		TArray<TSharedPtr<FJsonValue>> ActualDataLayerValues;
		ActualDataLayerValues.Reserve(Entry.ActualDataLayerNames.Num());
		for (const FString& ActualDataLayerName : Entry.ActualDataLayerNames)
		{
			ActualDataLayerValues.Add(MakeShared<FJsonValueString>(ActualDataLayerName));
		}
		EntryObject->SetArrayField(TEXT("actualDataLayers"), ActualDataLayerValues);
		EntryObject->SetBoolField(TEXT("candidate"), Entry.bCandidate);
		EntryObject->SetStringField(TEXT("rejectReason"), Entry.RejectReason.IsEmpty() ? TEXT("None") : Entry.RejectReason);
		EntryObject->SetNumberField(TEXT("firstBatchMaterialIndex"), Entry.FirstBatchMaterialIndex);
		EntryObject->SetNumberField(TEXT("batchMaterialIndexCount"), Entry.BatchMaterialIndexCount);
		EntryValues.Add(MakeShared<FJsonValueObject>(EntryObject));
	}
	RootObject->SetArrayField(TEXT("entries"), EntryValues);

	const TArray<FMaterialBatchBuildGeometryMergeSourcePlan> GeometryMergeSources =
		BuildGeometryMergeSourcePlans(Plan.PlannedEntries);
	SetGeometryMergePlanJsonField(RootObject, Plan, GeometryMergeSources);

	TArray<TSharedPtr<FJsonValue>> MaterialRowValues;
	MaterialRowValues.Reserve(Plan.PlannedMaterialRows.Num());
	for (const FMaterialBatchBuildMaterialRow& Row : Plan.PlannedMaterialRows)
	{
		TSharedRef<FJsonObject> RowObject = MakeShared<FJsonObject>();
		RowObject->SetNumberField(TEXT("batchMaterialIndex"), Row.BatchMaterialIndex);
		RowObject->SetNumberField(TEXT("sourceEntryIndex"), Row.SourceEntryIndex);
		RowObject->SetNumberField(TEXT("materialSlotIndex"), Row.MaterialSlotIndex);
		RowObject->SetStringField(TEXT("sourceKind"), Row.SourceKind);
		RowObject->SetStringField(TEXT("sourcePath"), Row.SourcePath);
		RowObject->SetStringField(TEXT("actor"), Row.ActorName);
		RowObject->SetStringField(TEXT("component"), Row.ComponentName);
		RowObject->SetStringField(TEXT("asset"), Row.AssetPath);
		RowObject->SetStringField(TEXT("materialSlotName"), Row.MaterialSlotName);
		RowObject->SetStringField(TEXT("material"), Row.MaterialPath);

		TArray<TSharedPtr<FJsonValue>> TextureChannelValues;
		TextureChannelValues.Reserve(Row.TextureChannels.Num());
		for (const FMaterialBatchBuildTextureChannelPlan& Channel : Row.TextureChannels)
		{
			const FMaterialBatchBuildTextureChannelPlan EvaluatedChannel = WithEvaluatedTextureArrayEligibility(Channel);
			TSharedRef<FJsonObject> ChannelObject = MakeShared<FJsonObject>();
			ChannelObject->SetStringField(TEXT("channel"), EvaluatedChannel.ChannelName);
			ChannelObject->SetStringField(TEXT("parameter"), EvaluatedChannel.ParameterName);
			ChannelObject->SetStringField(TEXT("texture"), EvaluatedChannel.TexturePath);
			ChannelObject->SetStringField(TEXT("textureClass"), EvaluatedChannel.TextureClass);
			ChannelObject->SetBoolField(TEXT("found"), EvaluatedChannel.bFoundTexture);
			ChannelObject->SetNumberField(TEXT("width"), EvaluatedChannel.TextureWidth);
			ChannelObject->SetNumberField(TEXT("height"), EvaluatedChannel.TextureHeight);
			ChannelObject->SetBoolField(TEXT("arrayBuildEligible"), EvaluatedChannel.bTextureArrayBuildEligible);
			ChannelObject->SetStringField(TEXT("arrayBuildReason"), EvaluatedChannel.TextureArrayBuildReason);
			TextureChannelValues.Add(MakeShared<FJsonValueObject>(ChannelObject));
		}
		RowObject->SetArrayField(TEXT("textureChannels"), TextureChannelValues);

		MaterialRowValues.Add(MakeShared<FJsonValueObject>(RowObject));
	}
	RootObject->SetArrayField(TEXT("materialRows"), MaterialRowValues);

	FMaterialBatchBuildTextureArrayPlans TextureArrayPlans;
	TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows = BuildPropertyRowPlans(
		Plan.PlannedMaterialRows,
		TextureArrayPlans);

	TSharedRef<FJsonObject> TextureArraysObject = MakeShared<FJsonObject>();
	SetTextureArrayJsonField(TextureArraysObject, TEXT("baseColor"), TextureArrayPlans.BaseColor);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("normal"), TextureArrayPlans.Normal);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("orm"), TextureArrayPlans.Orm);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("emissive"), TextureArrayPlans.Emissive);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("mask"), TextureArrayPlans.Mask);
	RootObject->SetObjectField(TEXT("textureArrays"), TextureArraysObject);

	const TArray<FMaterialBatchBuildVTAtlasEntry> VTAtlasEntries = BuildVTAtlasEntries(Plan);
	const FMaterialBatchBuildVTAtlasPayload VTAtlasPayload = BuildVTAtlasPayload(Plan);
	const float EstimatedStreamingPoolMB = SumTextureArraySourceMB(Plan);
	const float EstimatedVTPoolMB = SumVTAtlasSourceMB(VTAtlasEntries);
	const bool bDuplicateResidencyRisk = Plan.TextureBackend.Equals(TEXT("VTAtlas"), ESearchCase::IgnoreCase)
		&& EstimatedStreamingPoolMB > 0.f
		&& EstimatedVTPoolMB > 0.f;
	TSharedRef<FJsonObject> VTAtlasObject = MakeShared<FJsonObject>();
	VTAtlasObject->SetStringField(TEXT("package"), Plan.VTAtlasPackage);
	VTAtlasObject->SetStringField(TEXT("channel"), Plan.VTAtlasChannel);
	VTAtlasObject->SetStringField(TEXT("layoutPolicy"), TEXT("DeterministicGridByUniqueTexture"));
	VTAtlasObject->SetStringField(TEXT("virtualTextureLayout"), VTAtlasPayload.VirtualTextureLayout);
	VTAtlasObject->SetNumberField(TEXT("entryCount"), VTAtlasEntries.Num());
	VTAtlasObject->SetNumberField(TEXT("width"), VTAtlasPayload.Width);
	VTAtlasObject->SetNumberField(TEXT("height"), VTAtlasPayload.Height);
	VTAtlasObject->SetNumberField(TEXT("columns"), VTAtlasPayload.Columns);
	VTAtlasObject->SetNumberField(TEXT("rows"), VTAtlasPayload.Rows);
	VTAtlasObject->SetNumberField(TEXT("tilePaddingPixels"), VTAtlasPayload.TilePaddingPixels);
	VTAtlasObject->SetNumberField(TEXT("estimatedVTPoolMB"), EstimatedVTPoolMB);
	VTAtlasObject->SetNumberField(TEXT("estimatedStreamingPoolMB"), EstimatedStreamingPoolMB);
	VTAtlasObject->SetBoolField(TEXT("duplicateResidencyRisk"), bDuplicateResidencyRisk);
	TArray<TSharedPtr<FJsonValue>> VTAtlasEntryValues;
	VTAtlasEntryValues.Reserve(VTAtlasEntries.Num());
	for (const FMaterialBatchBuildVTAtlasEntry& AtlasEntry : VTAtlasEntries)
	{
		TSharedRef<FJsonObject> AtlasEntryObject = MakeShared<FJsonObject>();
		AtlasEntryObject->SetNumberField(TEXT("atlasEntryIndex"), AtlasEntry.AtlasEntryIndex);
		AtlasEntryObject->SetStringField(TEXT("channel"), AtlasEntry.ChannelName);
		AtlasEntryObject->SetStringField(TEXT("texture"), AtlasEntry.TexturePath);
		AtlasEntryObject->SetStringField(TEXT("textureClass"), AtlasEntry.TextureClass);
		AtlasEntryObject->SetNumberField(TEXT("width"), AtlasEntry.Width);
		AtlasEntryObject->SetNumberField(TEXT("height"), AtlasEntry.Height);
		AtlasEntryObject->SetStringField(TEXT("virtualTextureLayout"), AtlasEntry.VirtualTextureLayout);
		AtlasEntryObject->SetNumberField(TEXT("udimNumber"), AtlasEntry.UdimNumber);
		AtlasEntryObject->SetNumberField(TEXT("tileU"), AtlasEntry.TileU);
		AtlasEntryObject->SetNumberField(TEXT("tileV"), AtlasEntry.TileV);
		AtlasEntryObject->SetNumberField(TEXT("tilePaddingPixels"), AtlasEntry.TilePaddingPixels);
		AtlasEntryObject->SetStringField(TEXT("uvRemapStatus"), AtlasEntry.UVRemapStatus);
		SetVector2DJsonField(AtlasEntryObject, TEXT("uvRectMin"), AtlasEntry.UVRectMin);
		SetVector2DJsonField(AtlasEntryObject, TEXT("uvRectMax"), AtlasEntry.UVRectMax);
		AtlasEntryObject->SetNumberField(TEXT("estimatedSourceMB"), AtlasEntry.EstimatedSourceMB);
		VTAtlasEntryValues.Add(MakeShared<FJsonValueObject>(AtlasEntryObject));
	}
	VTAtlasObject->SetArrayField(TEXT("entries"), VTAtlasEntryValues);
	RootObject->SetObjectField(TEXT("vtAtlas"), VTAtlasObject);
	SetResidencyRiskPlanJsonField(RootObject, BuildResidencyRiskPlan(Plan));
	ApplyVTAtlasRectsToPropertyRows(VTAtlasEntries, TextureArrayPlans, PropertyRows);
	SetPropertyTextureLayoutJsonField(RootObject, PropertyRows.Num());

	TArray<TSharedPtr<FJsonValue>> PropertyRowValues;
	PropertyRowValues.Reserve(PropertyRows.Num());
	for (const FMaterialBatchBuildPropertyRowPlan& PropertyRow : PropertyRows)
	{
		TSharedRef<FJsonObject> PropertyRowObject = MakeShared<FJsonObject>();
		PropertyRowObject->SetNumberField(TEXT("batchMaterialIndex"), PropertyRow.BatchMaterialIndex);
		PropertyRowObject->SetStringField(TEXT("material"), PropertyRow.MaterialPath);
		PropertyRowObject->SetNumberField(TEXT("baseColorSlice"), PropertyRow.BaseColorSliceIndex);
		PropertyRowObject->SetNumberField(TEXT("normalSlice"), PropertyRow.NormalSliceIndex);
		PropertyRowObject->SetNumberField(TEXT("ormSlice"), PropertyRow.OrmSliceIndex);
		PropertyRowObject->SetNumberField(TEXT("emissiveSlice"), PropertyRow.EmissiveSliceIndex);
		PropertyRowObject->SetNumberField(TEXT("maskSlice"), PropertyRow.MaskSliceIndex);
		PropertyRowObject->SetStringField(TEXT("lightInfoTexture"), PropertyRow.LightInfoTexturePath);
		PropertyRowObject->SetStringField(TEXT("sourceTexture"), PropertyRow.SourceTexturePath);
		SetVector2DJsonField(PropertyRowObject, TEXT("uvRectMin"), PropertyRow.UVRectMin);
		SetVector2DJsonField(PropertyRowObject, TEXT("uvRectMax"), PropertyRow.UVRectMax);
		PropertyRowObject->SetStringField(TEXT("normalSourceTexture"), PropertyRow.NormalSourceTexturePath);
		SetVector2DJsonField(PropertyRowObject, TEXT("normalUVRectMin"), PropertyRow.NormalUVRectMin);
		SetVector2DJsonField(PropertyRowObject, TEXT("normalUVRectMax"), PropertyRow.NormalUVRectMax);
		PropertyRowObject->SetStringField(TEXT("ormSourceTexture"), PropertyRow.OrmSourceTexturePath);
		SetVector2DJsonField(PropertyRowObject, TEXT("ormUVRectMin"), PropertyRow.OrmUVRectMin);
		SetVector2DJsonField(PropertyRowObject, TEXT("ormUVRectMax"), PropertyRow.OrmUVRectMax);
		PropertyRowObject->SetStringField(TEXT("surfaceKind"), Plan.SurfaceKind);
		PropertyRowObject->SetStringField(TEXT("bakePolicy"), Plan.BakePolicy);
		PropertyRowObject->SetStringField(TEXT("materialQualityFlags"), Plan.SanitizedTierName);
		PropertyRowValues.Add(MakeShared<FJsonValueObject>(PropertyRowObject));
	}
	RootObject->SetArrayField(TEXT("propertyRows"), PropertyRowValues);

	FString Output;
	TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer =
		TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&Output);
	FJsonSerializer::Serialize(RootObject, Writer);
	return Output;
}

FString FMaterialBatchBuildPlanBuilder::SanitizePackageSegment(const FString& Segment, const FString& Fallback)
{
	FString Trimmed = Segment;
	Trimmed.TrimStartAndEndInline();
	if (Trimmed.IsEmpty())
	{
		Trimmed = Fallback;
	}

	FString Result;
	Result.Reserve(Trimmed.Len());
	bool bLastWasUnderscore = false;
	for (const TCHAR Character : Trimmed)
	{
		const bool bKeep = FChar::IsAlnum(Character) || Character == TEXT('_');
		if (bKeep)
		{
			Result.AppendChar(Character);
			bLastWasUnderscore = false;
		}
		else if (!bLastWasUnderscore)
		{
			Result.AppendChar(TEXT('_'));
			bLastWasUnderscore = true;
		}
	}

	while (Result.StartsWith(TEXT("_")))
	{
		Result.RightChopInline(1);
	}
	while (Result.EndsWith(TEXT("_")))
	{
		Result.LeftChopInline(1);
	}

	return Result.IsEmpty() ? Fallback : Result;
}

FString FMaterialBatchBuildPlanBuilder::NormalizeOutputRoot(const FString& OutputRoot)
{
	FString Result = OutputRoot;
	Result.TrimStartAndEndInline();
	if (Result.IsEmpty())
	{
		Result = TEXT("/Game/Generated/MaterialBatch");
	}
	while (Result.EndsWith(TEXT("/")))
	{
		Result.LeftChopInline(1);
	}
	return Result;
}
