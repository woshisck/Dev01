#include "MaterialBatch/MaterialBatchParentMaterialSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "FileHelpers.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"

namespace
{
constexpr const TCHAR* ReportFileName = TEXT("MaterialBatchParentMaterialSetupReport.md");
constexpr const TCHAR* BatchParentMaterialPath = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas");
constexpr const TCHAR* DefaultBaseColorArrayPath = TEXT("/Game/Art/Material/EnvMaterial/Texture/T_Replace_Array_A");
constexpr const TCHAR* DefaultNormalArrayPath = TEXT("/Game/Art/Material/EnvMaterial/Texture/T_Replace_Array_N");
constexpr const TCHAR* DefaultOrmArrayPath = TEXT("/Game/Art/Material/EnvMaterial/Texture/T_Replace_Array_M");
constexpr const TCHAR* DefaultVTAtlasPath = TEXT("/Game/Generated/MaterialBatch/Mid/FloorBrick03_Probe/VT_Atlas_FloorBrick03_Probe");
constexpr const TCHAR* DefaultPropertyTexturePath = TEXT("/Game/Generated/MaterialBatch/Mid/FloorBrick03_Probe/T_PropTexture_FloorBrick03_Probe");

FString ToObjectPath(const FString& PackagePath)
{
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *AssetName);
}

template<typename AssetT>
AssetT* LoadAssetByPackagePath(const FString& PackagePath)
{
	return Cast<AssetT>(StaticLoadObject(AssetT::StaticClass(), nullptr, *ToObjectPath(PackagePath), nullptr, LOAD_NoWarn));
}

template<typename ExpressionT>
ExpressionT* AddExpression(UMaterial* Material, int32 NodeX, int32 NodeY)
{
	if (!Material || !Material->GetEditorOnlyData())
	{
		return nullptr;
	}

	ExpressionT* Expression = NewObject<ExpressionT>(Material);
	Expression->MaterialExpressionEditorX = NodeX;
	Expression->MaterialExpressionEditorY = NodeY;
	Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
	return Expression;
}

UMaterialExpressionTextureObjectParameter* AddTextureObjectParameter(
	UMaterial* Material,
	const FName ParameterName,
	UTexture* DefaultTexture,
	EMaterialSamplerType SamplerType,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionTextureObjectParameter* Parameter =
		AddExpression<UMaterialExpressionTextureObjectParameter>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->Texture = DefaultTexture;
		Parameter->SamplerType = SamplerType;
		Parameter->Group = TEXT("Material Batch");
	}
	return Parameter;
}

UMaterialExpressionScalarParameter* AddScalarParameter(
	UMaterial* Material,
	const FName ParameterName,
	float DefaultValue,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionScalarParameter* Parameter =
		AddExpression<UMaterialExpressionScalarParameter>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->DefaultValue = DefaultValue;
		Parameter->Group = TEXT("Material Batch");
	}
	return Parameter;
}

void AddCustomInput(
	UMaterialExpressionCustom* CustomNode,
	const FName InputName,
	UMaterialExpression* InputExpression,
	int32 OutputIndex = 0)
{
	if (!CustomNode)
	{
		return;
	}

	FCustomInput& Input = CustomNode->Inputs.AddDefaulted_GetRef();
	Input.InputName = InputName;
	if (InputExpression)
	{
		Input.Input.Connect(OutputIndex, InputExpression);
	}
}

UMaterialExpressionComponentMask* AddMask(
	UMaterial* Material,
	UMaterialExpression* Source,
	bool bR,
	bool bG,
	bool bB,
	bool bA,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionComponentMask* Mask = AddExpression<UMaterialExpressionComponentMask>(Material, NodeX, NodeY);
	if (Mask)
	{
		Mask->R = bR;
		Mask->G = bG;
		Mask->B = bB;
		Mask->A = bA;
		if (Source)
		{
			Mask->Input.Connect(0, Source);
		}
	}
	return Mask;
}

