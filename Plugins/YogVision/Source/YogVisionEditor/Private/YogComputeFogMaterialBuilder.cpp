#include "YogStage12FogEditorLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Engine/VolumeTexture.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace YogComputeMaterial
{
template<class T> T* Node(UMaterial* M)
{
    T* N=NewObject<T>(M);M->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(N);return N;
}
void Input(UMaterialExpressionCustom* C,FName Name,UMaterialExpression* N,int32 Pin=0)
{auto& I=C->Inputs.AddDefaulted_GetRef();I.InputName=Name;I.Input.Connect(Pin,N);}
}
bool UYogStage12FogEditorLibrary::BuildComputeFogMaterial()
{
    const FString Package=TEXT("/YogVision/Stage12/Compute/M_ComputeVolumeFog");
    UMaterial* M=LoadObject<UMaterial>(nullptr,*(Package+TEXT(".M_ComputeVolumeFog")),nullptr,LOAD_NoWarn);
    if(!M){M=NewObject<UMaterial>(CreatePackage(*Package),TEXT("M_ComputeVolumeFog"),RF_Public|RF_Standalone|RF_Transactional);FAssetRegistryModule::AssetCreated(M);}
    M->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    M->MaterialDomain=MD_Volume;M->BlendMode=BLEND_Additive;M->SetShadingModel(MSM_DefaultLit);
    auto* C=YogComputeMaterial::Node<UMaterialExpressionCustom>(M);C->Inputs.Empty();C->OutputType=CMOT_Float4;C->Description=TEXT("Persistent GPU 3D simulation + independent XYZ shape");
    const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    if(!Plugin||!FFileHelper::LoadFileToString(C->Code,*(Plugin->GetBaseDir()/TEXT("Resources/ComputeVolumeFog.hlsl"))))return false;
    YogComputeMaterial::Input(C,TEXT("WorldPosition"),YogComputeMaterial::Node<UMaterialExpressionWorldPosition>(M));
    YogComputeMaterial::Input(C,TEXT("CameraPosition"),YogComputeMaterial::Node<UMaterialExpressionCameraPositionWS>(M));
    auto Scalar=[&](const TCHAR* Name,float Value){auto* N=YogComputeMaterial::Node<UMaterialExpressionScalarParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;N->Group=TEXT("Compute Fog");YogComputeMaterial::Input(C,Name,N);};
    auto Vector=[&](const TCHAR* Name,FLinearColor Value){auto* N=YogComputeMaterial::Node<UMaterialExpressionVectorParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;N->Group=TEXT("Compute Fog");YogComputeMaterial::Input(C,Name,N,5);};
    UVolumeTexture* Shape=LoadObject<UVolumeTexture>(nullptr,TEXT("/YogVision/Stage12/T_Stage12DensityShape.T_Stage12DensityShape"));
    if(!Shape)return false;
    for(const TCHAR* Name:{TEXT("DensityField"),TEXT("BackgroundShape")})
    {
        auto* N=YogComputeMaterial::Node<UMaterialExpressionTextureObjectParameter>(M);N->ParameterName=Name;N->Texture=Shape;N->SamplerType=SAMPLERTYPE_LinearColor;YogComputeMaterial::Input(C,Name,N);
    }
    Vector(TEXT("VolumeCenter"),FLinearColor(1800,-600,500,0));Vector(TEXT("VolumeExtent"),FLinearColor(2200,2400,1400,0));
    Vector(TEXT("VolumeAxisX"),FLinearColor(1,0,0,0));Vector(TEXT("VolumeAxisY"),FLinearColor(0,1,0,0));Vector(TEXT("VolumeAxisZ"),FLinearColor(0,0,1,0));
    Vector(TEXT("SimulationCenter"),FLinearColor(1800,-600,500,0));Vector(TEXT("SimulationExtent"),FLinearColor(900,900,600,0));
    Vector(TEXT("ShapeScaleXYZ"),FLinearColor(1,1,1,0));Vector(TEXT("GridResolution"),FLinearColor(64,64,48,0));
    Vector(TEXT("ObserverPosition"),FLinearColor::Transparent);Vector(TEXT("FallbackLightDirection"),FLinearColor(.6f,-.6f,.6f,0));
    Vector(TEXT("NearColour"),FLinearColor(.1125f,.6787f,.7448f));Vector(TEXT("FarColour"),FLinearColor(.1106f,.1505f,.35f));
    Scalar(TEXT("Density"),.0025f);Scalar(TEXT("SimulationReady"),0);Scalar(TEXT("ClearRadius"),400);Scalar(TEXT("ClearFeather"),500);Scalar(TEXT("ClearResidual"),.2f);
    Scalar(TEXT("PaletteMix"),.07f);Scalar(TEXT("PaletteDistance"),3000);Scalar(TEXT("FallbackLightStrength"),.012f);
    auto* Alpha=YogComputeMaterial::Node<UMaterialExpressionComponentMask>(M);Alpha->Input.Connect(0,C);Alpha->R=false;Alpha->G=false;Alpha->B=false;Alpha->A=true;
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,C);M->GetEditorOnlyData()->SubsurfaceColor.Connect(0,Alpha);
    auto* Albedo=YogComputeMaterial::Node<UMaterialExpressionVectorParameter>(M);Albedo->ParameterName=TEXT("FogAlbedo");Albedo->DefaultValue=FLinearColor(.7f,.75f,.8f);
    M->GetEditorOnlyData()->BaseColor.Connect(0,Albedo);M->PostEditChange();M->MarkPackageDirty();
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(M->GetOutermost(),M,*FPackageName::LongPackageNameToFilename(Package,FPackageName::GetAssetPackageExtension()),Args);
}
