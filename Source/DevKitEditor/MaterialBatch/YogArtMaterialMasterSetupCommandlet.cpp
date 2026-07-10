#include "YogArtMaterialMasterSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "FileHelpers.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureOutput.h"
#include "Materials/MaterialExpressionRuntimeVirtualTextureSampleParameter.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Engine/Texture2D.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "VT/RuntimeVirtualTexture.h"
#include "VT/RuntimeVirtualTextureEnum.h"

namespace
{
constexpr const TCHAR* ReportFileName = TEXT("YogArtMaterialMasterSetupReport.md");

constexpr const TCHAR* DefaultTexturePath = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
constexpr const TCHAR* DefaultNormalTexturePath = TEXT("/Engine/EngineMaterials/DefaultNormal.DefaultNormal");
constexpr const TCHAR* DefaultNoVTColorTexturePath = TEXT("/YogArt_Material/Texture/NoVT/T_Default_White_Color.T_Default_White_Color");
constexpr const TCHAR* DefaultNoVTMaskTexturePath = TEXT("/YogArt_Material/Texture/NoVT/T_Default_White_Mask.T_Default_White_Mask");

constexpr const TCHAR* DefaultGroundRVTPath = TEXT("/YogArt_Material/Data/RVT_Default_Ground");
constexpr const TCHAR* GroundMaterialPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Ground_RVT_Source");
constexpr const TCHAR* BuildingMaterialPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Building_Source");
constexpr const TCHAR* PropMaterialPath = TEXT("/YogArt_Material/MasterMaterial/Env/M_Yog_Prop_Source");

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

UTexture* LoadTextureOrFallback(const TCHAR* PrimaryObjectPath, const TCHAR* FallbackObjectPath)
{
	if (UTexture* Texture = LoadObject<UTexture>(nullptr, PrimaryObjectPath))
	{
		return Texture;
	}
	return LoadObject<UTexture>(nullptr, FallbackObjectPath);
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

bool SetObjectProperty(UObject* Object, const FName PropertyName, UObject* Value)
{
	if (!Object)
	{
		return false;
	}

	if (FObjectProperty* Property = FindFProperty<FObjectProperty>(Object->GetClass(), PropertyName))
	{
		Property->SetObjectPropertyValue_InContainer(Object, Value);
		return true;
	}
	return false;
}

template <typename EnumType>
bool SetEnumProperty(UObject* Object, const FName PropertyName, EnumType Value)
{
	if (!Object)
	{
		return false;
	}

	if (FEnumProperty* Property = FindFProperty<FEnumProperty>(Object->GetClass(), PropertyName))
	{
		Property->GetUnderlyingProperty()->SetIntPropertyValue(Property->ContainerPtrToValuePtr<void>(Object), static_cast<int64>(Value));
		return true;
	}
	if (FByteProperty* ByteProperty = FindFProperty<FByteProperty>(Object->GetClass(), PropertyName))
	{
		ByteProperty->SetPropertyValue_InContainer(Object, static_cast<uint8>(Value));
		return true;
	}
	return false;
}

void AddComment(UMaterial* Material, const FString& Text, int32 NodeX, int32 NodeY, int32 SizeX = 520, int32 SizeY = 140)
{
	UMaterialExpressionComment* Comment = AddExpression<UMaterialExpressionComment>(Material, NodeX, NodeY);
	if (Comment)
	{
		Comment->Text = Text;
		Comment->SizeX = SizeX;
		Comment->SizeY = SizeY;
	}
}

UMaterialExpressionTextureCoordinate* AddUV(UMaterial* Material, int32 CoordinateIndex, int32 NodeX, int32 NodeY)
{
	UMaterialExpressionTextureCoordinate* UV = AddExpression<UMaterialExpressionTextureCoordinate>(Material, NodeX, NodeY);
	if (UV)
	{
		UV->CoordinateIndex = CoordinateIndex;
	}
	return UV;
}

UMaterialExpressionTextureSampleParameter2D* AddTextureSample(
	UMaterial* Material,
	const FName ParameterName,
	UTexture* DefaultTexture,
	EMaterialSamplerType SamplerType,
	UMaterialExpression* UV,
	int32 NodeX,
	int32 NodeY,
	const FName Group)
{
	UMaterialExpressionTextureSampleParameter2D* Parameter =
		AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->Texture = DefaultTexture;
		Parameter->SamplerType = SamplerType;
		Parameter->Group = Group;
		if (UV)
		{
			Parameter->Coordinates.Connect(0, UV);
		}
	}
	return Parameter;
}

UMaterialExpressionScalarParameter* AddScalar(
	UMaterial* Material,
	const FName ParameterName,
	float DefaultValue,
	int32 NodeX,
	int32 NodeY,
	const FName Group)
{
	UMaterialExpressionScalarParameter* Parameter = AddExpression<UMaterialExpressionScalarParameter>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->DefaultValue = DefaultValue;
		Parameter->Group = Group;
	}
	return Parameter;
}

