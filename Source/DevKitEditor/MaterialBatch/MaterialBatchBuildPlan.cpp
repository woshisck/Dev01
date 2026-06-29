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
	static const FString Path = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Building_Batch.M_Env_Building_Batch");
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
		{ 4, TEXT("MaskSlice"), TEXT("maskSlice"), TEXT("int32"), INDEX_NONE, TEXT("Texture2DArray slice index for Mask") }
	};
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
}

FMaterialBatchBuildPlan FMaterialBatchBuildPlanBuilder::CreateDryRunPlan(const FMaterialBatchBuildPlanOptions& Options)
{
	FMaterialBatchBuildPlan Plan;
	Plan.Options = Options;
	Plan.bDryRun = Options.bDryRun;
	Plan.SanitizedClusterName = SanitizePackageSegment(Options.ClusterName, TEXT("Default"));
	Plan.SanitizedTierName = SanitizePackageSegment(Options.TierName, TEXT("Medium"));

	const FString OutputRoot = NormalizeOutputRoot(Options.OutputRoot);
	Plan.OutputFolder = JoinPackagePath(JoinPackagePath(OutputRoot, Plan.SanitizedTierName), Plan.SanitizedClusterName);

	Plan.ProxyMeshPackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("SM_BatchProxy_%s"), *Plan.SanitizedClusterName));
	Plan.BatchMaterialInstancePackage = JoinPackagePath(Plan.OutputFolder, FString::Printf(TEXT("MI_Env_Batch_%s"), *Plan.SanitizedClusterName));
	Plan.BatchParentMaterialPackage = GetDefaultBatchParentMaterialPath();
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
		Plan.BaseColorArrayPackage,
		Plan.NormalArrayPackage,
		Plan.OrmArrayPackage,
		Plan.EmissiveArrayPackage,
		Plan.MaskArrayPackage,
		Plan.PropertyTexturePackage,
		Plan.MappingDataAssetPackage
	};

	return Plan;
}

