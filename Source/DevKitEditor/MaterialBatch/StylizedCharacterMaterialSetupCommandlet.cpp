#include "StylizedCharacterMaterialSetupCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionDeriveNormalZ.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionStylizedCharacterLightingOutput.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Engine/Texture2D.h"

namespace
{
constexpr const TCHAR* MaterialPackagePath = TEXT("/Game/Art/Material/CharacterMaterial/M_StylizedCharacterMaster");
constexpr const TCHAR* DefaultNormalBiasPackagePath = TEXT("/Game/Art/Material/CharacterMaterial/Utility/T_StylizedCharacter_DefaultNormalBias");
constexpr const TCHAR* DefaultMixMapPackagePath = TEXT("/Game/Art/Material/CharacterMaterial/Utility/T_StylizedCharacter_DefaultMixMap");
constexpr const TCHAR* HiddenEmissiveMaterialPackagePath = TEXT("/Game/Art/Material/LightingTools/M_StylizedHiddenEmissive");

FString ToObjectPath(const FString& PackagePath)
{
	return FString::Printf(TEXT("%s.%s"), *PackagePath, *FPackageName::GetLongPackageAssetName(PackagePath));
}

template <typename ExpressionT>
ExpressionT* AddExpression(UMaterial* Material, int32 X, int32 Y)
{
	ExpressionT* Expression = NewObject<ExpressionT>(Material);
	Expression->MaterialExpressionEditorX = X;
	Expression->MaterialExpressionEditorY = Y;
	Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
	return Expression;
}

UMaterialExpressionComponentMask* AddMask(UMaterial* Material, UMaterialExpression* Input, bool R, bool G, bool B, int32 X, int32 Y)
{
	UMaterialExpressionComponentMask* Mask = AddExpression<UMaterialExpressionComponentMask>(Material, X, Y);
	Mask->R = R;
	Mask->G = G;
	Mask->B = B;
	Mask->A = false;
	Mask->Input.Connect(0, Input);
	return Mask;
}

UMaterialExpressionConstant* AddConstant(UMaterial* Material, float Value, int32 X, int32 Y)
{
	UMaterialExpressionConstant* Constant = AddExpression<UMaterialExpressionConstant>(Material, X, Y);
	Constant->R = Value;
	return Constant;
}

void AddComment(UMaterial* Material, const FString& Text, int32 X, int32 Y, int32 Width, int32 Height)
{
	UMaterialExpressionComment* Comment = AddExpression<UMaterialExpressionComment>(Material, X, Y);
	Comment->Text = Text;
	Comment->SizeX = Width;
	Comment->SizeY = Height;
}

UTexture2D* CreateOrUpdatePackedDefault(const FString& PackagePath, const FColor& Color, TArray<UPackage*>& PackagesToSave)
{
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ToObjectPath(PackagePath), nullptr, LOAD_NoWarn);
	UPackage* Package = Texture ? Texture->GetOutermost() : CreatePackage(*PackagePath);
	if (!Package)
	{
		return nullptr;
	}

	if (!Texture)
	{
		Texture = NewObject<UTexture2D>(Package, *FPackageName::GetLongPackageAssetName(PackagePath), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Texture);
	}

	const uint8 Pixel[4] = { Color.B, Color.G, Color.R, Color.A };
	Texture->Modify();
	Texture->Source.Init(1, 1, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixel);
	Texture->SRGB = false;
	Texture->CompressionSettings = TC_Masks;
	Texture->MipGenSettings = TMGS_NoMipmaps;
	Texture->NeverStream = true;
	Texture->AddressX = TA_Clamp;
	Texture->AddressY = TA_Clamp;
	Texture->UpdateResource();
	Texture->MarkPackageDirty();
	Package->MarkPackageDirty();
	PackagesToSave.AddUnique(Package);
	return Texture;
}

UMaterialExpressionTextureSampleParameter2D* AddTextureParameter(
	UMaterial* Material,
	const FName Name,
	UTexture* Texture,
	UMaterialExpression* UV,
	int32 SortPriority,
	int32 X,
	int32 Y)
{
	UMaterialExpressionTextureSampleParameter2D* Parameter = AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, X, Y);
	Parameter->ParameterName = Name;
	Parameter->ExpressionGUID = FGuid::NewGuid();
	Parameter->Group = TEXT("Character Textures");
	Parameter->SortPriority = SortPriority;
	Parameter->Texture = Texture;
	Parameter->SamplerType = Name == TEXT("T_Color") ? SAMPLERTYPE_Color : SAMPLERTYPE_Masks;
	Parameter->Coordinates.Connect(0, UV);
	return Parameter;
}

