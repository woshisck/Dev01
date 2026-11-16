#include "YogStage12FogEditorLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Engine/Texture2D.h"
#include "Engine/VolumeTexture.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/SavePackage.h"

namespace NativeAtlasFog
{
template<class T> T* Node(UMaterial* M)
{
    T* N=NewObject<T>(M);M->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(N);return N;
}
void Input(UMaterialExpressionCustom* C,FName Name,UMaterialExpression* N,int32 Pin=0)
{auto& I=C->Inputs.AddDefaulted_GetRef();I.InputName=Name;I.Input.Connect(Pin,N);}
}
bool UYogStage12FogEditorLibrary::BuildNativeAtlasFogMaterial()
{
    const FString Package=TEXT("/YogVision/Stage12/Native/M_Stage12NativeAtlasFog");
    UMaterial* M=LoadObject<UMaterial>(nullptr,*(Package+TEXT(".M_Stage12NativeAtlasFog")),nullptr,LOAD_NoWarn);
    if(!M){M=NewObject<UMaterial>(CreatePackage(*Package),TEXT("M_Stage12NativeAtlasFog"),RF_Public|RF_Standalone|RF_Transactional);FAssetRegistryModule::AssetCreated(M);}
    M->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    M->MaterialDomain=MD_Volume;M->BlendMode=BLEND_Additive;M->SetShadingModel(MSM_DefaultLit);
    auto* C=NativeAtlasFog::Node<UMaterialExpressionCustom>(M);C->Inputs.Empty();C->OutputType=CMOT_Float4;C->Description=TEXT("Native fog voxel density + atlas velocity interpolation");
    const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    if(!Plugin||!FFileHelper::LoadFileToString(C->Code,*(Plugin->GetBaseDir()/TEXT("Resources/NativeAtlasFog.hlsl"))))return false;
    NativeAtlasFog::Input(C,TEXT("WorldPosition"),NativeAtlasFog::Node<UMaterialExpressionWorldPosition>(M));
    NativeAtlasFog::Input(C,TEXT("CameraPosition"),NativeAtlasFog::Node<UMaterialExpressionCameraPositionWS>(M));
    auto Scalar=[&](const TCHAR* Name,float Value){auto* N=NativeAtlasFog::Node<UMaterialExpressionScalarParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;N->Group=TEXT("Native Fog");NativeAtlasFog::Input(C,Name,N);};
    auto Vector=[&](const TCHAR* Name,FLinearColor Value){auto* N=NativeAtlasFog::Node<UMaterialExpressionVectorParameter>(M);N->ParameterName=Name;N->DefaultValue=Value;N->Group=TEXT("Native Fog");NativeAtlasFog::Input(C,Name,N,5);};
    auto Texture=[&](const TCHAR* Name,const TCHAR* Path)->bool{auto* T=LoadObject<UTexture>(nullptr,Path);if(!T)return false;auto* N=NativeAtlasFog::Node<UMaterialExpressionTextureObjectParameter>(M);N->ParameterName=Name;N->Texture=T;NativeAtlasFog::Input(C,Name,N);return true;};
    if(!Texture(TEXT("DensityAtlas"),TEXT("/YogVision/Stage12/Native/RadiSw7_density.RadiSw7_density"))
        || !Texture(TEXT("VelocityAtlas"),TEXT("/YogVision/Stage12/Native/RadiSw7_velocity.RadiSw7_velocity"))
        || !Texture(TEXT("DetailTexture"),TEXT("/YogVision/Stage12/T_Stage12DensityShape.T_Stage12DensityShape")))return false;
    Vector(TEXT("VolumeCenter"),FLinearColor(2200,-600,600,0));Vector(TEXT("VolumeExtent"),FLinearColor(2200,2400,1400,0));
    Vector(TEXT("VolumeAxisX"),FLinearColor(1,0,0,0));Vector(TEXT("VolumeAxisY"),FLinearColor(0,1,0,0));Vector(TEXT("VolumeAxisZ"),FLinearColor(0,0,1,0));
    Vector(TEXT("ObserverPosition"),FLinearColor::Transparent);Vector(TEXT("FallbackLightDirection"),FLinearColor(.6f,-.6f,.6f,0));
    Vector(TEXT("NearColour"),FLinearColor(.1125f,.6787f,.7448f));Vector(TEXT("FarColour"),FLinearColor(.1106f,.1505f,.35f));
    Scalar(TEXT("Density"),.0012f);Scalar(TEXT("FogTimeSeconds"),0);Scalar(TEXT("AtlasFPS"),4);Scalar(TEXT("FlowStrength"),.05f);
    Scalar(TEXT("ShapeScale"),.0006f);Scalar(TEXT("DetailStrength"),.5f);Scalar(TEXT("ClearRadius"),500);Scalar(TEXT("ClearFeather"),650);Scalar(TEXT("ClearResidual"),.15f);
    Scalar(TEXT("PaletteMix"),.07f);Scalar(TEXT("PaletteDistance"),3000);Scalar(TEXT("FallbackLightStrength"),.02f);
    auto* Alpha=NativeAtlasFog::Node<UMaterialExpressionComponentMask>(M);Alpha->Input.Connect(0,C);Alpha->R=false;Alpha->G=false;Alpha->B=false;Alpha->A=true;
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,C);M->GetEditorOnlyData()->SubsurfaceColor.Connect(0,Alpha);
    auto* Albedo=NativeAtlasFog::Node<UMaterialExpressionVectorParameter>(M);Albedo->ParameterName=TEXT("FogAlbedo");Albedo->DefaultValue=FLinearColor(.7f,.75f,.8f);
    M->GetEditorOnlyData()->BaseColor.Connect(0,Albedo);M->PostEditChange();M->MarkPackageDirty();
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(M->GetOutermost(),M,*FPackageName::LongPackageNameToFilename(Package,FPackageName::GetAssetPackageExtension()),Args);
}
UMaterialInterface* UYogStage12FogEditorLibrary::LoadNativeFogReference(const FString& StagedFluidNinjaRoot)
{
    FString Directory=FPaths::ConvertRelativePathToFull(StagedFluidNinjaRoot);FPaths::NormalizeDirectoryName(Directory);Directory+=TEXT("/");
    if(!FPaths::FileExists(Directory/TEXT("Usecases/Volumetrics/Instances_VolumeFog/MI_VolumeFog_Vortex.uasset")))return nullptr;
    FPackageName::RegisterMountPoint(TEXT("/Game/FluidNinja/"),Directory);
    return LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/FluidNinja/Usecases/Volumetrics/Instances_VolumeFog/MI_VolumeFog_Vortex.MI_VolumeFog_Vortex"));
}