void FMaterialBatchBuildPlanBuilder::ApplyCandidateSummary(
	FMaterialBatchBuildPlan& Plan,
	const FMaterialBatchBuildCandidateSummary& Summary)
{
	Plan.CandidateSummary = Summary;
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
		TEXT("albedo"),
		TEXT("diffuse"),
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

FMaterialBatchBuildPropertyTexturePayload FMaterialBatchBuildPlanBuilder::BuildPropertyTexturePayload(
	const FMaterialBatchBuildPlan& Plan)
{
	const TArray<FMaterialBatchBuildPropertyTextureColumnPlan> ColumnPlans = GetPropertyTextureColumnPlans();
	const TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows = BuildPropertyRowPlans(Plan.PlannedMaterialRows);

	FMaterialBatchBuildPropertyTexturePayload Payload;
	Payload.Width = ColumnPlans.Num();
	Payload.Height = PropertyRows.Num();
	Payload.bSRGB = false;
	Payload.SourceFormat = ETextureSourceFormat::TSF_RGBA16F;
	Payload.Pixels.SetNum(Payload.Width * Payload.Height);

	auto GetColumnValue = [](const FMaterialBatchBuildPropertyRowPlan& Row, const FString& SourceField) -> int32
	{
		if (SourceField == TEXT("baseColorSlice"))
		{
			return Row.BaseColorSliceIndex;
		}
		if (SourceField == TEXT("normalSlice"))
		{
			return Row.NormalSliceIndex;
		}
		if (SourceField == TEXT("ormSlice"))
		{
			return Row.OrmSliceIndex;
		}
		if (SourceField == TEXT("emissiveSlice"))
		{
			return Row.EmissiveSliceIndex;
		}
		if (SourceField == TEXT("maskSlice"))
		{
			return Row.MaskSliceIndex;
		}
		return INDEX_NONE;
	};

	for (int32 RowIndex = 0; RowIndex < PropertyRows.Num(); ++RowIndex)
	{
		const FMaterialBatchBuildPropertyRowPlan& PropertyRow = PropertyRows[RowIndex];
		for (int32 ColumnIndex = 0; ColumnIndex < ColumnPlans.Num(); ++ColumnIndex)
		{
			const int32 PixelIndex = RowIndex * Payload.Width + ColumnIndex;
			const int32 SliceValue = GetColumnValue(PropertyRow, ColumnPlans[ColumnIndex].SourceField);
			Payload.Pixels[PixelIndex] = FFloat16Color(FLinearColor(static_cast<float>(SliceValue), 0.0f, 0.0f, 0.0f));
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
	Payload.TextureBindings = {
		{ TEXT("T_Array_A"), Plan.BaseColorArrayPackage },
		{ TEXT("T_Array_N"), Plan.NormalArrayPackage },
		{ TEXT("T_Array_M"), Plan.OrmArrayPackage },
		{ Plan.BatchPropertyTextureParameterName, Plan.PropertyTexturePackage }
	};
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
	MappingData.ClusterName = Plan.SanitizedClusterName;
	MappingData.TierName = Plan.SanitizedTierName;
	MappingData.OutputFolder = Plan.OutputFolder;
	MappingData.ProxyMeshPackage = Plan.ProxyMeshPackage;
	MappingData.BatchMaterialInstancePackage = Plan.BatchMaterialInstancePackage;
	MappingData.BatchParentMaterialPackage = Plan.BatchParentMaterialPackage;
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
	const TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows =
		BuildPropertyRowPlans(Plan.PlannedMaterialRows, TextureArrayPlans);

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
	Lines.Add(FString::Printf(TEXT("- DataLayer: `%s`"), Plan.Options.DataLayerName.IsEmpty() ? TEXT("(not set)") : *Plan.Options.DataLayerName));
	Lines.Add(FString::Printf(TEXT("- Cluster: `%s`"), *Plan.Options.ClusterName));
	Lines.Add(FString::Printf(TEXT("- Tier: `%s`"), *Plan.Options.TierName));
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
	Lines.Add(TEXT("| Source | Actor | Component | Asset | Materials | LODs | Status | Reason | FirstBatchMaterialIndex | BatchMaterialIndexCount |"));
	Lines.Add(TEXT("| --- | --- | --- | --- | ---: | ---: | --- | --- | ---: | ---: |"));
	if (Plan.PlannedEntries.IsEmpty())
	{
		Lines.Add(TEXT("| (none) | `` | `` | `` | 0 | 0 | Pending | NotScanned | -1 | 0 |"));
	}
	else
	{
		for (const FMaterialBatchBuildPlannedEntry& Entry : Plan.PlannedEntries)
		{
			Lines.Add(FString::Printf(
				TEXT("| %s | `%s` | `%s` | `%s` | %d | %d | %s | %s | %d | %d |"),
				*Entry.SourceKind,
				Entry.ActorName.IsEmpty() ? TEXT("") : *Entry.ActorName,
				Entry.ComponentName.IsEmpty() ? TEXT("") : *Entry.ComponentName,
				Entry.AssetPath.IsEmpty() ? TEXT("") : *Entry.AssetPath,
				Entry.MaterialSlotCount,
				Entry.LodCount,
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
	Lines.Add(TEXT("- Merge granularity: `cluster`"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("| SourceEntryIndex | Actor | Component | StaticMesh | Location | Rotation | Scale | FirstBatchMaterialIndex | BatchMaterialIndexCount |"));
	Lines.Add(TEXT("| ---: | --- | --- | --- | --- | --- | --- | ---: | ---: |"));
	if (GeometryMergeSources.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | `` | `` | `` | 0,0,0 | 0,0,0 | 1,1,1 | -1 | 0 |"));
	}
	else
	{
		for (const FMaterialBatchBuildGeometryMergeSourcePlan& SourcePlan : GeometryMergeSources)
		{
			Lines.Add(FString::Printf(
				TEXT("| %d | `%s` | `%s` | `%s` | %.3f,%.3f,%.3f | %.3f,%.3f,%.3f | %.3f,%.3f,%.3f | %d | %d |"),
				SourcePlan.SourceEntryIndex,
				SourcePlan.ActorName.IsEmpty() ? TEXT("") : *SourcePlan.ActorName,
				SourcePlan.ComponentName.IsEmpty() ? TEXT("") : *SourcePlan.ComponentName,
				SourcePlan.StaticMeshPath.IsEmpty() ? TEXT("") : *SourcePlan.StaticMeshPath,
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
	const TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows = BuildPropertyRowPlans(
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
	Lines.Add(TEXT("| BatchMaterialIndex | BaseColorSlice | NormalSlice | ORMSlice | EmissiveSlice | MaskSlice | LightInfoTexture | Material |"));
	Lines.Add(TEXT("| ---: | ---: | ---: | ---: | ---: | ---: | --- | --- |"));
	if (PropertyRows.IsEmpty())
	{
		Lines.Add(TEXT("| -1 | -1 | -1 | -1 | -1 | -1 | `` | `` |"));
	}
	else
	{
		for (const FMaterialBatchBuildPropertyRowPlan& PropertyRow : PropertyRows)
		{
			Lines.Add(FString::Printf(
				TEXT("| %d | %d | %d | %d | %d | %d | `%s` | `%s` |"),
				PropertyRow.BatchMaterialIndex,
				PropertyRow.BaseColorSliceIndex,
				PropertyRow.NormalSliceIndex,
				PropertyRow.OrmSliceIndex,
				PropertyRow.EmissiveSliceIndex,
				PropertyRow.MaskSliceIndex,
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
	Lines.Add(TEXT("- Required Texture2DArray parameters: `T_Array_A`, `T_Array_N`, `T_Array_M`."));
	Lines.Add(TEXT("- The parent material must use the property texture row values as Texture2DArray slice indices; binding arrays to the original preview material is not sufficient for production batching."));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("## Current Scope"));
	Lines.Add(TEXT(""));
	Lines.Add(TEXT("- Dry-run mode creates a deterministic build plan only."));
	Lines.Add(TEXT("- `-ApplyMappingOnly` writes the mapping data asset."));
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
	RootObject->SetStringField(TEXT("cluster"), Plan.SanitizedClusterName);
	RootObject->SetStringField(TEXT("tier"), Plan.SanitizedTierName);
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

	TSharedRef<FJsonObject> PackagesObject = MakeShared<FJsonObject>();
	PackagesObject->SetStringField(TEXT("proxyMesh"), Plan.ProxyMeshPackage);
	PackagesObject->SetStringField(TEXT("batchMaterialInstance"), Plan.BatchMaterialInstancePackage);
	PackagesObject->SetStringField(TEXT("batchParentMaterial"), Plan.BatchParentMaterialPackage);
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
	TArray<TSharedPtr<FJsonValue>> RequiredArrayParameters;
	for (const TCHAR* ParameterName : { TEXT("T_Array_A"), TEXT("T_Array_N"), TEXT("T_Array_M") })
	{
		RequiredArrayParameters.Add(MakeShared<FJsonValueString>(ParameterName));
	}
	BatchMaterialContractObject->SetArrayField(TEXT("requiredTexture2DArrayParameters"), RequiredArrayParameters);
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
		EntryObject->SetNumberField(TEXT("materialSlots"), Entry.MaterialSlotCount);
		EntryObject->SetNumberField(TEXT("lods"), Entry.LodCount);
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
	const TArray<FMaterialBatchBuildPropertyRowPlan> PropertyRows = BuildPropertyRowPlans(
		Plan.PlannedMaterialRows,
		TextureArrayPlans);

	TSharedRef<FJsonObject> TextureArraysObject = MakeShared<FJsonObject>();
	SetTextureArrayJsonField(TextureArraysObject, TEXT("baseColor"), TextureArrayPlans.BaseColor);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("normal"), TextureArrayPlans.Normal);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("orm"), TextureArrayPlans.Orm);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("emissive"), TextureArrayPlans.Emissive);
	SetTextureArrayJsonField(TextureArraysObject, TEXT("mask"), TextureArrayPlans.Mask);
	RootObject->SetObjectField(TEXT("textureArrays"), TextureArraysObject);
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
