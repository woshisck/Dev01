#include "YogStage12FogEditorLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionSceneDepth.h"
#include "Materials/MaterialExpressionDistanceToNearestSurface.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Engine/VolumeTexture.h"
#include "TextureCompiler.h"
#include "Interfaces/IPluginManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace Stage12
{
template<class T> T* Node(UMaterial* M)
{
    T* N=NewObject<T>(M);M->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(N);
    N->MaterialExpressionEditorX=-600;N->MaterialExpressionEditorY=M->GetEditorOnlyData()->ExpressionCollection.Expressions.Num()*45;return N;
}
void Input(UMaterialExpressionCustom* C,FName Name,UMaterialExpression* N,int32 Pin=0)
{auto& I=C->Inputs.AddDefaulted_GetRef();I.InputName=Name;I.Input.Connect(Pin,N);}
// Periodic integer lattice noise keeps all three volume texture boundaries seamless.
float TileNoise(const FVector3f& P,int32 Cells,uint32 Seed)
{
    const FIntVector I(FMath::FloorToInt(P.X),FMath::FloorToInt(P.Y),FMath::FloorToInt(P.Z));
    FVector3f F(P.X-I.X,P.Y-I.Y,P.Z-I.Z);F=F*F*(FVector3f(3)-2*F);
    auto Hash=[&](int32 X,int32 Y,int32 Z)
    {
        const auto Wrap=[Cells](int32 V){return (V%Cells+Cells)%Cells;};
        uint32 H=uint32(Wrap(X))*73856093u^uint32(Wrap(Y))*19349663u^uint32(Wrap(Z))*83492791u^Seed;
        H^=H>>16;H*=0x7feb352du;H^=H>>15;H*=0x846ca68bu;H^=H>>16;
        return float(H&0xffffffu)/16777215.f;
    };
    return FMath::Lerp(FMath::Lerp(FMath::Lerp(Hash(I.X,I.Y,I.Z),Hash(I.X+1,I.Y,I.Z),F.X),FMath::Lerp(Hash(I.X,I.Y+1,I.Z),Hash(I.X+1,I.Y+1,I.Z),F.X),F.Y),
        FMath::Lerp(FMath::Lerp(Hash(I.X,I.Y,I.Z+1),Hash(I.X+1,I.Y,I.Z+1),F.X),FMath::Lerp(Hash(I.X,I.Y+1,I.Z+1),Hash(I.X+1,I.Y+1,I.Z+1),F.X),F.Y),F.Z);
}
UVolumeTexture* ShapeTexture()
{
    const FString Package=TEXT("/YogVision/Stage12/T_Stage12DensityShape");
    if(auto* Existing=LoadObject<UVolumeTexture>(nullptr,*(Package+TEXT(".T_Stage12DensityShape")),nullptr,LOAD_NoWarn))return Existing;
    auto* T=NewObject<UVolumeTexture>(CreatePackage(*Package),TEXT("T_Stage12DensityShape"),RF_Public|RF_Standalone);
    T->SRGB=false;T->CompressionSettings=TC_HDR;T->MipGenSettings=TMGS_SimpleAverage;T->AddressMode=TA_Wrap;
    T->UpdateSourceFromFunction([](int32 X,int32 Y,int32 Z,void* Out)
    {
        const FVector3f P((X+.5f)/64,(Y+.5f)/64,(Z+.5f)/64);
        const float Macro=.55f*TileNoise(P*3,3,73)+.27f*TileNoise(P*6,6,91)+.13f*TileNoise(P*12,12,127)+.05f*TileNoise(P*24,24,239);
        const float Detail=.72f*TileNoise(P*8,8,653)+.28f*TileNoise(P*16,16,991);
        *static_cast<FFloat16Color*>(Out)=FFloat16Color(FLinearColor(Macro,Detail,TileNoise(P*4,4,313),TileNoise(P*4,4,461)));
    },64,64,64,TSF_RGBA16F);
    T->PostEditChange();UTexture* Base=T;FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Base,1));
    FAssetRegistryModule::AssetCreated(T);FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    if(!UPackage::SavePackage(T->GetOutermost(),T,*FPackageName::LongPackageNameToFilename(Package,FPackageName::GetAssetPackageExtension()),Args))return nullptr;
    return T;
}
}
bool UYogStage12FogEditorLibrary::BuildStage12Material()
{
    const FString PackageName=TEXT("/YogVision/Stage12/M_Stage12SpatialFog");
    UMaterial* M=LoadObject<UMaterial>(nullptr,*(PackageName+TEXT(".M_Stage12SpatialFog")),nullptr,LOAD_NoWarn);
    if(!M){M=NewObject<UMaterial>(CreatePackage(*PackageName),TEXT("M_Stage12SpatialFog"),RF_Public|RF_Standalone|RF_Transactional);FAssetRegistryModule::AssetCreated(M);}
    M->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    M->MaterialDomain=MD_PostProcess;M->BlendableLocation=BL_SceneColorAfterDOF;M->BlendablePriority=100;M->bDisablePreExposureScale=false;
    auto* C=Stage12::Node<UMaterialExpressionCustom>(M);C->Inputs.Empty();C->OutputType=CMOT_Float3;C->Description=TEXT("Yog.Stage12.SpatialRaymarch.Shape.v2");
    auto P=IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    if(!P||!FFileHelper::LoadFileToString(C->Code,*(P->GetBaseDir()/TEXT("Resources/Stage12SpatialFog.hlsl"))))return false;
    Stage12::Input(C,TEXT("WorldPosition"),Stage12::Node<UMaterialExpressionWorldPosition>(M));
    Stage12::Input(C,TEXT("CameraPosition"),Stage12::Node<UMaterialExpressionCameraPositionWS>(M));
    Stage12::Input(C,TEXT("SceneDepth"),Stage12::Node<UMaterialExpressionSceneDepth>(M));
    auto* Scene=Stage12::Node<UMaterialExpressionSceneTexture>(M);Scene->SceneTextureId=PPI_PostProcessInput0;Stage12::Input(C,TEXT("SceneColour"),Scene);
    // This real expression sets the material's GDF resource usage flag; a string-only Custom call does not.
    Stage12::Input(C,TEXT("DistanceFieldUsage"),Stage12::Node<UMaterialExpressionDistanceToNearestSurface>(M));
    auto Scalar=[&](const TCHAR* Name,float Default){auto* N=Stage12::Node<UMaterialExpressionScalarParameter>(M);N->ParameterName=Name;N->DefaultValue=Default;N->Group=TEXT("Stage12");Stage12::Input(C,Name,N);};
    auto Vector=[&](const TCHAR* Name,FLinearColor Default){auto* N=Stage12::Node<UMaterialExpressionVectorParameter>(M);N->ParameterName=Name;N->DefaultValue=Default;N->Group=TEXT("Stage12");Stage12::Input(C,Name,N,5);};
    UVolumeTexture* Shape=Stage12::ShapeTexture();if(!Shape)return false;
    auto* ShapeNode=Stage12::Node<UMaterialExpressionTextureObjectParameter>(M);ShapeNode->ParameterName=TEXT("DensityShapeTexture");ShapeNode->Texture=Shape;Stage12::Input(C,TEXT("DensityShapeTexture"),ShapeNode);
    Scalar(TEXT("UseDensityShape"),1);Scalar(TEXT("ShapeStrength"),1);Scalar(TEXT("ShapeScale"),.0005);
    Scalar(TEXT("DetailStrength"),.35);Scalar(TEXT("DetailScale"),.0025);Scalar(TEXT("ShapeEdgeSoftness"),.2);
    Scalar(TEXT("BodyCoreDensity"),.15);Scalar(TEXT("BodyRoundness"),1);Vector(TEXT("ShapeStretch"),FLinearColor(1,1,.7,0));
    Scalar(TEXT("FlowWarpStrength"),35);Scalar(TEXT("FlowPeriod"),24);Scalar(TEXT("FogTimeSeconds"),0);Vector(TEXT("FlowVelocity"),FLinearColor(5,-2,1,0));
    const FString FieldPackage=TEXT("/YogVision/Stage12/T_Stage12FieldProbe");
    auto* Field=LoadObject<UVolumeTexture>(nullptr,*(FieldPackage+TEXT(".T_Stage12FieldProbe")),nullptr,LOAD_NoWarn);
    if(!Field)
    {
        Field=NewObject<UVolumeTexture>(CreatePackage(*FieldPackage),TEXT("T_Stage12FieldProbe"),RF_Public|RF_Standalone);
        Field->SRGB=false;Field->CompressionSettings=TC_HDR;Field->MipGenSettings=TMGS_NoMipmaps;
        Field->UpdateSourceFromFunction([](int32 X,int32 Y,int32 Z,void* Out)
        {
            const FVector P((X+.5)/16.,(Y+.5)/16.,(Z+.5)/16.);
            const float Blob=FMath::Exp(-FVector::DistSquared(P,FVector(.45,.5,.5))*60.f);
            // Signed floating-point contract: velocity cm/s in RGB; density delta /m in A.
            *static_cast<FFloat16Color*>(Out)=FFloat16Color(FLinearColor(0,Blob*80,0,-Blob*.1f));
        },16,16,16,TSF_RGBA16F);
        Field->PostEditChange();UTexture* T=Field;FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&T,1));
        FAssetRegistryModule::AssetCreated(Field);FSavePackageArgs FieldArgs;FieldArgs.TopLevelFlags=RF_Public|RF_Standalone;FieldArgs.SaveFlags=SAVE_NoError;
        if(!UPackage::SavePackage(Field->GetOutermost(),Field,*FPackageName::LongPackageNameToFilename(FieldPackage,FPackageName::GetAssetPackageExtension()),FieldArgs))return false;
    }
    auto* FieldNode=Stage12::Node<UMaterialExpressionTextureObjectParameter>(M);FieldNode->ParameterName=TEXT("FieldTexture");FieldNode->Texture=Field;Stage12::Input(C,TEXT("FieldTexture"),FieldNode);
    Scalar(TEXT("ExternalFieldStrength"),0);
    Vector(TEXT("VolumeCenter"),FLinearColor(1500,0,1000,1));Vector(TEXT("VolumeExtent"),FLinearColor(1700,2000,1200,1));
    Vector(TEXT("VolumeAxisX"),FLinearColor(1,0,0,0));Vector(TEXT("VolumeAxisY"),FLinearColor(0,1,0,0));Vector(TEXT("VolumeAxisZ"),FLinearColor(0,0,1,0));
    Vector(TEXT("FogAlbedo"),FLinearColor(.85,.9,.95));Vector(TEXT("AmbientRadiance"),FLinearColor(.035,.045,.06));
    Vector(TEXT("NearColour"),FLinearColor(.1125,.6787,.7448));Vector(TEXT("FarColour"),FLinearColor(.1106,.1505,.35));
    Vector(TEXT("DirectionalDirection"),FLinearColor(.6,-.6,.6,0));Vector(TEXT("DirectionalColour"),FLinearColor(.08,.1,.14));
    Vector(TEXT("GameplayObserver"),FLinearColor::Transparent);Vector(TEXT("ExplicitPreviewFocus"),FLinearColor::Transparent);
    for(int i=0;i<4;i++) {Vector(*FString::Printf(TEXT("LightPosition%d"),i),FLinearColor::Transparent);Vector(*FString::Printf(TEXT("LightDirection%d"),i),FLinearColor(0,0,0,-2));Vector(*FString::Printf(TEXT("LightColour%d"),i),FLinearColor::Transparent);Scalar(*FString::Printf(TEXT("LightInnerCos%d"),i),.9);}
    Scalar(TEXT("Density"),.12);Scalar(TEXT("DensityVariation"),.3);Scalar(TEXT("NoiseScale"),.002);Scalar(TEXT("EdgeWidth"),350);
    Scalar(TEXT("AmbientStrength"),1);Scalar(TEXT("StepCount"),64);Scalar(TEXT("ShadowSteps"),24);Scalar(TEXT("MaxDistance"),15000);
    Scalar(TEXT("UseGeometryShadows"),1);Scalar(TEXT("GeometryShadowBias"),8);Scalar(TEXT("PaletteStrength"),.5);Scalar(TEXT("PaletteDistance"),3000);Scalar(TEXT("PaletteMix"),.07);
    Scalar(TEXT("DirectionalStrength"),1);Scalar(TEXT("LocalLightScale"),.015);Scalar(TEXT("LightClearStrength"),0);
    Scalar(TEXT("VisionEnabled"),1);Scalar(TEXT("ObserverClearRadius"),500);Scalar(TEXT("ObserverFeather"),650);Scalar(TEXT("ClearDensityFraction"),.1);
    Scalar(TEXT("PreviewFocusHeight"),150);Scalar(TEXT("PreviewFocusMaxDistance"),7000);Scalar(TEXT("DebugMode"),0);
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,C);M->PostEditChange();M->MarkPackageDirty();
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
    return UPackage::SavePackage(M->GetOutermost(),M,*FPackageName::LongPackageNameToFilename(PackageName,FPackageName::GetAssetPackageExtension()),Args);
}
