#include "MaterialBatch/MaterialPerformanceTemplateSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/CommandletReportUtils.h"
#include "FileHelpers.h"
#include "MaterialDomain.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"

namespace
{
constexpr const TCHAR* ReportFileName = TEXT("MaterialPerformanceTemplateSetupReport.md");
constexpr const TCHAR* SourceParentMaterialPath = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_MasterA_Source");
constexpr const TCHAR* BakedVTAtlasMaterialPath = TEXT("/Game/Art/Material/EnvMaterial/Main/M_Env_Baked_VTAtlas");
constexpr const TCHAR* TierInstanceRoot = TEXT("/Game/Art/Material/EnvMaterial/Instances");
constexpr const TCHAR* DefaultTexturePath = TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture");
constexpr const TCHAR* DefaultNormalTexturePath = TEXT("/Engine/EngineResources/DefaultNormal.DefaultNormal");

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

UMaterialExpressionTextureSampleParameter2D* AddTextureSampleParameter(
	UMaterial* Material,
	const FName ParameterName,
	UTexture* DefaultTexture,
	EMaterialSamplerType SamplerType,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionTextureSampleParameter2D* Parameter =
		AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->Texture = DefaultTexture;
		Parameter->SamplerType = SamplerType;
		Parameter->Group = TEXT("Source Material Contract");
	}
	return Parameter;
}

UMaterialExpressionScalarParameter* AddScalarParameter(
	UMaterial* Material,
	const FName ParameterName,
	float DefaultValue,
	int32 NodeX,
	int32 NodeY,
	const FName Group = TEXT("Performance Tier"))
{
	UMaterialExpressionScalarParameter* Parameter =
		AddExpression<UMaterialExpressionScalarParameter>(Material, NodeX, NodeY);
	if (Parameter)
	{
		Parameter->ParameterName = ParameterName;
		Parameter->DefaultValue = DefaultValue;
		Parameter->Group = Group;
	}
	return Parameter;
}

UMaterialExpressionLinearInterpolate* AddLerp(
	UMaterial* Material,
	UMaterialExpression* A,
	UMaterialExpression* B,
	UMaterialExpression* Alpha,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionLinearInterpolate* Lerp = AddExpression<UMaterialExpressionLinearInterpolate>(Material, NodeX, NodeY);
	if (Lerp)
	{
		if (A)
		{
			Lerp->A.Connect(0, A);
		}
		if (B)
		{
			Lerp->B.Connect(0, B);
		}
		if (Alpha)
		{
			Lerp->Alpha.Connect(0, Alpha);
		}
	}
	return Lerp;
}