UMaterial* CreateOrResetMaterial()
{
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *ToObjectPath(MaterialPackagePath), nullptr, LOAD_NoWarn);
	UPackage* Package = Material ? Material->GetOutermost() : CreatePackage(MaterialPackagePath);
	if (!Package)
	{
		return nullptr;
	}

	if (!Material)
	{
		Material = NewObject<UMaterial>(Package, *FPackageName::GetLongPackageAssetName(MaterialPackagePath), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Material);
	}

	Material->Modify();
	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->ExpressionCollection.Expressions.Empty();
	Data->BaseColor.Expression = nullptr;
	Data->Metallic.Expression = nullptr;
	Data->Roughness.Expression = nullptr;
	Data->Normal.Expression = nullptr;
	Data->Specular.Expression = nullptr;
	Data->AmbientOcclusion.Expression = nullptr;
	Data->EmissiveColor.Expression = nullptr;
	Data->Opacity.Expression = nullptr;
	Data->OpacityMask.Expression = nullptr;

	Material->MaterialDomain = MD_Surface;
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_StylizedCharacterLit);
	Material->TwoSided = false;
	Material->bTangentSpaceNormal = true;
	Material->SetUsageByFlag(MATUSAGE_SkeletalMesh, true);
	return Material;
}

UMaterial* CreateOrResetHiddenEmissiveMaterial()
{
	UMaterial* Material = LoadObject<UMaterial>(nullptr, *ToObjectPath(HiddenEmissiveMaterialPackagePath), nullptr, LOAD_NoWarn);
	UPackage* Package = Material ? Material->GetOutermost() : CreatePackage(HiddenEmissiveMaterialPackagePath);
	if (!Package)
	{
		return nullptr;
	}

	if (!Material)
	{
		Material = NewObject<UMaterial>(Package, *FPackageName::GetLongPackageAssetName(HiddenEmissiveMaterialPackagePath), RF_Public | RF_Standalone | RF_Transactional);
		FAssetRegistryModule::AssetCreated(Material);
	}

	Material->Modify();
	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->ExpressionCollection.Expressions.Empty();
	Data->BaseColor.Expression = nullptr;
	Data->Metallic.Expression = nullptr;
	Data->Roughness.Expression = nullptr;
	Data->Normal.Expression = nullptr;
	Data->Specular.Expression = nullptr;
	Data->AmbientOcclusion.Expression = nullptr;
	Data->EmissiveColor.Expression = nullptr;
	Data->Opacity.Expression = nullptr;
	Data->OpacityMask.Expression = nullptr;

	Material->MaterialDomain = MD_Surface;
	Material->BlendMode = BLEND_Opaque;
	Material->SetShadingModel(MSM_Unlit);
	Material->TwoSided = true;
	Material->bTangentSpaceNormal = false;
	return Material;
}

void BuildHiddenEmissiveMaterialGraph(UMaterial* Material)
{
	UMaterialExpressionVectorParameter* Color = AddExpression<UMaterialExpressionVectorParameter>(Material, -520, -100);
	Color->ParameterName = TEXT("Emissive Color");
	Color->ExpressionGUID = FGuid::NewGuid();
	Color->DefaultValue = FLinearColor::White;
	Color->Group = TEXT("Hidden Emissive Source");
	Color->SortPriority = 0;

	UMaterialExpressionScalarParameter* Intensity = AddExpression<UMaterialExpressionScalarParameter>(Material, -520, 100);
	Intensity->ParameterName = TEXT("Emissive Intensity");
	Intensity->ExpressionGUID = FGuid::NewGuid();
	Intensity->DefaultValue = 20.0f;
	Intensity->Group = TEXT("Hidden Emissive Source");
	Intensity->SortPriority = 1;

	UMaterialExpressionMultiply* Emissive = AddExpression<UMaterialExpressionMultiply>(Material, -180, 0);
	Emissive->A.Connect(0, Color);
	Emissive->B.Connect(0, Intensity);
	Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Emissive);

	AddComment(Material, TEXT("The source mesh is hidden in game, but remains in the Lumen scene through Affect Indirect Lighting While Hidden."), -620, -260, 940, 120);
}