void AddBatchInputs(
	UMaterialExpressionCustom* CustomNode,
	UMaterialExpression* UV0,
	UMaterialExpression* BatchIndex,
	UMaterialExpression* BaseColorArray,
	UMaterialExpression* NormalArray,
	UMaterialExpression* OrmArray,
	UMaterialExpression* VTAtlas,
	UMaterialExpression* PropertyTexture,
	UMaterialExpression* BatchRowCount,
	UMaterialExpression* PropertyColumnCount)
{
	AddCustomInput(CustomNode, TEXT("UV"), UV0);
	AddCustomInput(CustomNode, TEXT("BatchIndex"), BatchIndex);
	AddCustomInput(CustomNode, TEXT("T_Array_A"), BaseColorArray);
	AddCustomInput(CustomNode, TEXT("T_Array_N"), NormalArray);
	AddCustomInput(CustomNode, TEXT("T_Array_M"), OrmArray);
	AddCustomInput(CustomNode, TEXT("VT_Atlas"), VTAtlas);
	AddCustomInput(CustomNode, TEXT("_PropTexture"), PropertyTexture);
	AddCustomInput(CustomNode, TEXT("BatchRowCount"), BatchRowCount);
	AddCustomInput(CustomNode, TEXT("PropertyColumnCount"), PropertyColumnCount);
}

FString BuildVTAtlasLookupCode(
	float UVMinXColumn,
	float UVMinYColumn,
	float UVMaxXColumn,
	float UVMaxYColumn,
	const TCHAR* ReturnExpression)
{
	return FString::Printf(
		TEXT("float rowCount = max(BatchRowCount, 1.0);\n")
		TEXT("float columnCount = max(PropertyColumnCount, 1.0);\n")
		TEXT("float batch = clamp(floor(BatchIndex + 0.001), 0.0, rowCount - 1.0);\n")
		TEXT("float2 propRowUV = float2(0.0, (batch + 0.5) / rowCount);\n")
		TEXT("propRowUV.x = (%.1f + 0.5) / columnCount;\n")
		TEXT("float uvMinX = Texture2DSample(_PropTexture, _PropTextureSampler, propRowUV).r;\n")
		TEXT("propRowUV.x = (%.1f + 0.5) / columnCount;\n")
		TEXT("float uvMinY = Texture2DSample(_PropTexture, _PropTextureSampler, propRowUV).r;\n")
		TEXT("propRowUV.x = (%.1f + 0.5) / columnCount;\n")
		TEXT("float uvMaxX = Texture2DSample(_PropTexture, _PropTextureSampler, propRowUV).r;\n")
		TEXT("propRowUV.x = (%.1f + 0.5) / columnCount;\n")
		TEXT("float uvMaxY = Texture2DSample(_PropTexture, _PropTextureSampler, propRowUV).r;\n")
		TEXT("float2 atlasUV = lerp(float2(uvMinX, uvMinY), float2(uvMaxX, uvMaxY), saturate(UV));\n")
		TEXT("float4 sampled = Texture2DSample(VT_Atlas, VT_AtlasSampler, atlasUV);\n")
		TEXT("%s"),
		UVMinXColumn,
		UVMinYColumn,
		UVMaxXColumn,
		UVMaxYColumn,
		ReturnExpression);
}