UMaterialExpressionMultiply* AddMultiply(
	UMaterial* Material,
	UMaterialExpression* A,
	UMaterialExpression* B,
	int32 NodeX,
	int32 NodeY)
{
	UMaterialExpressionMultiply* Multiply = AddExpression<UMaterialExpressionMultiply>(Material, NodeX, NodeY);
	if (Multiply)
	{
		if (A)
		{
			Multiply->A.Connect(0, A);
		}
		if (B)
		{
			Multiply->B.Connect(0, B);
		}
	}
	return Multiply;
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

UMaterial* CreateOrResetSourceMaterial(bool bForce, TArray<FString>& ReportLines, TArray<UPackage*>& DirtyPackages)
{
	const FString PackagePath = SourceParentMaterialPath;
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
		ReportLines.Add(FString::Printf(TEXT("- Found existing `%s`; no changes made without `-Force`."), *ToObjectPath(PackagePath)));
		return Material;
	}

	if (!Material)
	{
		Material = NewObject<UMaterial>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Material);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *ToObjectPath(PackagePath)));
	}
	else
	{
		ReportLines.Add(FString::Printf(TEXT("- Rebuilt `%s`."), *ToObjectPath(PackagePath)));
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

	UTexture* DefaultTexture = LoadObject<UTexture>(nullptr, DefaultTexturePath);
	UTexture* DefaultNormalTexture = LoadObject<UTexture>(nullptr, DefaultNormalTexturePath);

	UMaterialExpressionTextureCoordinate* UV0 = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -1500, -320);
	if (UV0)
	{
		UV0->CoordinateIndex = 0;
	}

	UMaterialExpressionTextureSampleParameter2D* BaseColorA =
		AddTextureSampleParameter(Material, TEXT("T_BaseColor_A"), DefaultTexture, SAMPLERTYPE_Color, -1260, -620);
	UMaterialExpressionTextureSampleParameter2D* BaseColorB =
		AddTextureSampleParameter(Material, TEXT("T_BaseColor_B"), DefaultTexture, SAMPLERTYPE_Color, -1260, -480);
	UMaterialExpressionTextureSampleParameter2D* BaseColorC =
		AddTextureSampleParameter(Material, TEXT("T_BaseColor_C"), DefaultTexture, SAMPLERTYPE_Color, -1260, -340);
	UMaterialExpressionTextureSampleParameter2D* NormalA =
		AddTextureSampleParameter(Material, TEXT("T_Normal_A"), DefaultNormalTexture, SAMPLERTYPE_Normal, -1260, -120);
	UMaterialExpressionTextureSampleParameter2D* OrmA =
		AddTextureSampleParameter(Material, TEXT("T_ORM_A"), DefaultTexture, SAMPLERTYPE_Masks, -1260, 100);
	UMaterialExpressionTextureSampleParameter2D* HeightB =
		AddTextureSampleParameter(Material, TEXT("T_Height_B"), DefaultTexture, SAMPLERTYPE_Masks, -1260, 320);
	UMaterialExpressionTextureSampleParameter2D* HeightC =
		AddTextureSampleParameter(Material, TEXT("T_Height_C"), DefaultTexture, SAMPLERTYPE_Masks, -1260, 460);
	UMaterialExpressionTextureSampleParameter2D* MaskA =
		AddTextureSampleParameter(Material, TEXT("T_Mask_A"), DefaultTexture, SAMPLERTYPE_Masks, -1260, 620);
	UMaterialExpressionTextureSampleParameter2D* LightInfo =
		AddTextureSampleParameter(Material, TEXT("T_LightInfo"), DefaultTexture, SAMPLERTYPE_LinearColor, -1260, 800);

	UMaterialExpressionScalarParameter* LayerBWeight =
		AddScalarParameter(Material, TEXT("LayerBWeight"), 1.0f, -980, -520, TEXT("Source Blend"));
	UMaterialExpressionScalarParameter* LayerCWeight =
		AddScalarParameter(Material, TEXT("LayerCWeight"), 1.0f, -980, -380, TEXT("Source Blend"));
	UMaterialExpressionScalarParameter* MaterialQuality =
		AddScalarParameter(Material, TEXT("TierMaterialQuality"), 3.0f, -980, -160);
	UMaterialExpressionScalarParameter* MaxRuntimeBlendLayers =
		AddScalarParameter(Material, TEXT("MaxRuntimeBlendLayers"), 3.0f, -980, -20);
	UMaterialExpressionScalarParameter* DynamicOverlayQuality =
		AddScalarParameter(Material, TEXT("DynamicOverlayQuality"), 3.0f, -980, 120);
	UMaterialExpressionScalarParameter* MaterialLightQuality =
		AddScalarParameter(Material, TEXT("MaterialLightQuality"), 3.0f, -980, 260);
	UMaterialExpressionScalarParameter* MaterialLightMaxLightInfoCount =
		AddScalarParameter(Material, TEXT("MaterialLightMaxLightInfoCount"), 4.0f, -980, 400);
	UMaterialExpressionScalarParameter* LightInfoIntensity =
		AddScalarParameter(Material, TEXT("LightInfoIntensity"), 0.0f, -980, 800, TEXT("Material Light"));
	UMaterialExpressionScalarParameter* UseBakedResult =
		AddScalarParameter(Material, TEXT("UseBakedResult"), 0.0f, -980, 540);

	UMaterialExpressionComponentMask* HeightBMask = AddMask(Material, HeightB, true, false, false, false, -980, 320);
	UMaterialExpressionComponentMask* HeightCMask = AddMask(Material, HeightC, true, false, false, false, -980, 460);
	UMaterialExpressionMultiply* LayerBAlpha = AddMultiply(Material, LayerBWeight, HeightBMask, -740, -520);
	UMaterialExpressionMultiply* LayerCAlpha = AddMultiply(Material, LayerCWeight, HeightCMask, -740, -380);
	UMaterialExpressionLinearInterpolate* BaseAB = AddLerp(Material, BaseColorA, BaseColorB, LayerBAlpha, -500, -560);
	UMaterialExpressionLinearInterpolate* BaseABC = AddLerp(Material, BaseAB, BaseColorC, LayerCAlpha, -260, -520);

	UMaterialExpressionComponentMask* AO = AddMask(Material, OrmA, true, false, false, false, -500, 40);
	UMaterialExpressionComponentMask* Roughness = AddMask(Material, OrmA, false, true, false, false, -500, 140);
	UMaterialExpressionComponentMask* Metallic = AddMask(Material, OrmA, false, false, true, false, -500, 240);
	UMaterialExpressionComponentMask* LightRGB = AddMask(Material, LightInfo, true, true, true, false, -740, 760);
	UMaterialExpressionMultiply* LightScaledByQuality = AddMultiply(Material, LightRGB, MaterialLightQuality, -500, 760);
	UMaterialExpressionMultiply* LightEmissive = AddMultiply(Material, LightScaledByQuality, LightInfoIntensity, -260, 760);

	Data->BaseColor.Connect(0, BaseABC);
	Data->Normal.Connect(0, NormalA);
	Data->AmbientOcclusion.Connect(0, AO);
	Data->Roughness.Connect(0, Roughness);
	Data->Metallic.Connect(0, Metallic);
	Data->EmissiveColor.Connect(0, LightEmissive);

	// Keep these parameters visible in the graph so material instances expose the tier contract even before final feature wiring.
	(void)UV0;
	(void)MaskA;
	(void)MaterialQuality;
	(void)MaxRuntimeBlendLayers;
	(void)DynamicOverlayQuality;
	(void)MaterialLightMaxLightInfoCount;
	(void)UseBakedResult;

	Material->PostEditChange();
	Material->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	return Material;
}