UMaterialExpressionVectorParameter* AddVector(
	UMaterial* Material,
	const FName ParameterName,
	const FLinearColor& DefaultValue,
	int32 NodeX,
	int32 NodeY,
	const FName Group)
{
	UMaterialExpressionVectorParameter* Parameter = AddExpression<UMaterialExpressionVectorParameter>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->DefaultValue = DefaultValue;
		Parameter->Group = Group;
	}
	return Parameter;
}

UMaterialExpressionConstant* AddConstant(UMaterial* Material, float Value, int32 NodeX, int32 NodeY)
{
	UMaterialExpressionConstant* Constant = AddExpression<UMaterialExpressionConstant>(Material, NodeX, NodeY);
	if (Constant)
	{
		Constant->R = Value;
	}
	return Constant;
}

UMaterialExpressionConstant3Vector* AddConstant3(UMaterial* Material, const FLinearColor& Value, int32 NodeX, int32 NodeY)
{
	UMaterialExpressionConstant3Vector* Constant = AddExpression<UMaterialExpressionConstant3Vector>(Material, NodeX, NodeY);
	if (Constant)
	{
		Constant->Constant = Value;
	}
	return Constant;
}

UMaterialExpressionComponentMask* AddMask(
	UMaterial* Material,
	UMaterialExpression* Source,
	bool bR,
	bool bG,
	bool bB,
	bool bA,
	int32 NodeX,
	int32 NodeY,
	int32 OutputIndex = 0)
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
			Mask->Input.Connect(OutputIndex, Source);
		}
	}
	return Mask;
}

UMaterialExpressionMultiply* AddMultiply(
	UMaterial* Material,
	UMaterialExpression* A,
	UMaterialExpression* B,
	int32 NodeX,
	int32 NodeY,
	int32 AOutputIndex = 0,
	int32 BOutputIndex = 0)
{
	UMaterialExpressionMultiply* Multiply = AddExpression<UMaterialExpressionMultiply>(Material, NodeX, NodeY);
	if (Multiply)
	{
		if (A)
		{
			Multiply->A.Connect(AOutputIndex, A);
		}
		if (B)
		{
			Multiply->B.Connect(BOutputIndex, B);
		}
	}
	return Multiply;
}

UMaterialExpressionAdd* AddAdd(
	UMaterial* Material,
	UMaterialExpression* A,
	UMaterialExpression* B,
	int32 NodeX,
	int32 NodeY,
	int32 AOutputIndex = 0,
	int32 BOutputIndex = 0)
{
	UMaterialExpressionAdd* Add = AddExpression<UMaterialExpressionAdd>(Material, NodeX, NodeY);
	if (Add)
	{
		if (A)
		{
			Add->A.Connect(AOutputIndex, A);
		}
		if (B)
		{
			Add->B.Connect(BOutputIndex, B);
		}
	}
	return Add;
}

UMaterialExpressionLinearInterpolate* AddLerp(
	UMaterial* Material,
	UMaterialExpression* A,
	UMaterialExpression* B,
	UMaterialExpression* Alpha,
	int32 NodeX,
	int32 NodeY,
	int32 AOutputIndex = 0,
	int32 BOutputIndex = 0,
	int32 AlphaOutputIndex = 0)
{
	UMaterialExpressionLinearInterpolate* Lerp = AddExpression<UMaterialExpressionLinearInterpolate>(Material, NodeX, NodeY);
	if (Lerp)
	{
		if (A)
		{
			Lerp->A.Connect(AOutputIndex, A);
		}
		if (B)
		{
			Lerp->B.Connect(BOutputIndex, B);
		}
		if (Alpha)
		{
			Lerp->Alpha.Connect(AlphaOutputIndex, Alpha);
		}
	}
	return Lerp;
}