void BuildMaterialGraph(UMaterial* Material, UTexture* DefaultColor, UTexture* DefaultNormalBias, UTexture* DefaultMixMap)
{
	UMaterialExpressionTextureCoordinate* UV = AddExpression<UMaterialExpressionTextureCoordinate>(Material, -1500, -250);
	UV->CoordinateIndex = 0;

	UMaterialExpressionTextureSampleParameter2D* Color = AddTextureParameter(Material, TEXT("T_Color"), DefaultColor, UV, 0, -1200, -600);
	UMaterialExpressionTextureSampleParameter2D* NormalBias = AddTextureParameter(Material, TEXT("T_Normal"), DefaultNormalBias, UV, 1, -1200, -180);
	UMaterialExpressionTextureSampleParameter2D* MixMap = AddTextureParameter(Material, TEXT("T_MixMap"), DefaultMixMap, UV, 2, -1200, 360);

	UMaterialExpressionComponentMask* ColorRGB = AddMask(Material, Color, true, true, true, -820, -600);
	UMaterialExpressionComponentMask* NormalXY = AddMask(Material, NormalBias, true, true, false, -820, -180);
	UMaterialExpressionComponentMask* DiffuseBias = AddMask(Material, NormalBias, false, false, true, -820, 20);
	UMaterialExpressionComponentMask* SpecMask = AddMask(Material, MixMap, true, false, false, -820, 340);
	UMaterialExpressionComponentMask* Glossiness = AddMask(Material, MixMap, false, true, false, -820, 470);
	(void)AddMask(Material, MixMap, false, false, true, -820, 600);

	UMaterialExpressionMultiply* NormalScale = AddExpression<UMaterialExpressionMultiply>(Material, -520, -180);
	NormalScale->A.Connect(0, NormalXY);
	NormalScale->B.Connect(0, AddConstant(Material, 2.0f, -720, -80));
	UMaterialExpressionSubtract* NormalSigned = AddExpression<UMaterialExpressionSubtract>(Material, -280, -180);
	NormalSigned->A.Connect(0, NormalScale);
	NormalSigned->B.Connect(0, AddConstant(Material, 1.0f, -480, -40));
	UMaterialExpressionDeriveNormalZ* DerivedNormal = AddExpression<UMaterialExpressionDeriveNormalZ>(Material, 0, -180);
	DerivedNormal->InXY.Connect(0, NormalSigned);

	UMaterialExpressionOneMinus* Roughness = AddExpression<UMaterialExpressionOneMinus>(Material, -520, 470);
	Roughness->Input.Connect(0, Glossiness);

	UMaterialExpressionScalarParameter* Metallic = AddExpression<UMaterialExpressionScalarParameter>(Material, -520, 340);
	Metallic->ParameterName = TEXT("Metallic");
	Metallic->ExpressionGUID = FGuid::NewGuid();
	Metallic->DefaultValue = 0.0f;
	Metallic->SliderMin = 0.0f;
	Metallic->SliderMax = 1.0f;
	Metallic->Group = TEXT("PBR Surface");
	Metallic->SortPriority = 0;

	UMaterialExpressionScalarParameter* SpecIntensity = AddExpression<UMaterialExpressionScalarParameter>(Material, -520, 610);
	SpecIntensity->ParameterName = TEXT("SpecIntensity");
	SpecIntensity->ExpressionGUID = FGuid::NewGuid();
	SpecIntensity->DefaultValue = 1.0f;
	SpecIntensity->SliderMin = 0.0f;
	SpecIntensity->SliderMax = 2.0f;
	SpecIntensity->Group = TEXT("Stylized Specular");
	SpecIntensity->SortPriority = 0;

	UMaterialExpressionMultiply* SpecMaskStrength = AddExpression<UMaterialExpressionMultiply>(Material, -280, 550);
	SpecMaskStrength->A.Connect(0, SpecMask);
	SpecMaskStrength->B.Connect(0, SpecIntensity);
	UMaterialExpressionSaturate* Specular = AddExpression<UMaterialExpressionSaturate>(Material, 0, 550);
	Specular->Input.Connect(0, SpecMaskStrength);

	UMaterialExpressionScalarParameter* LightingProfile = AddExpression<UMaterialExpressionScalarParameter>(Material, -420, 820);
	LightingProfile->ParameterName = TEXT("Lighting Profile");
	LightingProfile->ExpressionGUID = FGuid::NewGuid();
	LightingProfile->DefaultValue = 0.0f;
	LightingProfile->Group = TEXT("Stylized Character Lighting");
	LightingProfile->SortPriority = 0;

	UMaterialExpressionStylizedCharacterLightingOutput* StylizedOutput = AddExpression<UMaterialExpressionStylizedCharacterLightingOutput>(Material, -40, 720);
	StylizedOutput->DiffuseBias.Connect(0, DiffuseBias);
	StylizedOutput->LightingProfile.Connect(0, LightingProfile);

	UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
	Data->BaseColor.Connect(0, ColorRGB);
	Data->Metallic.Connect(0, Metallic);
	Data->Roughness.Connect(0, Roughness);
	Data->Normal.Connect(0, DerivedNormal);
	Data->Specular.Connect(0, Specular);

	AddComment(Material, TEXT("T_Color: RGB only. Alpha is intentionally unused."), -1260, -760, 650, 120);
	AddComment(Material, TEXT("T_Normal: RG = packed tangent normal XY; B = Diffuse Bias [0,1], remapped to [-1,1] in MSM_StylizedCharacterLit. Import as Masks, not Normalmap/BC5."), -1260, -330, 1120, 130);
	AddComment(Material, TEXT("T_MixMap: R = SpecMask; G = Glossiness (Roughness = 1-G); B = MatCapMask reserved and currently inactive. Metallic is a separate scalar and defaults to 0."), -1260, 210, 1080, 130);
	AddComment(Material, TEXT("Specular = saturate(T_MixMap.R * SpecIntensity). Specular tint and profile-wide intensity are supplied by the selected Stylized Lighting Profile."), -560, 500, 980, 120);
	AddComment(Material, TEXT("Stylized Character Lighting Output: Diffuse Bias and Lighting Profile only. GI normal is generated automatically from the smooth vertex normal."), -480, 650, 920, 130);
}
}