UMaterialInstanceConstant* CreateOrLoadTierInstance(
	const FString& PackagePath,
	UMaterialInterface* Parent,
	TArray<FString>& ReportLines,
	TArray<UPackage*>& DirtyPackages)
{
	UMaterialInstanceConstant* Instance = LoadAssetByPackagePath<UMaterialInstanceConstant>(PackagePath);
	UPackage* Package = Instance ? Instance->GetOutermost() : CreatePackage(*PackagePath);
	if (!Package || !Parent)
	{
		ReportLines.Add(FString::Printf(TEXT("- Skipped `%s`: missing package or parent material."), *PackagePath));
		return nullptr;
	}

	if (!Instance)
	{
		const FString AssetName = FPackageName::GetLongPackageAssetName(PackagePath);
		Instance = NewObject<UMaterialInstanceConstant>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Instance);
		ReportLines.Add(FString::Printf(TEXT("- Created `%s`."), *ToObjectPath(PackagePath)));
	}
	else
	{
		ReportLines.Add(FString::Printf(TEXT("- Updated `%s`."), *ToObjectPath(PackagePath)));
	}

	Instance->Modify();
	Instance->SetParentEditorOnly(Parent);
	Instance->MarkPackageDirty();
	Package->MarkPackageDirty();
	DirtyPackages.AddUnique(Package);
	return Instance;
}

void SetScalar(UMaterialInstanceConstant* Instance, const FName ParameterName, float Value)
{
	if (Instance)
	{
		Instance->SetScalarParameterValueEditorOnly(FMaterialParameterInfo(ParameterName), Value);
		Instance->MarkPackageDirty();
	}
}

void ConfigureTierInstance(
	UMaterialInstanceConstant* Instance,
	float MaterialQuality,
	float MaxBlendLayers,
	float LayerBWeight,
	float LayerCWeight,
	float DynamicOverlayQuality,
	float MaterialLightQuality,
	float MaterialLightMaxLightInfoCount,
	float UseBakedResult)
{
	SetScalar(Instance, TEXT("TierMaterialQuality"), MaterialQuality);
	SetScalar(Instance, TEXT("MaxRuntimeBlendLayers"), MaxBlendLayers);
	SetScalar(Instance, TEXT("LayerBWeight"), LayerBWeight);
	SetScalar(Instance, TEXT("LayerCWeight"), LayerCWeight);
	SetScalar(Instance, TEXT("DynamicOverlayQuality"), DynamicOverlayQuality);
	SetScalar(Instance, TEXT("MaterialLightQuality"), MaterialLightQuality);
	SetScalar(Instance, TEXT("MaterialLightMaxLightInfoCount"), MaterialLightMaxLightInfoCount);
	SetScalar(Instance, TEXT("UseBakedResult"), UseBakedResult);
	SetScalar(Instance, TEXT("LightInfoIntensity"), MaterialLightQuality > 0.0f ? 1.0f : 0.0f);
}