UMaterialExpressionStaticSwitchParameter* AddStaticSwitch(
	UMaterial* Material,
	const FName ParameterName,
	bool bDefaultValue,
	UMaterialExpression* TrueExpression,
	UMaterialExpression* FalseExpression,
	int32 NodeX,
	int32 NodeY,
	const FName Group,
	int32 TrueOutputIndex = 0,
	int32 FalseOutputIndex = 0)
{
	UMaterialExpressionStaticSwitchParameter* Switch = AddExpression<UMaterialExpressionStaticSwitchParameter>(Material, NodeX, NodeY);
	if (Switch)
	{
		Switch->ParameterName = ParameterName;
		Switch->DefaultValue = bDefaultValue;
		Switch->Group = Group;
		Switch->ExpressionGUID = FGuid::NewGuid();
		if (TrueExpression)
		{
			Switch->A.Connect(TrueOutputIndex, TrueExpression);
		}
		if (FalseExpression)
		{
			Switch->B.Connect(FalseOutputIndex, FalseExpression);
		}
	}
	return Switch;
}

UMaterialExpressionRuntimeVirtualTextureSampleParameter* AddRVTSample(
	UMaterial* Material,
	const FName ParameterName,
	URuntimeVirtualTexture* DefaultRVT,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionRuntimeVirtualTextureSampleParameter* Sample =
		AddExpression<UMaterialExpressionRuntimeVirtualTextureSampleParameter>(Material, NodeX, NodeY);
	if (Sample)
	{
		Sample->ParameterName = ParameterName;
		Sample->Group = TEXT("RVT");
		Sample->ExpressionGUID = FGuid::NewGuid();
		Sample->VirtualTexture = DefaultRVT;
		Sample->MaterialType = ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Roughness;
		if (DefaultRVT)
		{
			Sample->InitVirtualTextureDependentSettings();
		}
	}
	return Sample;
}

void ResetSurfaceMaterial(UMaterial* Material)
{
	if (!Material || !Material->GetEditorOnlyData())
	{
		return;
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
	Data->OpacityMask.Expression = nullptr;

	Material->MaterialDomain = MD_Surface;
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_DefaultLit);
	Material->TwoSided = false;
	Material->bTangentSpaceNormal = true;
}

UMaterial* CreateOrResetMaterial(
	const FString& PackagePath,
	bool bForce,
	bool& bOutCanEdit,
	TArray<FString>& ReportLines,
	TArray<UPackage*>& DirtyPackages)
{
	bOutCanEdit = false;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed to create package `%s`."), *PackagePath));
		return nullptr;
	}

	const FString ObjectPath = ToObjectPath(PackagePath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
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

	ResetSurfaceMaterial(Material);
	bOutCanEdit = true;
	Material->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	return Material;
}

URuntimeVirtualTexture* CreateOrResetDefaultRVT(bool bForce, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
{
	const FString PackagePath = DefaultGroundRVTPath;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		ReportLines.Add(FString::Printf(TEXT("- Failed to create package `%s`."), *PackagePath));
		return nullptr;
	}

	const FString ObjectPath = ToObjectPath(PackagePath);
	const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
	URuntimeVirtualTexture* RVT = LoadAssetByPackagePath<URuntimeVirtualTexture>(PackagePath);
	if (RVT && !bForce)
	{
		ReportLines.Add(FString::Printf(TEXT("- Found existing `%s`; no changes made without `-Force`."), *ObjectPath));
		return RVT;
	}

	if (!RVT)
	{
		RVT = NewObject<URuntimeVirtualTexture>(Package, URuntimeVirtualTexture::StaticClass(), *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(RVT);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *ObjectPath));
	}
	else
	{
		ReportLines.Add(FString::Printf(TEXT("- Rebuilt `%s`."), *ObjectPath));
	}

	SetEnumProperty(RVT, TEXT("MaterialType"), ERuntimeVirtualTextureMaterialType::BaseColor_Normal_Roughness);
	RVT->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	return RVT;
}