UStylizedCharacterMaterialSetupCommandlet::UStylizedCharacterMaterialSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UStylizedCharacterMaterialSetupCommandlet::Main(const FString& Params)
{
	if (!FParse::Param(*Params, TEXT("Apply")))
	{
		UE_LOG(LogTemp, Display, TEXT("Run StylizedCharacterMaterialSetup with -Apply to rebuild %s"), MaterialPackagePath);
		return LoadObject<UMaterial>(nullptr, *ToObjectPath(MaterialPackagePath), nullptr, LOAD_NoWarn) ? 0 : 1;
	}

	TArray<UPackage*> PackagesToSave;
	UTexture2D* DefaultNormalBias = CreateOrUpdatePackedDefault(DefaultNormalBiasPackagePath, FColor(128, 128, 128, 255), PackagesToSave);
	UTexture2D* DefaultMixMap = CreateOrUpdatePackedDefault(DefaultMixMapPackagePath, FColor(0, 0, 0, 255), PackagesToSave);
	UTexture* DefaultColor = LoadObject<UTexture>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	UMaterial* Material = CreateOrResetMaterial();
	UMaterial* HiddenEmissiveMaterial = CreateOrResetHiddenEmissiveMaterial();
	if (!Material || !HiddenEmissiveMaterial || !DefaultColor || !DefaultNormalBias || !DefaultMixMap)
	{
		UE_LOG(LogTemp, Error, TEXT("StylizedCharacterMaterialSetup could not create required assets."));
		return 1;
	}

	BuildMaterialGraph(Material, DefaultColor, DefaultNormalBias, DefaultMixMap);
	Material->PostEditChange();
	Material->MarkPackageDirty();
	Material->GetOutermost()->MarkPackageDirty();
	PackagesToSave.AddUnique(Material->GetOutermost());

	BuildHiddenEmissiveMaterialGraph(HiddenEmissiveMaterial);
	HiddenEmissiveMaterial->PostEditChange();
	HiddenEmissiveMaterial->MarkPackageDirty();
	HiddenEmissiveMaterial->GetOutermost()->MarkPackageDirty();
	PackagesToSave.AddUnique(HiddenEmissiveMaterial->GetOutermost());

	const bool bSaved = UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);
	UE_LOG(LogTemp, Display, TEXT("StylizedCharacterMaterialSetup %s %s and %s"), bSaved ? TEXT("rebuilt") : TEXT("failed to save"), *Material->GetPathName(), *HiddenEmissiveMaterial->GetPathName());
	return bSaved ? 0 : 1;
}