void AppendTemplateEvidence(const UMaterial* Material, TArray<FString>& ReportLines)
{
	bool bHasBaseColorA = false;
	bool bHasBaseColorB = false;
	bool bHasBaseColorC = false;
	bool bHasHeightB = false;
	bool bHasLightInfo = false;
	bool bHasTierQuality = false;
	bool bHasMaxBlendLayers = false;

	if (Material && Material->GetEditorOnlyData())
	{
		for (const TObjectPtr<UMaterialExpression>& ExpressionPtr : Material->GetEditorOnlyData()->ExpressionCollection.Expressions)
		{
			const UMaterialExpression* Expression = ExpressionPtr.Get();
			if (const UMaterialExpressionTextureSampleParameter2D* TextureParameter = Cast<UMaterialExpressionTextureSampleParameter2D>(Expression))
			{
				bHasBaseColorA |= TextureParameter->ParameterName == TEXT("T_BaseColor_A");
				bHasBaseColorB |= TextureParameter->ParameterName == TEXT("T_BaseColor_B");
				bHasBaseColorC |= TextureParameter->ParameterName == TEXT("T_BaseColor_C");
				bHasHeightB |= TextureParameter->ParameterName == TEXT("T_Height_B");
				bHasLightInfo |= TextureParameter->ParameterName == TEXT("T_LightInfo");
			}
			else if (const UMaterialExpressionScalarParameter* ScalarParameter = Cast<UMaterialExpressionScalarParameter>(Expression))
			{
				bHasTierQuality |= ScalarParameter->ParameterName == TEXT("TierMaterialQuality");
				bHasMaxBlendLayers |= ScalarParameter->ParameterName == TEXT("MaxRuntimeBlendLayers");
			}
		}
	}

	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Source Template Evidence"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("| Evidence | Present |"));
	ReportLines.Add(TEXT("| --- | --- |"));
	ReportLines.Add(FString::Printf(TEXT("| `T_BaseColor_A` sample | %s |"), bHasBaseColorA ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| `T_BaseColor_B` sample | %s |"), bHasBaseColorB ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| `T_BaseColor_C` sample | %s |"), bHasBaseColorC ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| `T_Height_B` blend input | %s |"), bHasHeightB ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| `T_LightInfo` material-light input | %s |"), bHasLightInfo ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| `TierMaterialQuality` scalar | %s |"), bHasTierQuality ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("| `MaxRuntimeBlendLayers` scalar | %s |"), bHasMaxBlendLayers ? TEXT("Yes") : TEXT("No")));
}
}