void AddSharedRVTContractNodes(
	UMaterial* Material,
	URuntimeVirtualTexture* DefaultRVT,
	UMaterialExpression* SourceBaseColor,
	UMaterialExpression*& OutFinalBaseColor,
	int32 NodeX,
	int32 NodeY)
{
	AddComment(Material, TEXT("RVT 接触融合：默认 Static Switch 关闭，开启后读取关卡 Ground RVT 做接地/湿边/污渍等增强。"), NodeX - 40, NodeY - 120, 660, 110);
	UMaterialExpressionRuntimeVirtualTextureSampleParameter* RVTSample =
		AddRVTSample(Material, TEXT("RVT_Ground"), DefaultRVT, NodeX, NodeY);
	UMaterialExpressionScalarParameter* RVTBlend =
		AddScalar(Material, TEXT("RVTContactBlendIntensity"), 0.0f, NodeX, NodeY + 220, TEXT("RVT"));
	UMaterialExpressionMultiply* RVTColorInfluence =
		AddMultiply(Material, SourceBaseColor, RVTSample, NodeX + 260, NodeY + 20, 0, 0);
	UMaterialExpressionLinearInterpolate* RVTBlendResult =
		AddLerp(Material, SourceBaseColor, RVTColorInfluence, RVTBlend, NodeX + 520, NodeY + 20);
	OutFinalBaseColor = AddStaticSwitch(
		Material,
		TEXT("UseRVTContactBlend"),
		false,
		RVTBlendResult,
		SourceBaseColor,
		NodeX + 780,
		NodeY + 20,
		TEXT("RVT"));
}

void ReportGroundRVTSourceTextureRules(UMaterial* GroundMaterial, TArray<FString>& ReportLines)
{
	if (!GroundMaterial || !GroundMaterial->GetEditorOnlyData())
	{
		return;
	}

	int32 VirtualSourceCount = 0;
	for (UMaterialExpression* Expression : GroundMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions)
	{
		const UMaterialExpressionTextureSampleParameter2D* TextureParameter =
			Cast<UMaterialExpressionTextureSampleParameter2D>(Expression);
		if (!TextureParameter || !TextureParameter->ParameterName.ToString().StartsWith(TEXT("T_Ground_")))
		{
			continue;
		}

		const UTexture2D* Texture2D = Cast<UTexture2D>(TextureParameter->Texture);
		if (Texture2D && Texture2D->VirtualTextureStreaming)
		{
			++VirtualSourceCount;
			ReportLines.Add(FString::Printf(
				TEXT("- Warning: `%s` uses VT source texture `%s`. RVT Output cannot sample virtual textures; use a non-VT source texture instead."),
				*TextureParameter->ParameterName.ToString(),
				*Texture2D->GetPathName()));
		}
	}

	if (VirtualSourceCount == 0)
	{
		ReportLines.Add(TEXT("- Ground RVT source texture check: pass. `T_Ground_*` parameters use non-VT Texture2D sources."));
	}
}

UMaterialExpression* AddMaterialLightContract(
	UMaterial* Material,
	UTexture* DefaultTexture,
	UMaterialExpression* UV0,
	int32 NodeX,
	int32 NodeY)
{
	AddComment(Material, TEXT("MaterialLight：默认 Static Switch 关闭，避免无意增加材质灯采样。需要假光照/赛璐璐辅助贴图时在 MI 打开。"), NodeX - 40, NodeY - 120, 700, 110);
	UMaterialExpressionTextureSampleParameter2D* LightInfo =
		AddTextureSample(Material, TEXT("T_MaterialLight"), DefaultTexture, SAMPLERTYPE_LinearColor, UV0, NodeX, NodeY, TEXT("Material Light"));
	UMaterialExpressionVectorParameter* LightTint =
		AddVector(Material, TEXT("MaterialLightTint"), FLinearColor::White, NodeX, NodeY + 180, TEXT("Material Light"));
	UMaterialExpressionScalarParameter* LightIntensity =
		AddScalar(Material, TEXT("MaterialLightIntensity"), 1.0f, NodeX, NodeY + 340, TEXT("Material Light"));
	UMaterialExpressionConstant3Vector* NoEmissive =
		AddConstant3(Material, FLinearColor::Black, NodeX + 360, NodeY + 260);
	UMaterialExpressionMultiply* Tinted =
		AddMultiply(Material, LightInfo, LightTint, NodeX + 260, NodeY + 20);
	UMaterialExpressionMultiply* Scaled =
		AddMultiply(Material, Tinted, LightIntensity, NodeX + 520, NodeY + 20);
	return AddStaticSwitch(
		Material,
		TEXT("UseMaterialLight"),
		false,
		Scaled,
		NoEmissive,
		NodeX + 780,
		NodeY + 20,
		TEXT("Material Light"));
}