UMaterial* CreateOrResetBatchParentMaterial(bool bForce, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
{
	const FString PackagePath = BatchParentMaterialPath;
	const FString ObjectPath = ToObjectPath(PackagePath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed: could not create package `%s`."), *PackagePath));
		return nullptr;
	}

	UMaterial* Material = LoadAssetByPackagePath<UMaterial>(PackagePath);
	if (Material && !bForce)
	{
		ReportLines.Add(FString::Printf(TEXT("- Found existing `%s`; no changes made without `-Force`."), *ObjectPath));
		return Material;
	}

	if (!Material)
	{
		Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Material);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *ObjectPath));
	}
	else
	{
		ReportLines.Add(FString::Printf(TEXT("- Rebuilt `%s`."), *ObjectPath));
	}

	if (!Material || !Material->GetEditorOnlyData())
	{
		ReportLines.Add(TEXT("- Failed: material editor-only data is unavailable."));
		return nullptr;
	}

	Material->Modify();
	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->ExpressionCollection.Expressions.Empty();
	Data->BaseColor.Expression = nullptr;
	Data->Normal.Expression = nullptr;
	Data->Roughness.Expression = nullptr;
	Data->Metallic.Expression = nullptr;
	Data->AmbientOcclusion.Expression = nullptr;
	Data->EmissiveColor.Expression = nullptr;

	Material->MaterialDomain = MD_Surface;
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TwoSided = false;
	Material->bTangentSpaceNormal = true;

	UTexture* DefaultBaseColorArray = LoadAssetByPackagePath<UTexture>(DefaultBaseColorArrayPath);
	UTexture* DefaultNormalArray = LoadAssetByPackagePath<UTexture>(DefaultNormalArrayPath);
	UTexture* DefaultOrmArray = LoadAssetByPackagePath<UTexture>(DefaultOrmArrayPath);
	UTexture* DefaultVTAtlas = LoadAssetByPackagePath<UTexture>(DefaultVTAtlasPath);
	UTexture* DefaultPropertyTexture = LoadAssetByPackagePath<UTexture>(DefaultPropertyTexturePath);

	UMaterialExpressionTextureCoordinate* UV0 = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -1300, -300);
	if (UV0)
	{
		UV0->CoordinateIndex = 0;
	}

	UMaterialExpressionTextureCoordinate* UV7 = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -1300, -120);
	if (UV7)
	{
		UV7->CoordinateIndex = 7;
	}

	UMaterialExpressionComponentMask* BatchIndex = AddMask(Material, UV7, true, false, false, false, -1060, -120);
	UMaterialExpressionTextureObjectParameter* BaseColorArray = AddTextureObjectParameter(Material, TEXT("T_Array_A"), DefaultBaseColorArray, SAMPLERTYPE_Color, -1300, 120);
	UMaterialExpressionTextureObjectParameter* NormalArray = AddTextureObjectParameter(Material, TEXT("T_Array_N"), DefaultNormalArray, SAMPLERTYPE_Normal, -1300, 260);
	UMaterialExpressionTextureObjectParameter* OrmArray = AddTextureObjectParameter(Material, TEXT("T_Array_M"), DefaultOrmArray, SAMPLERTYPE_Masks, -1300, 400);
	UMaterialExpressionTextureObjectParameter* VTAtlas = AddTextureObjectParameter(Material, TEXT("VT_Atlas"), DefaultVTAtlas, SAMPLERTYPE_LinearColor, -1300, 540);
	UMaterialExpressionTextureObjectParameter* PropertyTexture = AddTextureObjectParameter(Material, TEXT("_PropTexture"), DefaultPropertyTexture, SAMPLERTYPE_LinearColor, -1300, 680);
	UMaterialExpressionScalarParameter* BatchRowCount = AddScalarParameter(Material, TEXT("BatchRowCount"), 24.0f, -1060, 120);
	UMaterialExpressionScalarParameter* PropertyColumnCount = AddScalarParameter(Material, TEXT("PropertyColumnCount"), 17.0f, -1060, 260);

	UMaterialExpressionCustom* BaseColorNode = AddExpression<UMaterialExpressionCustom>(Material, -560, -260);
	if (BaseColorNode)
	{
		BaseColorNode->Description = TEXT("MaterialBatch BaseColor from VT_Atlas UVRect");
		BaseColorNode->OutputType = CMOT_Float3;
		BaseColorNode->ContainsClipInstruction = CMCI_No;
		BaseColorNode->Code = BuildVTAtlasLookupCode(5.0f, 6.0f, 7.0f, 8.0f, TEXT("return sampled.rgb;"));
		AddBatchInputs(BaseColorNode, UV0, BatchIndex, BaseColorArray, NormalArray, OrmArray, VTAtlas, PropertyTexture, BatchRowCount, PropertyColumnCount);
	}

	UMaterialExpressionCustom* NormalNode = AddExpression<UMaterialExpressionCustom>(Material, -560, 40);
	if (NormalNode)
	{
		NormalNode->Description = TEXT("MaterialBatch Normal from VT_Atlas UVRect");
		NormalNode->OutputType = CMOT_Float3;
		NormalNode->ContainsClipInstruction = CMCI_No;
		NormalNode->Code = BuildVTAtlasLookupCode(9.0f, 10.0f, 11.0f, 12.0f, TEXT("return normalize(sampled.xyz * 2.0 - 1.0);"));
		AddBatchInputs(NormalNode, UV0, BatchIndex, BaseColorArray, NormalArray, OrmArray, VTAtlas, PropertyTexture, BatchRowCount, PropertyColumnCount);
	}

	UMaterialExpressionCustom* OrmNode = AddExpression<UMaterialExpressionCustom>(Material, -560, 340);
	if (OrmNode)
	{
		OrmNode->Description = TEXT("MaterialBatch ORM from VT_Atlas UVRect");
		OrmNode->OutputType = CMOT_Float4;
		OrmNode->ContainsClipInstruction = CMCI_No;
		OrmNode->Code = BuildVTAtlasLookupCode(13.0f, 14.0f, 15.0f, 16.0f, TEXT("return sampled;"));
		AddBatchInputs(OrmNode, UV0, BatchIndex, BaseColorArray, NormalArray, OrmArray, VTAtlas, PropertyTexture, BatchRowCount, PropertyColumnCount);
	}

	UMaterialExpressionComponentMask* AmbientOcclusion = AddMask(Material, OrmNode, true, false, false, false, -180, 260);
	UMaterialExpressionComponentMask* Roughness = AddMask(Material, OrmNode, false, true, false, false, -180, 360);
	UMaterialExpressionComponentMask* Metallic = AddMask(Material, OrmNode, false, false, true, false, -180, 460);

	Data->BaseColor.Connect(0, BaseColorNode);
	Data->Normal.Connect(0, NormalNode);
	Data->AmbientOcclusion.Connect(0, AmbientOcclusion);
	Data->Roughness.Connect(0, Roughness);
	Data->Metallic.Connect(0, Metallic);

	Material->PostEditChange();
	Material->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);

	return Material;
}

