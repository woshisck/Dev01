#include "YogPlanarFogEditorLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Texture2D.h"
#include "Engine/VolumeTexture.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogYogPlanarFogBuilder, Log, All);

namespace YogPlanarFogMaterial
{
template<class T>
T* Node(UMaterial* Material, int32 X = -600, int32 Y = 0)
{
    T* Expression = NewObject<T>(Material);
    Expression->MaterialExpressionEditorX = X;
    Expression->MaterialExpressionEditorY = Y;
    Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(Expression);
    return Expression;
}

void Input(UMaterialExpressionCustom* Custom, FName Name, UMaterialExpression* Expression, int32 OutputIndex = 0)
{
    FCustomInput& Input = Custom->Inputs.AddDefaulted_GetRef();
    Input.InputName = Name;
    Input.Input.Connect(OutputIndex, Expression);
}

UTexture2D* OptionalTexture(const TCHAR* Path, UTexture2D* Fallback)
{
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
    return Texture ? Texture : Fallback;
}
}

bool UYogPlanarFogEditorLibrary::BuildPlanarFogFinish()
{
    using namespace YogPlanarFogMaterial;
    const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("YogVision"));FString Code;
    if(!Plugin||!FFileHelper::LoadFileToString(Code,*(Plugin->GetBaseDir()/TEXT("Resources/PlanarFogFinish.hlsl"))))return false;
    UTexture2D* White=LoadObject<UTexture2D>(nullptr,TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
    if(!White)return false;
    const FString PackageName=TEXT("/YogVision/Planar/M_PlanarFogFinish");
    UMaterial* M=LoadObject<UMaterial>(nullptr,*(PackageName+TEXT(".M_PlanarFogFinish")),nullptr,LOAD_NoWarn|LOAD_Quiet);
    const bool New=!M;if(New)M=NewObject<UMaterial>(CreatePackage(*PackageName),TEXT("M_PlanarFogFinish"),RF_Public|RF_Standalone|RF_Transactional);
    M->PreEditChange(nullptr);M->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    M->MaterialDomain=MD_PostProcess;M->BlendableLocation=BL_SceneColorAfterDOF;M->BlendablePriority=60;M->bDisablePreExposureScale=false;
    auto* C=Node<UMaterialExpressionCustom>(M);C->Code=Code;C->OutputType=CMOT_Float3;
    const struct{const TCHAR* Name;ESceneTextureId Id;} Scenes[]={{TEXT("SceneColor"),PPI_PostProcessInput0},{TEXT("SceneNormal"),PPI_WorldNormal},{TEXT("SceneBaseColor"),PPI_BaseColor}};
    for(const auto& S:Scenes){auto* N=Node<UMaterialExpressionSceneTexture>(M);N->SceneTextureId=S.Id;Input(C,S.Name,N);}
    C->Description=TEXT("Architectural direction only; brush mapping uses native scattering before artist PP.");
    for(const TCHAR* Name:{TEXT("AirFogTransmittance")})
    {auto* N=Node<UMaterialExpressionTextureObjectParameter>(M);N->ParameterName=Name;N->Texture=White;N->SamplerType=SAMPLERTYPE_LinearColor;Input(C,Name,N);}
    for(const TCHAR* Name:{TEXT("FallbackLightDirection")})
    {auto* N=Node<UMaterialExpressionVectorParameter>(M);N->ParameterName=Name;N->DefaultValue=FLinearColor(1,1,1,0);Input(C,Name,N,5);}
    const struct{const TCHAR* Name;float Value;} Scalars[]={{TEXT("FormStrength"),.5f}};
    for(const auto& S:Scalars){auto* N=Node<UMaterialExpressionScalarParameter>(M);N->ParameterName=S.Name;N->DefaultValue=S.Value;Input(C,S.Name,N);}
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,C);M->PostEditChange();M->MarkPackageDirty();if(New)FAssetRegistryModule::AssetCreated(M);
    FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;A.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(M->GetOutermost(),M,*FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension()),A);
}
bool UYogPlanarFogEditorLibrary::BuildPlanarFogMaterial()
{
    using namespace YogPlanarFogMaterial;
    if (!IsInGameThread())
    {
        UE_LOG(LogYogPlanarFogBuilder, Error, TEXT("Planar fog material generation requires the game thread."));
        return false;
    }

    // Resolve all required sources before touching the generated material.
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    FString Code;
    if (!Plugin || !FFileHelper::LoadFileToString(Code, *FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/PlanarVolumeFog.hlsl")))
        || Code.TrimStartAndEnd().IsEmpty())
    {
        UE_LOG(LogYogPlanarFogBuilder, Error, TEXT("Cannot read YogVision Resources/PlanarVolumeFog.hlsl."));
        return false;
    }

    UTexture2D* White = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture"));
    UVolumeTexture* Noise = LoadObject<UVolumeTexture>(nullptr, TEXT("/YogVision/Stage12/T_Stage12DensityShape.T_Stage12DensityShape"));
    if (!White || !Noise)
    {
        UE_LOG(LogYogPlanarFogBuilder, Error, TEXT("Planar fog requires Engine WhiteSquareTexture and /YogVision/Stage12/T_Stage12DensityShape."));
        return false;
    }
    UTexture2D* DensityPattern = OptionalTexture(TEXT("/YogVision/Planar/T_PlanarFogDensity.T_PlanarFogDensity"), White);

    const FString PackageName = TEXT("/YogVision/Planar/M_PlanarVolumeFog");
    const FString ObjectPath = PackageName + TEXT(".M_PlanarVolumeFog");
    UObject* Existing = LoadObject<UObject>(nullptr, *ObjectPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    UMaterial* Material = Cast<UMaterial>(Existing);
    if (Existing && !Material)
    {
        UE_LOG(LogYogPlanarFogBuilder, Error, TEXT("%s already exists with a different asset type."), *ObjectPath);
        return false;
    }
    const bool bNewMaterial = !Material;
    if (!Material)
        Material = NewObject<UMaterial>(CreatePackage(*PackageName), TEXT("M_PlanarVolumeFog"), RF_Public | RF_Standalone | RF_Transactional);

    Material->PreEditChange(nullptr);
    Material->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    Material->MaterialDomain = MD_Volume;
    Material->BlendMode = BLEND_Additive;
    Material->SetShadingModel(MSM_DefaultLit);

    UMaterialExpressionCustom* Custom = Node<UMaterialExpressionCustom>(Material, 0, 0);
    Custom->Inputs.Empty();
    Custom->Code = MoveTemp(Code);
    Custom->OutputType = CMOT_Float4;
    Custom->Description = TEXT("YogPlanarFog.Generated.v3: native medium only; brush independent of extinction/albedo/emission");
    FCustomOutput& AlbedoOutput = Custom->AdditionalOutputs.AddDefaulted_GetRef();
    AlbedoOutput.OutputName = TEXT("FogAlbedoOut");
    AlbedoOutput.OutputType = CMOT_Float3;
    Custom->RebuildOutputs();

    UMaterialExpressionWorldPosition* Position = Node<UMaterialExpressionWorldPosition>(Material, -1000, -400);
    Position->WorldPositionShaderOffset = WPT_Default;
    Input(Custom, TEXT("WorldPosition"), Position);
    Input(Custom, TEXT("CameraPosition"), Node<UMaterialExpressionCameraPositionWS>(Material, -1000, -200));

    int32 ParameterRow = 0;
    auto Texture = [&](const TCHAR* Name, UTexture* Default)
    {
        auto* Parameter = Node<UMaterialExpressionTextureObjectParameter>(Material, -1000, ParameterRow++ * 160);
        Parameter->ParameterName = Name;
        Parameter->Texture = Default;
        Parameter->SamplerType = SAMPLERTYPE_LinearColor;
        Parameter->Group = TEXT("Planar Fog | Textures");
        Input(Custom, Name, Parameter);
    };
    Texture(TEXT("PlanarField"), White);
    Texture(TEXT("DensityPattern"), DensityPattern);
    Texture(TEXT("VolumeNoise"), Noise);

    auto Vector = [&](const TCHAR* Name, FLinearColor Default, const TCHAR* Group)
    {
        auto* Parameter = Node<UMaterialExpressionVectorParameter>(Material, -650, ParameterRow++ * 100);
        Parameter->ParameterName = Name;
        Parameter->DefaultValue = Default;
        Parameter->Group = Group;
        // Vector parameter pin 5 preserves RGBA; pin 0 would silently drop A.
        Input(Custom, Name, Parameter, 5);
    };
    Vector(TEXT("VolumeCenter"), FLinearColor(0, 0, 0, 0), TEXT("Planar Fog | Geometry"));
    Vector(TEXT("VolumeExtent"), FLinearColor(2400, 2400, 1600, 0), TEXT("Planar Fog | Geometry"));
    Vector(TEXT("VolumeAxisX"), FLinearColor(1, 0, 0, 0), TEXT("Planar Fog | Geometry"));
    Vector(TEXT("VolumeAxisY"), FLinearColor(0, 1, 0, 0), TEXT("Planar Fog | Geometry"));
    Vector(TEXT("VolumeAxisZ"), FLinearColor(0, 0, 1, 0), TEXT("Planar Fog | Geometry"));
    Vector(TEXT("SimulationCenter"), FLinearColor(0, 0, 0, 0), TEXT("Planar Fog | Simulation"));
    Vector(TEXT("SimulationExtent"), FLinearColor(1600, 1600, 1, 0), TEXT("Planar Fog | Simulation"));
    Vector(TEXT("ObserverPosition"), FLinearColor(0, 0, 0, 0), TEXT("Planar Fog | Clearing"));
    Vector(TEXT("FallbackLightDirection"), FLinearColor(.6f, -.6f, .6f, 0), TEXT("Planar Fog | Lighting"));
    Vector(TEXT("NearColour"), FLinearColor(.1125f, .6787f, .7448f, 1), TEXT("Planar Fog | Colour"));
    Vector(TEXT("FarColour"), FLinearColor(.1106f, .1505f, .35f, 1), TEXT("Planar Fog | Colour"));
    Vector(TEXT("FogAlbedo"), FLinearColor(.7f, .75f, .8f, 1), TEXT("Planar Fog | Lighting"));

    auto Scalar = [&](const TCHAR* Name, float Default, const TCHAR* Group)
    {
        auto* Parameter = Node<UMaterialExpressionScalarParameter>(Material, -350, ParameterRow++ * 80);
        Parameter->ParameterName = Name;
        Parameter->DefaultValue = Default;
        Parameter->Group = Group;
        Input(Custom, Name, Parameter);
    };
    Scalar(TEXT("Density"), .0014f, TEXT("Planar Fog | Medium"));
    Scalar(TEXT("SimulationReady"), 0, TEXT("Planar Fog | Simulation"));
    Scalar(TEXT("ClearRadius"), 400, TEXT("Planar Fog | Clearing"));
    Scalar(TEXT("ClearFeather"), 600, TEXT("Planar Fog | Clearing"));
    Scalar(TEXT("ClearResidual"), .18f, TEXT("Planar Fog | Clearing"));
    Scalar(TEXT("PaletteMix"), .07f, TEXT("Planar Fog | Colour"));
    Scalar(TEXT("PaletteDistance"), 3000, TEXT("Planar Fog | Colour"));
    Scalar(TEXT("AmbientStrength"), .18f, TEXT("Planar Fog | Lighting"));
    Scalar(TEXT("HeightCenter"), 0, TEXT("Planar Fog | Geometry"));
    Scalar(TEXT("HeightHalfSize"), 800, TEXT("Planar Fog | Geometry"));
    Scalar(TEXT("HeightSoftness"), 250, TEXT("Planar Fog | Geometry"));
    Scalar(TEXT("DensityFloor"), .18f, TEXT("Planar Fog | Medium"));
    Scalar(TEXT("DensityPower"), 1, TEXT("Planar Fog | Medium"));
    Scalar(TEXT("PatternWorldSize"), 1600, TEXT("Planar Fog | Medium"));
    Scalar(TEXT("NoiseWorldSize"), 240, TEXT("Planar Fog | Medium"));
    Scalar(TEXT("NoiseStrength"), .35f, TEXT("Planar Fog | Medium"));
    Scalar(TEXT("FlowTime"), 0, TEXT("Planar Fog | Simulation"));
    Scalar(TEXT("FlowDisplacement"), 16, TEXT("Planar Fog | Simulation"));
    Scalar(TEXT("FogTimeSeconds"), 0, TEXT("Planar Fog | Simulation"));

    auto* Emission = Node<UMaterialExpressionComponentMask>(Material, 350, -100);
    Emission->Input.Connect(0, Custom);
    Emission->R = Emission->G = Emission->B = true;
    Emission->A = false;
    auto* Extinction = Node<UMaterialExpressionComponentMask>(Material, 350, 100);
    Extinction->Input.Connect(0, Custom);
    Extinction->R = Extinction->G = Extinction->B = false;
    Extinction->A = true;
    Material->GetEditorOnlyData()->EmissiveColor.Connect(0, Emission);
    Material->GetEditorOnlyData()->SubsurfaceColor.Connect(0, Extinction);
    Material->GetEditorOnlyData()->BaseColor.Connect(1, Custom);

    Material->PostEditChange();
    Material->MarkPackageDirty();
    if (bNewMaterial) FAssetRegistryModule::AssetCreated(Material);
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    const bool bSaved = UPackage::SavePackage(Material->GetOutermost(), Material, *Filename, Args);
    UE_LOG(LogYogPlanarFogBuilder, Log, TEXT("Planar fog material %s: %s"), bSaved ? TEXT("saved") : TEXT("save failed"), *ObjectPath);
    return bSaved;
}