void BuildGroundMaterial(UMaterial* Material, UTexture* DefaultColorTexture, UTexture* DefaultMaskTexture, UTexture* DefaultNormalTexture)
{
	UMaterialExpressionTextureCoordinate* UV0 = AddUV(Material, 0, -1700, -420);
	UMaterialExpressionVertexColor* VertexColor = AddExpression<UMaterialExpressionVertexColor>(Material, -1700, -180);
	AddComment(Material, TEXT("地面 Source：VertexColor.R/G 控制 B/C 层混合；输出同一套结果到主材质和 RVT Output。"), -1740, -760, 720, 150);

	UMaterialExpressionTextureSampleParameter2D* BaseA =
		AddTextureSample(Material, TEXT("T_Ground_BaseColor_A"), DefaultColorTexture, SAMPLERTYPE_Color, UV0, -1380, -640, TEXT("Ground Source"));
	UMaterialExpressionTextureSampleParameter2D* BaseB =
		AddTextureSample(Material, TEXT("T_Ground_BaseColor_B"), DefaultColorTexture, SAMPLERTYPE_Color, UV0, -1380, -500, TEXT("Ground Source"));
	UMaterialExpressionTextureSampleParameter2D* BaseC =
		AddTextureSample(Material, TEXT("T_Ground_BaseColor_C"), DefaultColorTexture, SAMPLERTYPE_Color, UV0, -1380, -360, TEXT("Ground Source"));
	UMaterialExpressionTextureSampleParameter2D* Normal =
		AddTextureSample(Material, TEXT("T_Ground_Normal_A"), DefaultNormalTexture, SAMPLERTYPE_Normal, UV0, -1380, -80, TEXT("Ground Source"));
	UMaterialExpressionTextureSampleParameter2D* ORM =
		AddTextureSample(Material, TEXT("T_Ground_ORM_A"), DefaultMaskTexture, SAMPLERTYPE_Masks, UV0, -1380, 120, TEXT("Ground Source"));
	(void)AddTextureSample(Material, TEXT("T_Ground_Mask_A"), DefaultMaskTexture, SAMPLERTYPE_Masks, UV0, -1380, 320, TEXT("Ground Source"));

	UMaterialExpressionComponentMask* VertexR = AddMask(Material, VertexColor, true, false, false, false, -1100, -240);
	UMaterialExpressionComponentMask* VertexG = AddMask(Material, VertexColor, false, true, false, false, -1100, -120);
	UMaterialExpressionScalarParameter* LayerBWeight = AddScalar(Material, TEXT("LayerBWeight"), 1.0f, -1100, 40, TEXT("Ground Blend"));
	UMaterialExpressionScalarParameter* LayerCWeight = AddScalar(Material, TEXT("LayerCWeight"), 1.0f, -1100, 180, TEXT("Ground Blend"));
	UMaterialExpressionMultiply* LayerBAlpha = AddMultiply(Material, VertexR, LayerBWeight, -820, -240);
	UMaterialExpressionMultiply* LayerCAlpha = AddMultiply(Material, VertexG, LayerCWeight, -820, -120);
	UMaterialExpressionLinearInterpolate* BaseAB = AddLerp(Material, BaseA, BaseB, LayerBAlpha, -520, -560);
	UMaterialExpressionLinearInterpolate* BaseABC = AddLerp(Material, BaseAB, BaseC, LayerCAlpha, -260, -500);

	UMaterialExpressionComponentMask* AO = AddMask(Material, ORM, true, false, false, false, -900, 80);
	UMaterialExpressionComponentMask* Roughness = AddMask(Material, ORM, false, true, false, false, -900, 200);
	UMaterialExpressionComponentMask* Metallic = AddMask(Material, ORM, false, false, true, false, -900, 320);

	UMaterialExpressionScalarParameter* RVTOutputOpacity = AddScalar(Material, TEXT("RVTOutputOpacity"), 1.0f, -520, 500, TEXT("Ground RVT"));
	UMaterialExpressionRuntimeVirtualTextureOutput* RVTOutput = AddExpression<UMaterialExpressionRuntimeVirtualTextureOutput>(Material, -120, 280);
	if (RVTOutput)
	{
		RVTOutput->BaseColor.Connect(0, BaseABC);
		RVTOutput->Roughness.Connect(0, Roughness);
		RVTOutput->Normal.Connect(0, Normal);
		RVTOutput->Opacity.Connect(0, RVTOutputOpacity);
	}

	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->BaseColor.Connect(0, BaseABC);
	Data->Normal.Connect(0, Normal);
	Data->AmbientOcclusion.Connect(0, AO);
	Data->Roughness.Connect(0, Roughness);
	Data->Metallic.Connect(0, Metallic);
}