UMaterialPerformanceTemplateSetupCommandlet::UMaterialPerformanceTemplateSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UMaterialPerformanceTemplateSetupCommandlet::Main(const FString& Params)
{
	const bool bApply = FParse::Param(*Params, TEXT("Apply"));
	const bool bForce = FParse::Param(*Params, TEXT("Force"));

	TArray<FString> ReportLines;
	ReportLines.Add(TEXT("# Material Performance Template Setup"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(FString::Printf(TEXT("- Source parent material: `%s`"), *ToObjectPath(SourceParentMaterialPath)));
	ReportLines.Add(FString::Printf(TEXT("- Baked/VT atlas material contract: `%s`"), *ToObjectPath(BakedVTAtlasMaterialPath)));
	ReportLines.Add(FString::Printf(TEXT("- Apply: %s"), bApply ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(FString::Printf(TEXT("- Force: %s"), bForce ? TEXT("Yes") : TEXT("No")));
	ReportLines.Add(TEXT("- Source template approach: Unreal material graph nodes; no project .ush dependency."));
	ReportLines.Add(TEXT("- Tier control surface: material instance scalar parameters plus runtime `FYogMaterialPerformanceTierInterface`."));
	ReportLines.Add(TEXT("- Baked/VT control surface: `TexCoord7.x -> _PropTexture row -> VT_Atlas UV rect` on `M_Env_Baked_VTAtlas`."));

	UMaterial* SourceMaterial = nullptr;
	TArray<UPackage*> DirtyPackages;
	if (bApply)
	{
		SourceMaterial = CreateOrResetSourceMaterial(bForce, ReportLines, DirtyPackages);
		if (SourceMaterial)
		{
			UMaterialInstanceConstant* Epic = CreateOrLoadTierInstance(
				FString::Printf(TEXT("%s/MI_Env_MasterA_Source_Epic"), TierInstanceRoot),
				SourceMaterial,
				ReportLines,
				DirtyPackages);
			UMaterialInstanceConstant* High = CreateOrLoadTierInstance(
				FString::Printf(TEXT("%s/MI_Env_MasterA_Source_High"), TierInstanceRoot),
				SourceMaterial,
				ReportLines,
				DirtyPackages);
			UMaterialInstanceConstant* Mid = CreateOrLoadTierInstance(
				FString::Printf(TEXT("%s/MI_Env_MasterA_Source_Mid"), TierInstanceRoot),
				SourceMaterial,
				ReportLines,
				DirtyPackages);
			UMaterialInstanceConstant* Low = CreateOrLoadTierInstance(
				FString::Printf(TEXT("%s/MI_Env_MasterA_Source_Low"), TierInstanceRoot),
				SourceMaterial,
				ReportLines,
				DirtyPackages);

			ConfigureTierInstance(Epic, 3.0f, 3.0f, 1.0f, 1.0f, 3.0f, 3.0f, 4.0f, 0.0f);
			ConfigureTierInstance(High, 2.0f, 3.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 0.0f);
			ConfigureTierInstance(Mid, 1.0f, 2.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 0.0f);
			ConfigureTierInstance(Low, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
		}

		if (!DirtyPackages.IsEmpty())
		{
			ReportLines.Add(UEditorLoadingAndSavingUtils::SavePackages(DirtyPackages, true)
				? TEXT("- Saved dirty material template packages.")
				: TEXT("- Failed to save one or more material template packages."));
		}
	}
	else
	{
		SourceMaterial = LoadAssetByPackagePath<UMaterial>(SourceParentMaterialPath);
		ReportLines.Add(SourceMaterial
			? TEXT("- Found source parent material in report-only mode.")
			: TEXT("- Missing source parent material in report-only mode. Run with `-Apply` to create it."));
	}

	AppendTemplateEvidence(SourceMaterial, ReportLines);

	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("## Tier Parameter Template"));
	ReportLines.Add(TEXT(""));
	ReportLines.Add(TEXT("| Tier | MaterialQuality | MaxRuntimeBlendLayers | LayerB | LayerC | OverlayQuality | MaterialLightQuality | LightInfoCount | UseBakedResult |"));
	ReportLines.Add(TEXT("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |"));
	ReportLines.Add(TEXT("| Epic | 3 | 3 | 1 | 1 | 3 | 3 | 4 | 0 |"));
	ReportLines.Add(TEXT("| High | 2 | 3 | 1 | 1 | 2 | 2 | 2 | 0 |"));
	ReportLines.Add(TEXT("| Mid | 1 | 2 | 1 | 0 | 1 | 1 | 1 | 0 |"));
	ReportLines.Add(TEXT("| Low | 0 | 1 | 0 | 0 | 0 | 0 | 0 | 1 |"));

	FString ReportPath;
	FString SharedReportPath;
	if (!DevKitEditorCommandletReports::SaveReportLines(ReportFileName, ReportLines, ReportPath, SharedReportPath))
	{
		UE_LOG(LogTemp, Error, TEXT("MaterialPerformanceTemplateSetup could not save reports."));
		return 1;
	}

	UE_LOG(LogTemp, Display, TEXT("MaterialPerformanceTemplateSetup wrote: %s"), *ReportPath);
	UE_LOG(LogTemp, Display, TEXT("MaterialPerformanceTemplateSetup wrote shared report: %s"), *SharedReportPath);
	return SourceMaterial ? 0 : 1;
}