void AppendMaterialEvidence(const UMaterial* Material, TArray<FString>& ReportLines)
{
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

	if (Material && Material->GetEditorOnlyData())
	{
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
	}

	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Contract Evidence"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("| Evidence | Present |"));
	ReportLines.Add(TEXT("| --- | --- |"));
	ReportLines.Add(FString::Printf(TEXT("| TextureCoordinate index 7 | %s |"), bHasTexCoord7 ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Texture object parameter `T_Array_A` | %s |"), bHasBaseColorArray ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Texture object parameter `T_Array_N` | %s |"), bHasNormalArray ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Texture object parameter `T_Array_M` | %s |"), bHasOrmArray ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Texture object parameter `VT_Atlas` | %s |"), bHasVTAtlas ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Texture object parameter `_PropTexture` | %s |"), bHasPropertyTexture ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Scalar parameter `BatchRowCount` | %s |"), bHasBatchRowCount ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Scalar parameter `PropertyColumnCount` | %s |"), bHasPropertyColumnCount ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Custom HLSL `Texture2DArraySample` | %s |"), bHasTextureArraySample ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Custom HLSL `VT_Atlas` sample | %s |"), bHasVTAtlasSample ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| Custom HLSL `_PropTexture` sample | %s |"), bHasPropertyTextureSample ? TEXT("Yes") : TEXT("No")));
}
}

UMaterialBatchParentMaterialSetupCommandlet::UMaterialBatchParentMaterialSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMaterialBatchParentMaterialSetupCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bForce = FParse::Param(*Params, TEXT("Force"));

	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Material Batch Parent Material Setup"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- Target: `%s`"), *ToObjectPath(BatchParentMaterialPath)));
	ReportLines.Add(FString::Printf(TEXT("- Apply: %s"), bApply ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("- Force: %s"), bForce ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(TEXT("- Contract: `TexCoord7.x -> batchMaterialIndex -> _PropTexture row -> VT_Atlas UVRect`."));

	UMaterial* Material = nullptr;
	TArray<UPackage*> DirtyPackages;
	if (bApply)
	{
		Material = CreateOrResetBatchParentMaterial(bForce, ReportLines, DirtyPackages);
		if (!DirtyPackages.IsEmpty())
		{
			if (UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, true))
			{
				ReportLines.Add(TEXT("- Saved dirty material packages."));
			}
			else
			{
				ReportLines.Add(TEXT("- Failed to save dirty material packages."));
			}
		}
	}
	else
	{
		Material = LoadAssetByPackagePath<UMaterial>(BatchParentMaterialPath);
		ReportLines.Add(Material
			? TEXT("- Found target material in report-only mode.")
			: TEXT("- Missing target material in report-only mode. Run with `-Apply` to create it."));
	}

	AppendMaterialEvidence(Material, ReportLines);

	FString ReportPath;
	FString SharedReportPath;
	if (!DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialBatchParentMaterialSetup could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("MaterialBatchParentMaterialSetup wrote: %s"), *ReportPath);
	UE_LOG(LogTemp, Display, TEXT("MaterialBatchParentMaterialSetup wrote shared report: %s"), *SharedReportPath);
	return Material ? 0 : 1;
}