void BuildBuildingMaterial(UMaterial* Material, UTexture* DefaultTexture, UTexture* DefaultMaskTexture, UTexture* DefaultNormalTexture, URuntimeVirtualTexture* DefaultRVT)
{
	UMaterialExpressionTextureCoordinate* UV0 = AddUV(Material, 0, -1700, -420);
	UMaterialExpressionTextureCoordinate* UV1 = AddUV(Material, 1, -1700, -260);
	UMaterialExpressionVertexColor* VertexColor = AddExpression<UMaterialExpressionVertexColor>(Material, -1700, -80);
	AddComment(Material, TEXT("Building Source：墙面/建筑默认不进自动合批；保留 UV0、UV1 与 VertexColor 给 Mesh Paint/UDIM/手动实例化。"), -1740, -760, 760, 150);

	UMaterialExpressionTextureSampleParameter2D* BaseA =
		AddTextureSample(Material, TEXT("T_Building_BaseColor_A"), DefaultTexture, SAMPLERTYPE_Color, UV0, -1380, -640, TEXT("Building Source"));
	UMaterialExpressionTextureSampleParameter2D* BaseB =
		AddTextureSample(Material, TEXT("T_Building_BaseColor_B"), DefaultTexture, SAMPLERTYPE_Color, UV0, -1380, -500, TEXT("Building Source"));
	UMaterialExpressionTextureSampleParameter2D* NormalA =
		AddTextureSample(Material, TEXT("T_Building_Normal_A"), DefaultNormalTexture, SAMPLERTYPE_Normal, UV0, -1380, -220, TEXT("Building Source"));
	(void)AddTextureSample(Material, TEXT("T_Building_Normal_B"), DefaultNormalTexture, SAMPLERTYPE_Normal, UV0, -1380, -80, TEXT("Building Source"));
	UMaterialExpressionTextureSampleParameter2D* ORM =
		AddTextureSample(Material, TEXT("T_Building_ORM_A"), DefaultMaskTexture, SAMPLERTYPE_Masks, UV0, -1380, 120, TEXT("Building Source"));
	(void)AddTextureSample(Material, TEXT("T_Building_Mask_A"), DefaultMaskTexture, SAMPLERTYPE_Masks, UV1, -1380, 320, TEXT("Building Source"));

	UMaterialExpressionComponentMask* VertexR = AddMask(Material, VertexColor, true, false, false, false, -1100, -160);
	UMaterialExpressionScalarParameter* MeshPaintBlendStrength = AddScalar(Material, TEXT("MeshPaintBlendStrength"), 1.0f, -1100, 20, TEXT("Building Blend"));
	UMaterialExpressionMultiply* PaintAlpha = AddMultiply(Material, VertexR, MeshPaintBlendStrength, -820, -160);
	UMaterialExpressionLinearInterpolate* SourceBase = AddLerp(Material, BaseA, BaseB, PaintAlpha, -520, -560);

	UMaterialExpression* FinalBase = SourceBase;
	AddSharedRVTContractNodes(Material, DefaultRVT, SourceBase, FinalBase, -520, 700);

	UMaterialExpressionComponentMask* AO = AddMask(Material, ORM, true, false, false, false, -900, 80);
	UMaterialExpressionComponentMask* Roughness = AddMask(Material, ORM, false, true, false, false, -900, 200);
	UMaterialExpressionComponentMask* Metallic = AddMask(Material, ORM, false, false, true, false, -900, 320);
	UMaterialExpression* Emissive = AddMaterialLightContract(Material, DefaultTexture, UV0, -520, 1180);

	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->BaseColor.Connect(0, FinalBase);
	Data->Normal.Connect(0, NormalA);
	Data->AmbientOcclusion.Connect(0, AO);
	Data->Roughness.Connect(0, Roughness);
	Data->Metallic.Connect(0, Metallic);
	Data->EmissiveColor.Connect(0, Emissive);
}

void BuildPropMaterial(UMaterial* Material, UTexture* DefaultTexture, UTexture* DefaultMaskTexture, UTexture* DefaultNormalTexture, URuntimeVirtualTexture* DefaultRVT)
{
	UMaterialExpressionTextureCoordinate* UV0 = AddUV(Material, 0, -1700, -420);
	UMaterialExpressionTextureCoordinate* UV1 = AddUV(Material, 1, -1700, -260);
	UMaterialExpressionVertexColor* VertexColor = AddExpression<UMaterialExpressionVertexColor>(Material, -1700, -80);
	AddComment(Material, TEXT("Prop Source：静态小物件默认使用普通贴图；Prop.Batched 后续可由工具用 UV1 或 VertexColor.A 写入 TextureCollection/批处理 index。"), -1740, -760, 800, 150);

	UMaterialExpressionTextureSampleParameter2D* Base =
		AddTextureSample(Material, TEXT("T_Prop_BaseColor"), DefaultTexture, SAMPLERTYPE_Color, UV0, -1380, -640, TEXT("Prop Source"));
	UMaterialExpressionTextureSampleParameter2D* Normal =
		AddTextureSample(Material, TEXT("T_Prop_Normal"), DefaultNormalTexture, SAMPLERTYPE_Normal, UV0, -1380, -360, TEXT("Prop Source"));
	UMaterialExpressionTextureSampleParameter2D* ORM =
		AddTextureSample(Material, TEXT("T_Prop_ORM"), DefaultMaskTexture, SAMPLERTYPE_Masks, UV0, -1380, -120, TEXT("Prop Source"));
	(void)AddTextureSample(Material, TEXT("T_Prop_Mask"), DefaultMaskTexture, SAMPLERTYPE_Masks, UV0, -1380, 120, TEXT("Prop Source"));

	UMaterialExpressionComponentMask* UV1Index = AddMask(Material, UV1, true, false, false, false, -1380, 360);
	UMaterialExpressionComponentMask* VertexAlphaIndex = AddMask(Material, VertexColor, false, false, false, true, -1380, 500);
	(void)AddStaticSwitch(Material, TEXT("UseVertexColorAForBatchIndex"), true, VertexAlphaIndex, UV1Index, -1040, 430, TEXT("Batch Index"));
	(void)AddScalar(Material, TEXT("TextureCollectionIndexPreview"), 0.0f, -1040, 620, TEXT("Batch Index"));
	(void)AddScalar(Material, TEXT("BatchMaterialIndex"), 0.0f, -1040, 760, TEXT("Batch Index"));

	UMaterialExpression* FinalBase = Base;
	AddSharedRVTContractNodes(Material, DefaultRVT, Base, FinalBase, -520, 560);

	UMaterialExpressionComponentMask* AO = AddMask(Material, ORM, true, false, false, false, -900, -160);
	UMaterialExpressionComponentMask* Roughness = AddMask(Material, ORM, false, true, false, false, -900, -40);
	UMaterialExpressionComponentMask* Metallic = AddMask(Material, ORM, false, false, true, false, -900, 80);
	UMaterialExpression* Emissive = AddMaterialLightContract(Material, DefaultTexture, UV0, -520, 1040);

	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->BaseColor.Connect(0, FinalBase);
	Data->Normal.Connect(0, Normal);
	Data->AmbientOcclusion.Connect(0, AO);
	Data->Roughness.Connect(0, Roughness);
	Data->Metallic.Connect(0, Metallic);
	Data->EmissiveColor.Connect(0, Emissive);
}

void FinalizeMaterial(UMaterial* Material, TArray<FString>& ReportLines)
{
	if (!Material)
	{
		return;
	}
	Material->PostEditChange();
	Material->MarkPackageDirty();
	ReportLines.Add(FString::Printf(TEXT("- Finalized `%s`."), *Material->GetPathName()));
}
}

UYogArtMaterialMasterSetupCommandlet::UYogArtMaterialMasterSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UYogArtMaterialMasterSetupCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bForce = FParse::Param(*Params, TEXT("Force"));

	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# YogArt Material Master Setup"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- Apply: %s"), bApply ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("- Force: %s"), bForce ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(TEXT("- Scope: YogArt_Material plugin master materials for scene props, buildings, and ground RVT authoring."));
	ReportLines.Add(TEXT("- Default switches: `UseRVTContactBlend=false`, `UseMaterialLight=false` to avoid hidden sampling cost."));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Target Assets"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- Ground RVT source: `%s`"), *ToObjectPath(GroundMaterialPath)));
	ReportLines.Add(FString::Printf(TEXT("- Building source: `%s`"), *ToObjectPath(BuildingMaterialPath)));
	ReportLines.Add(FString::Printf(TEXT("- Prop source: `%s`"), *ToObjectPath(PropMaterialPath)));
	ReportLines.Add(FString::Printf(TEXT("- Default RVT placeholder: `%s`"), *ToObjectPath(DefaultGroundRVTPath)));

	TArray<UPackage*> DirtyPackages;
	URuntimeVirtualTexture* DefaultRVT = LoadAssetByPackagePath<URuntimeVirtualTexture>(DefaultGroundRVTPath);
	UMaterial* GroundMaterial = LoadAssetByPackagePath<UMaterial>(GroundMaterialPath);
	UMaterial* BuildingMaterial = LoadAssetByPackagePath<UMaterial>(BuildingMaterialPath);
	UMaterial* PropMaterial = LoadAssetByPackagePath<UMaterial>(PropMaterialPath);

	if (bApply)
	{
		DefaultRVT = CreateOrResetDefaultRVT(bForce, ReportLines, DirtyPackages);

		UTexture* DefaultTexture = LoadObject<UTexture>(nullptr, DefaultTexturePath);
		UTexture* DefaultNormalTexture = LoadObject<UTexture>(nullptr, DefaultNormalTexturePath);
		UTexture* DefaultNoVTColorTexture = LoadTextureOrFallback(DefaultNoVTColorTexturePath, DefaultTexturePath);
		UTexture* DefaultNoVTMaskTexture = LoadTextureOrFallback(DefaultNoVTMaskTexturePath, DefaultTexturePath);
		UTexture* DefaultNoVTNormalTexture = DefaultNormalTexture;

		bool bCanEditGround = false;
		GroundMaterial = CreateOrResetMaterial(GroundMaterialPath, bForce, bCanEditGround, ReportLines, DirtyPackages);
		if (GroundMaterial && bCanEditGround)
		{
			BuildGroundMaterial(GroundMaterial, DefaultNoVTColorTexture, DefaultNoVTMaskTexture, DefaultNoVTNormalTexture);
			FinalizeMaterial(GroundMaterial, ReportLines);
		}

		bool bCanEditBuilding = false;
		BuildingMaterial = CreateOrResetMaterial(BuildingMaterialPath, bForce, bCanEditBuilding, ReportLines, DirtyPackages);
		if (BuildingMaterial && bCanEditBuilding)
		{
			BuildBuildingMaterial(BuildingMaterial, DefaultTexture, DefaultNoVTMaskTexture, DefaultNormalTexture, DefaultRVT);
			FinalizeMaterial(BuildingMaterial, ReportLines);
		}

		bool bCanEditProp = false;
		PropMaterial = CreateOrResetMaterial(PropMaterialPath, bForce, bCanEditProp, ReportLines, DirtyPackages);
		if (PropMaterial && bCanEditProp)
		{
			BuildPropMaterial(PropMaterial, DefaultTexture, DefaultNoVTMaskTexture, DefaultNormalTexture, DefaultRVT);
			FinalizeMaterial(PropMaterial, ReportLines);
		}

		if (!DirtyPackages.IsEmpty())
		{
			ReportLines.Add(UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, true)
				? TEXT("- Saved dirty YogArt material packages.")
				: TEXT("- Failed to save one or more YogArt material packages."));
		}
	}
	else
	{
		ReportLines.Add(DefaultRVT ? TEXT("- Found default RVT placeholder.") : TEXT("- Missing default RVT placeholder. Run with `-Apply`."));
		ReportLines.Add(GroundMaterial ? TEXT("- Found ground material.") : TEXT("- Missing ground material. Run with `-Apply`."));
		ReportLines.Add(BuildingMaterial ? TEXT("- Found building material.") : TEXT("- Missing building material. Run with `-Apply`."));
		ReportLines.Add(PropMaterial ? TEXT("- Found prop material.") : TEXT("- Missing prop material. Run with `-Apply`."));
	}
	ReportGroundRVTSourceTextureRules(GroundMaterial, ReportLines);

	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Contract Summary"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("- `M_Yog_Ground_RVT_Source`: ground layer blend using VertexColor.R/G and writes BaseColor/Normal/Roughness to RVT Output. Its `T_Ground_*` source textures must keep `VirtualTextureStreaming=false`."));
	ReportLines.Add(TEXT("- `M_Yog_Building_Source`: building/wall source material, keeps Mesh Paint and UV1 support, with optional RVT contact and MaterialLight switches."));
	ReportLines.Add(TEXT("- `M_Yog_Prop_Source`: prop source material, reserves UV1.x or VertexColor.A batch-index inputs for future Texture Collection/batch tools, with optional RVT contact and MaterialLight switches."));

	FString ReportPath;
	FString SharedReportPath;
	if (!DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("YogArtMaterialMasterSetup could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("YogArtMaterialMasterSetup wrote: %s"), *ReportPath);
	UE_LOG(LogTemp, Display, TEXT("YogArtMaterialMasterSetup wrote shared report: %s"), *SharedReportPath);
	return (DefaultRVT && GroundMaterial && BuildingMaterial && PropMaterial) ? 0 : 1;
}
