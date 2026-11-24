#include "YogAirFogEditorService.h"
#include "YogVisionSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTextureObjectParameter.h"
#include "Engine/Texture2D.h"
#include "Factories/TextureFactory.h"
#include "TextureCompiler.h"
#include "Materials/MaterialInstanceConstant.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UObjectIterator.h"
#include "Editor.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "LevelEditorViewport.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/App.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace AirFogEditor
{
constexpr const TCHAR* ArtistMaterial = TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline");
constexpr const TCHAR* CompatibleArtistMaterial = TEXT("/YogVision/AirFog/MI_AirFogArtistCompat.MI_AirFogArtistCompat");
TStrongObjectPtr<UMaterialInterface> WorkbenchPalette;
constexpr const TCHAR* ArtistGuardCode = TEXT("if (!IsFinite(SceneDepth) || SceneDepth <= 0 || SceneDepth >= 1000000 || !all(IsFinite(ArtColor))) return SceneColor.rgb; return ArtColor;");
template<class T> T* Node(UMaterial* M)
{
    T* N = NewObject<T>(M);
    M->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(N);
    N->MaterialExpressionEditorX = -700;
    N->MaterialExpressionEditorY = M->GetEditorOnlyData()->ExpressionCollection.Expressions.Num() * 60;
    return N;
}
void Input(UMaterialExpressionCustom* C, FName Name, UMaterialExpression* E, bool bScalar=false)
{
    auto& I = C->Inputs.AddDefaulted_GetRef(); I.InputName=Name;
    // VectorParameter pin 0 is RGB; pin 5 preserves radius / cone / flag alpha.
    I.Input.Connect(E->IsA<UMaterialExpressionVectorParameter>() ? 5 : 0,E);
    if (bScalar) { I.Input.Mask=1; I.Input.MaskR=1; I.Input.MaskG=I.Input.MaskB=I.Input.MaskA=0; }
}
UMaterialExpressionScalarParameter* Scalar(UMaterial* M, FName Name, float Value)
{
    auto* N=Node<UMaterialExpressionScalarParameter>(M); N->ParameterName=Name; N->DefaultValue=Value; N->Group=TEXT("AirFog"); return N;
}
UMaterialExpressionVectorParameter* Vector(UMaterial* M, FName Name, FLinearColor Value)
{
    auto* N=Node<UMaterialExpressionVectorParameter>(M); N->ParameterName=Name; N->DefaultValue=Value; N->Group=TEXT("AirFog"); return N;
}
bool Save(UObject* Asset)
{
    FString File=FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
    Asset->MarkPackageDirty(); return UPackage::SavePackage(Asset->GetOutermost(),Asset,*File,Args);
}
UTexture2D* BuildOilBrush()
{
    const TCHAR* PackageName=TEXT("/YogVision/Textures/T_VolumeBrushOil");
    const TCHAR* AssetPath=TEXT("/YogVision/Textures/T_VolumeBrushOil.T_VolumeBrushOil");
    if(FPackageName::DoesPackageExist(PackageName))return LoadObject<UTexture2D>(nullptr,AssetPath,nullptr,LOAD_NoWarn);
    auto Plugin=IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    const FString File=Plugin?Plugin->GetBaseDir()/TEXT("Resources/T_VolumeBrushOil.png"):FString();
    if(!FPaths::FileExists(File))return nullptr;
    UPackage* Package=CreatePackage(PackageName);
    auto* Factory=NewObject<UTextureFactory>();Factory->SuppressImportOverwriteDialog();
    Factory->ColorSpaceMode=ETextureSourceColorSpace::Linear;Factory->CompressionSettings=TC_Masks;
    bool Cancelled=false;
    auto* Texture=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),Package,
        TEXT("T_VolumeBrushOil"),RF_Public|RF_Standalone,File,nullptr,GWarn,Cancelled));
    if(!Texture)return nullptr;
    UTexture* Imported=Texture;FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Imported,1));
    Texture->SRGB=false;Texture->CompressionSettings=TC_Masks;
    Texture->AddressX=TA_Mirror;Texture->AddressY=TA_Mirror;
    Texture->PowerOfTwoMode=ETexturePowerOfTwoSetting::StretchToPowerOfTwo;
    Texture->MipGenSettings=TMGS_FromTextureGroup;Texture->LODGroup=TEXTUREGROUP_World;
    Texture->PostEditChange();FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Imported,1));
    FAssetRegistryModule::AssetCreated(Texture);return Save(Texture)?Texture:nullptr;
}
bool BuildScreenTexture(const TCHAR* Name, const TCHAR* Resource, bool bFlow)
{
    const FString PackageName = FString(TEXT("/YogVision/Textures/")) + Name;
    if (FPackageName::DoesPackageExist(PackageName))
        return LoadObject<UTexture2D>(nullptr, *(PackageName + TEXT(".") + Name)) != nullptr;
    const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("YogVision"));
    const FString File = Plugin ? Plugin->GetBaseDir() / TEXT("Resources") / Resource : FString();
    if (!FPaths::FileExists(File)) return false;
    auto* Factory = NewObject<UTextureFactory>();
    Factory->SuppressImportOverwriteDialog();
    Factory->ColorSpaceMode = ETextureSourceColorSpace::Linear;
    Factory->CompressionSettings = bFlow ? TC_VectorDisplacementmap : TC_Masks;
    bool Cancelled = false;
    auto* Texture = Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(), CreatePackage(*PackageName),
        FName(Name), RF_Public | RF_Standalone, File, nullptr, GWarn, Cancelled));
    if (!Texture) return false;
    UTexture* Imported = Texture;
    FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Imported, 1));
    Texture->SRGB = false;
    Texture->CompressionSettings = bFlow ? TC_VectorDisplacementmap : TC_Masks;
    Texture->AddressX = TA_Mirror; Texture->AddressY = TA_Mirror;
    Texture->PowerOfTwoMode = ETexturePowerOfTwoSetting::StretchToPowerOfTwo;
    Texture->MaxTextureSize = bFlow ? 1024 : 2048;
    Texture->MipGenSettings = TMGS_FromTextureGroup;
    Texture->PostEditChange();
    FTextureCompilingManager::Get().FinishCompilation(MakeArrayView(&Imported, 1));
    FAssetRegistryModule::AssetCreated(Texture);
    return Save(Texture);
}
bool BuildMaterial(bool bDensity, const FString& Code)
{
    const FString Name=bDensity?TEXT("M_AirFogDensity"):TEXT("M_AirFogFinish");
    const FString Path=TEXT("/YogVision/AirFog/")+Name;
    UMaterial* M=FPackageName::DoesPackageExist(Path)?LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name),nullptr,LOAD_NoWarn):nullptr;
    if(M)
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("YogAirFogRebuildGenerated"))) return true;
        bool bOwned=false;
        for(UMaterialExpression* E:M->GetEditorOnlyData()->ExpressionCollection.Expressions)
            if(auto* C=Cast<UMaterialExpressionCustom>(E)) bOwned|=C->Description==TEXT("YogAirFog.Generated.v1");
        if(!bOwned) return false;
        M->GetEditorOnlyData()->ExpressionCollection.Expressions.Empty();
    }
    else
    {
        M=NewObject<UMaterial>(CreatePackage(*Path),*Name,RF_Public|RF_Standalone|RF_Transactional);
        FAssetRegistryModule::AssetCreated(M);
    }
    M->MaterialDomain=bDensity?MD_Volume:MD_PostProcess;
    M->SetShadingModel(MSM_DefaultLit);
    if(bDensity) M->BlendMode=BLEND_Additive;
    else { M->BlendableLocation=BL_SceneColorAfterDOF; M->BlendablePriority=60; M->bDisablePreExposureScale=false; }
    auto* C=Node<UMaterialExpressionCustom>(M); C->Inputs.Empty(); C->Code=Code;
    C->Description=TEXT("YogAirFog.Generated.v1"); C->OutputType=bDensity?CMOT_Float4:CMOT_Float3; C->MaterialExpressionEditorX=200;
    Input(C,TEXT("WorldPosition"),Node<UMaterialExpressionWorldPosition>(M));
    Input(C,TEXT("CameraPosition"),Node<UMaterialExpressionCameraPositionWS>(M));
    Input(C,TEXT("Focus"),Vector(M,TEXT("Focus"),FLinearColor::Black));
    Input(C,TEXT("VisionSource"),Vector(M,TEXT("VisionSource"),FLinearColor::Transparent));
    Input(C,TEXT("UseScreenVision"),Scalar(M,TEXT("UseScreenVision"),1));
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FName P(*FString::Printf(TEXT("LightPosition%d"), Index));
        const FName D(*FString::Printf(TEXT("LightDirection%d"), Index));
        const FName L(*FString::Printf(TEXT("LightColour%d"), Index));
        Input(C,P,Vector(M,P,FLinearColor::Black));
        Input(C,D,Vector(M,D,FLinearColor(0,0,0,-2)));
        Input(C,L,Vector(M,L,FLinearColor::Transparent));
    }
    Input(C,TEXT("TimeSeconds"),Scalar(M,TEXT("TimeSeconds"),0));
    if(!bDensity)Input(C,TEXT("AirFogDebug"),Scalar(M,TEXT("AirFogDebug"),0));
    Input(C,TEXT("UseArtistPalette"),Scalar(M,TEXT("UseArtistPalette"),1));
    Input(C,TEXT("ArtistNearColor"),Vector(M,TEXT("ArtistNearColor"),FLinearColor(.112495f,.678705f,.744792f,1)));
    Input(C,TEXT("ArtistFarColor"),Vector(M,TEXT("ArtistFarColor"),FLinearColor(.1106f,.1505f,.35f,1)));
    Input(C,TEXT("ArtistFogMix"),Scalar(M,TEXT("ArtistFogMix"),.07f));
    const struct { const TCHAR* Name; const TCHAR* Path; } TextureInputs[] = {
        {TEXT("FlowTexture"),TEXT("/YogVision/Textures/T_ScreenBrushFlow.T_ScreenBrushFlow")},
        {TEXT("StrokeTexture"),TEXT("/YogVision/Textures/T_ScreenVisionStrokes.T_ScreenVisionStrokes")},
        {TEXT("AirFogTransmittance"),TEXT("/Engine/EngineResources/WhiteSquareTexture.WhiteSquareTexture")}};
    for (const auto& Entry : TextureInputs)
    {
        if (bDensity && FString(Entry.Name) == TEXT("AirFogTransmittance")) continue;
        auto* Texture = Node<UMaterialExpressionTextureObjectParameter>(M);
        Texture->ParameterName = Entry.Name;
        Texture->Texture = LoadObject<UTexture2D>(nullptr, Entry.Path);
        if (!Texture->Texture) return false;
        Input(C, Texture->ParameterName, Texture);
    }
    FYogAirFogParameters Defaults;
    for(TFieldIterator<FProperty> It(FYogAirFogParameters::StaticStruct());It;++It)
    {
        if(auto* F=CastField<FFloatProperty>(*It)) Input(C,It->GetFName(),Scalar(M,It->GetFName(),F->GetPropertyValue_InContainer(&Defaults)));
        else if(auto* I=CastField<FIntProperty>(*It)) Input(C,It->GetFName(),Scalar(M,It->GetFName(),I->GetPropertyValue_InContainer(&Defaults)));
        else if(auto* S=CastField<FStructProperty>(*It);S&&S->Struct==TBaseStructure<FLinearColor>::Get())
            Input(C,It->GetFName(),Vector(M,It->GetFName(),*S->ContainerPtrToValuePtr<FLinearColor>(&Defaults)));
        else if(auto* VectorProperty=CastField<FStructProperty>(*It);VectorProperty&&VectorProperty->Struct==TBaseStructure<FVector>::Get())
        {
            const FVector V = *VectorProperty->ContainerPtrToValuePtr<FVector>(&Defaults);
            Input(C,It->GetFName(),Vector(M,It->GetFName(),FLinearColor(V.X,V.Y,V.Z,1.f)));
        }
    }
    if(bDensity)
    {
        auto* Albedo=Vector(M,TEXT("FogAlbedo"),Defaults.FogAlbedo);
        auto* Extinction=Node<UMaterialExpressionComponentMask>(M); Extinction->Input.Connect(0,C); Extinction->A=true;
        Extinction->R=Extinction->G=Extinction->B=false;
        auto* Emission=Node<UMaterialExpressionComponentMask>(M); Emission->Input.Connect(0,C);
        Emission->R=Emission->G=Emission->B=true; Emission->A=false;
        M->GetEditorOnlyData()->BaseColor.Connect(0,Albedo);
        M->GetEditorOnlyData()->SubsurfaceColor.Connect(0,Extinction); // Per-metre extinction; voxelizer converts to cm.
        M->GetEditorOnlyData()->EmissiveColor.Connect(0,Emission);
    }
    else
    {
        const struct { const TCHAR* Name; ESceneTextureId Id; bool bScalar; } Textures[]={
            {TEXT("SceneColor"),PPI_PostProcessInput0,false},{TEXT("SceneDepth"),PPI_SceneDepth,true},
            {TEXT("SceneNormal"),PPI_WorldNormal,false},{TEXT("SceneRoughness"),PPI_Roughness,true},
            {TEXT("SceneBaseColor"),PPI_BaseColor,false}};
        for(const auto& T:Textures){auto* N=Node<UMaterialExpressionSceneTexture>(M);N->SceneTextureId=T.Id;Input(C,T.Name,N,T.bScalar);}
        M->GetEditorOnlyData()->EmissiveColor.Connect(0,C);
    }
    M->PostEditChange(); return Save(M);
}

bool BuildArtistCompatibility()
{
    const TCHAR* MaterialPath=TEXT("/YogVision/AirFog/M_AirFogArtistCompat");
    const TCHAR* InstancePath=TEXT("/YogVision/AirFog/MI_AirFogArtistCompat");
    if(FPackageName::DoesPackageExist(InstancePath))
    {
        if(!FParse::Param(FCommandLine::Get(),TEXT("YogAirFogRebuildArtistCompat")))return true;
        // Explicitly refresh only our guard on an owned existing clone. Retain
        // any artistic edits made to this independent compatibility material.
        auto* MI=LoadObject<UMaterialInstanceConstant>(nullptr,CompatibleArtistMaterial);
        UMaterial* M=MI?MI->GetMaterial():nullptr;
        if(!M||M->GetOutermost()->GetName()!=MaterialPath)return false;
        for(UMaterialExpression* E:M->GetEditorOnlyData()->ExpressionCollection.Expressions)
            if(auto* C=Cast<UMaterialExpressionCustom>(E);C&&C->Description==TEXT("YogAirFog.ArtistCompatibility.v1"))
            {C->Code=ArtistGuardCode;M->PostEditChange();return Save(M);}
        return false;
    }
    auto* Original=LoadObject<UMaterialInstanceConstant>(nullptr,ArtistMaterial);
    UMaterial* Source=Original?Original->GetMaterial():nullptr;
    if(!Source)return false;
    // Duplicate only into the owned plugin. Original MI overrides and artistic
    // texture/function graphs are retained; the user's source assets stay intact.
    auto* M=DuplicateObject<UMaterial>(Source,CreatePackage(MaterialPath),TEXT("M_AirFogArtistCompat"));
    if(!M)return false;
    M->SetFlags(RF_Public|RF_Standalone|RF_Transactional);
    UMaterialExpressionDivide* Estimate=nullptr;
    for(UMaterialExpression* E:M->GetEditorOnlyData()->ExpressionCollection.Expressions)
        if(E->GetFName()==TEXT("MaterialExpressionDivide_2"))Estimate=Cast<UMaterialExpressionDivide>(E);
    if(!Estimate||Estimate->MaterialExpressionEditorX!=-2800||Estimate->MaterialExpressionEditorY!=-1408)return false;
    auto* SafeDenominator=Node<UMaterialExpressionMax>(M);
    SafeDenominator->A=Estimate->B;SafeDenominator->ConstB=.001f;
    Estimate->B.Connect(0,SafeDenominator);
    auto* Guard=Node<UMaterialExpressionCustom>(M);Guard->Description=TEXT("YogAirFog.ArtistCompatibility.v1");Guard->OutputType=CMOT_Float3;
    Guard->Code=ArtistGuardCode;
    auto& OldInput=Guard->Inputs.AddDefaulted_GetRef();OldInput.InputName=TEXT("ArtColor");OldInput.Input=M->GetEditorOnlyData()->EmissiveColor;
    auto* Color=Node<UMaterialExpressionSceneTexture>(M);Color->SceneTextureId=PPI_PostProcessInput0;Input(Guard,TEXT("SceneColor"),Color);
    auto* Depth=Node<UMaterialExpressionSceneTexture>(M);Depth->SceneTextureId=PPI_SceneDepth;Input(Guard,TEXT("SceneDepth"),Depth,true);
    M->GetEditorOnlyData()->EmissiveColor.Connect(0,Guard);
    M->PostEditChange();FAssetRegistryModule::AssetCreated(M);
    if(!Save(M))return false;
    auto* MI=DuplicateObject<UMaterialInstanceConstant>(Original,CreatePackage(InstancePath),TEXT("MI_AirFogArtistCompat"));
    if(!MI)return false;
    MI->SetFlags(RF_Public|RF_Standalone|RF_Transactional);MI->SetParentEditorOnly(M);MI->PostEditChange();
    FAssetRegistryModule::AssetCreated(MI);return Save(MI);
}

struct FArtSlot
{
    TWeakObjectPtr<APostProcessVolume> Volume;
    TStrongObjectPtr<UMaterialInstanceDynamic> Replacement;
    TStrongObjectPtr<UMaterialInterface> Original;
    float Weight=1;
    int32 Index=INDEX_NONE;
};
TArray<FArtSlot> Slots;
TWeakObjectPtr<AYogAirFogVolume> PreviewActor;
TWeakObjectPtr<UWorld> PreviewWorld;
TWeakObjectPtr<UYogVisionSubsystem> LegacySystem;
bool bLegacyEnabled=false;
TMap<TWeakObjectPtr<UWorld>,int32> Saves;
TArray<TWeakObjectPtr<AYogAirFogVolume>> SuspendedActors;
bool bSuspended=false;

bool OwnsWorld(const UWorld* W,const UWorld* Saving)
{
    return W&&Saving&&(W==Saving||(Saving->PersistentLevel&&W->GetLevels().Contains(Saving->PersistentLevel)));
}
bool Owns(UWorld* Saving)
{
    return OwnsWorld(PreviewWorld.Get(),Saving);
}
void RestoreArt()
{
    for(auto& S:Slots) if(auto* V=S.Volume.Get())
        for(int32 Index=0;Index<V->Settings.WeightedBlendables.Array.Num();++Index)
        {
            auto& B=V->Settings.WeightedBlendables.Array[Index];
            if(B.Object==S.Replacement.Get()){B.Object=S.Original.Get();S.Index=Index;}
        }
}
void ApplyArt()
{
    for(auto& S:Slots) if(auto* V=S.Volume.Get())
        if(V->Settings.WeightedBlendables.Array.IsValidIndex(S.Index))
        {
            auto& B=V->Settings.WeightedBlendables.Array[S.Index];
            if(B.Object==S.Original.Get()) B.Object=S.Replacement.Get();
        }
}
void SnapshotArt(UWorld* W)
{
    for(TActorIterator<APostProcessVolume> It(W);It;++It)
        for(int32 Index=0;Index<It->Settings.WeightedBlendables.Array.Num();++Index)
        {
            auto& B=It->Settings.WeightedBlendables.Array[Index];
            if(auto* M=Cast<UMaterialInterface>(B.Object);M&&M->GetPathName()==ArtistMaterial)
            {
                UMaterialInterface* Compatible=LoadObject<UMaterialInterface>(nullptr,CompatibleArtistMaterial);
                auto* MID=UMaterialInstanceDynamic::Create(Compatible?Compatible:M,GetTransientPackage());
                // Audited parent graph: height mask becomes 1, cancelling ONLY its fog branch.
                MID->SetScalarParameterValue(TEXT("FogHeight"),-2750.f);
                MID->SetScalarParameterValue(TEXT("FogContrast"),2750.f);
                FArtSlot S;S.Volume=*It;S.Original.Reset(M);S.Replacement.Reset(MID);S.Weight=B.Weight;S.Index=Index;
                Slots.Add(MoveTemp(S)); B.Object=MID;
            }
        }
}
void TryResume()
{
    bool bSavingPreview=false;
    for(const auto& Pair:Saves) if(Pair.Value>0&&Owns(Pair.Key.Get())) bSavingPreview=true;
    if(bSuspended&&!bSavingPreview){bSuspended=false;ApplyArt();}
    for(int32 Index=SuspendedActors.Num()-1;Index>=0;--Index)
    {
        auto* A=SuspendedActors[Index].Get();
        if(!A||!A->GetWorld()||A->GetWorld()->bIsTearingDown){SuspendedActors.RemoveAtSwap(Index);continue;}
        bool bSaving=false;
        for(const auto& Pair:Saves) if(Pair.Value>0&&OwnsWorld(A->GetWorld(),Pair.Key.Get())) bSaving=true;
        if(!bSaving){A->SuspendForSave(false);SuspendedActors.RemoveAtSwap(Index);}
    }
}
}

bool FYogAirFogEditorService::BuildAssets(FText& Message)
{
    auto P=IPluginManager::Get().FindPlugin(TEXT("YogVision")); FString Density,Finish,Flow,Regions;
    bool bOK=P&&FFileHelper::LoadFileToString(Density,*(P->GetBaseDir()/TEXT("Resources/ScreenFogDensity.hlsl")))
        &&FFileHelper::LoadFileToString(Finish,*(P->GetBaseDir()/TEXT("Resources/ScreenFogFinish.hlsl")))
        &&FFileHelper::LoadFileToString(Flow,*(P->GetBaseDir()/TEXT("Resources/ScreenBrushFlow.hlsl")))
        &&FFileHelper::LoadFileToString(Regions,*(P->GetBaseDir()/TEXT("Resources/ScreenFogShared.hlsl")));
    if(bOK) bOK=AirFogEditor::BuildScreenTexture(TEXT("T_ScreenVisionStrokes"),TEXT("T_VisionStrokes.png"),false)
        &&AirFogEditor::BuildScreenTexture(TEXT("T_StaticCharcoal"),TEXT("T_StaticCharcoal.png"),false)
        &&AirFogEditor::BuildScreenTexture(TEXT("T_ScreenBrushFlow"),TEXT("T_ScreenBrushFlow.png"),true)
        &&AirFogEditor::BuildMaterial(true,Flow+TEXT("\n")+Regions+TEXT("\n")+Density)
        &&AirFogEditor::BuildMaterial(false,Flow+TEXT("\n")+Regions+TEXT("\n")+Finish)&&AirFogEditor::BuildArtistCompatibility();
    Message=FText::FromString(bOK?TEXT("空气雾与远景结构资源已就绪。现有美术资产保持原样。"):TEXT("空气雾资源准备失败，请检查日志或已修改的生成材质。"));
    UE_LOG(LogTemp,Display,TEXT("YOG_AIR_FOG_ASSETS success=%d"),bOK); return bOK;
}
bool FYogAirFogEditorService::Preview(const FYogAirFogParameters& P,AActor* Focus,bool bFinish,FText& Message)
{
    UWorld* W=GEditor?GEditor->GetEditorWorldContext().World():nullptr;
    if(!W||GEditor->PlayWorld||W->bIsTearingDown){Message=FText::FromString(TEXT("请在关卡编辑器中预览空气雾。"));return false;}
    if(!BuildAssets(Message)) return false;
    if(AirFogEditor::PreviewWorld.Get()!=W) RevertPreview();
    auto* A=AirFogEditor::PreviewActor.Get();
    if(!A)
    {
        AirFogEditor::PreviewWorld=W;
        AirFogEditor::LegacySystem=W->GetSubsystem<UYogVisionSubsystem>();
        if(auto* V=AirFogEditor::LegacySystem.Get()){AirFogEditor::bLegacyEnabled=V->IsEnabled();V->SetEnabled(false);}
        AirFogEditor::SnapshotArt(W);
        FActorSpawnParameters Args;Args.ObjectFlags=RF_Transient|RF_DuplicateTransient;Args.Name=MakeUniqueObjectName(W,AYogAirFogVolume::StaticClass(),TEXT("AirFogPreview"));
        A=W->SpawnActor<AYogAirFogVolume>(FVector::ZeroVector,FRotator::ZeroRotator,Args);
        if(!A){RevertPreview();return false;}
        A->bAutoActivate=false; A->SetActorLabel(TEXT("空气雾 / 临时分层预览")); AirFogEditor::PreviewActor=A;
    }
    // Focus is retained in the public signature for old callers. All fog geometry
    // and the bounds anchor now resolve the rendered camera, independent of selection.
    A->SetPaletteMaterial(GetPaletteMaterial());
    A->bFinishEnabled=bFinish;A->ApplyParameters(P);
    if(!A->SetAirFogEnabled(true)){Message=FText::FromString(A->GetStatusText());RevertPreview();return false;}
    if(GCurrentLevelEditingViewportClient){GCurrentLevelEditingViewportClient->SetRealtime(true);GCurrentLevelEditingViewportClient->Invalidate();}
    Message=FText::FromString(A->GetStatusText());return true;
}
void FYogAirFogEditorService::UpdatePreview(const FYogAirFogParameters& P,bool bFinish)
{
    if(auto* A=AirFogEditor::PreviewActor.Get()){A->SetPaletteMaterial(GetPaletteMaterial());A->bFinishEnabled=bFinish;A->ApplyParameters(P);}
}
UMaterialInterface* FYogAirFogEditorService::GetPaletteMaterial()
{
    if (AirFogEditor::WorkbenchPalette.IsValid()) return AirFogEditor::WorkbenchPalette.Get();
    for(const auto& Slot:AirFogEditor::Slots)if(Slot.Original.IsValid())return Slot.Original.Get();
    return LoadObject<UMaterialInterface>(nullptr,AirFogEditor::ArtistMaterial);
}
void FYogAirFogEditorService::RevertPreview()
{
    AirFogEditor::RestoreArt();AirFogEditor::Slots.Reset();AirFogEditor::bSuspended=false;
    AirFogEditor::SuspendedActors.Remove(AirFogEditor::PreviewActor);
    if(auto* A=AirFogEditor::PreviewActor.Get()){A->SetAirFogEnabled(false);A->Destroy();}
    if(auto* V=AirFogEditor::LegacySystem.Get();V&&V->GetWorld()&&!V->GetWorld()->bIsTearingDown&&!V->IsEnabled())V->SetEnabled(AirFogEditor::bLegacyEnabled);
    AirFogEditor::PreviewActor.Reset();AirFogEditor::PreviewWorld.Reset();AirFogEditor::LegacySystem.Reset();
}
bool FYogAirFogEditorService::IsPreviewActive(){return AirFogEditor::PreviewActor.IsValid();}
FString FYogAirFogEditorService::GetPreviewStatus()
{
    if(auto* A=AirFogEditor::PreviewActor.Get())return A->GetStatusText();
    return TEXT("尚未开启空气雾预览；美术场景保持原状态。");
}
void FYogAirFogEditorService::BeginSave(UWorld* W)
{
    if(!W)return;
    ++AirFogEditor::Saves.FindOrAdd(W);
    if(AirFogEditor::Owns(W)&&!AirFogEditor::bSuspended)
    {
        AirFogEditor::bSuspended=true;AirFogEditor::RestoreArt();
    }
    for(TObjectIterator<AYogAirFogVolume> It;It;++It)
        if(It->GetWorld()&&It->GetWorld()->WorldType==EWorldType::Editor&&AirFogEditor::OwnsWorld(It->GetWorld(),W)
            &&It->IsAirFogEnabled()&&!AirFogEditor::SuspendedActors.Contains(*It))
        {AirFogEditor::SuspendedActors.Add(*It);It->SuspendForSave(true);}
}
void FYogAirFogEditorService::EndSave(UWorld* W)
{
    if(int32* N=AirFogEditor::Saves.Find(W)){if(--*N<=0)AirFogEditor::Saves.Remove(W);}
    AirFogEditor::TryResume();
}
void FYogAirFogEditorService::RecoverInterruptedSaves()
{
    if(AirFogEditor::Saves.Num()&&!UE::IsSavingPackage()){AirFogEditor::Saves.Reset();AirFogEditor::TryResume();}
    if(IsPreviewActive()&&GEditor&&AirFogEditor::PreviewWorld.Get()!=GEditor->GetEditorWorldContext().World())RevertPreview();
    // Newly spawned editor actors need not have gameplay tick registration.
    // Pump enabled editor media after each frame, without double-ticking worlds
    // that already ran their native actor tick. Game/PIE use the normal tick path.
    UWorld* World=GEditor&&!GEditor->PlayWorld?GEditor->GetEditorWorldContext().World():nullptr;
    if(World&&World->WorldType==EWorldType::Editor&&!World->bIsTearingDown&&!UE::IsSavingPackage())
        for(TActorIterator<AYogAirFogVolume> It(World);It;++It)
        {
            const TOptional<FVector> Camera=GCurrentLevelEditingViewportClient&&GCurrentLevelEditingViewportClient->GetWorld()==World
                ? TOptional<FVector>(GCurrentLevelEditingViewportClient->GetViewLocation()) : TOptional<FVector>();
            It->SetEditorCameraAnchor(Camera);
            if(It->IsAirFogEnabled()&&!It->HasTickedThisFrame())It->Tick(static_cast<float>(FApp::GetDeltaTime()));
        }
}
bool FYogAirFogEditorService::Capture(const FString& Name)
{
    if(!GCurrentLevelEditingViewportClient||!GCurrentLevelEditingViewportClient->Viewport)return false;
    FViewport* V=GCurrentLevelEditingViewportClient->Viewport;V->Draw(false);TArray<FColor> Pixels;const FIntPoint Size=V->GetSizeXY();
    if(Size.GetMin()<32||!V->ReadPixels(Pixels))return false;
    // Scene-color ReadPixels has an unused alpha channel. Export an opaque capture.
    for(FColor& Pixel:Pixels)Pixel.A=255;
    TArray64<uint8> PNG;FImageUtils::PNGCompressImageArray(Size.X,Size.Y,Pixels,PNG);
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("AirFog/Captures");IFileManager::Get().MakeDirectory(*Dir,true);
    const FString File=Dir/(FPaths::GetBaseFilename(Name)+TEXT(".png"));bool bOK=FFileHelper::SaveArrayToFile(PNG,*File);
    UE_LOG(LogTemp,Display,TEXT("AIR_FOG_CAPTURE success=%d file=%s"),bOK,*File);return bOK;
}
namespace AirFogPresets
{
FString LegacyFile() { return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AirFog/ArtDraft.json")); }

bool FileForName(const FString& Name, FString& Out, FText& Message)
{
    if (!FYogAirFogEditorService::ValidatePresetName(Name, Message)) return false;
    const FString Directory = FYogAirFogEditorService::GetPresetDirectory();
    Out = FPaths::ConvertRelativePathToFull(Directory / (Name + TEXT(".json")));
    if (!FPaths::IsUnderDirectory(Out, Directory))
    {
        Message = FText::FromString(TEXT("草稿路径必须位于 Saved/AirFog/Presets。"));
        return false;
    }
    return true;
}

bool ValidPalette(const FSoftObjectPath& Path, FText& Message)
{
    UMaterialInterface* Material = Path.IsValid() && Path.GetSubPathString().IsEmpty()
        ? Cast<UMaterialInterface>(Path.TryLoad()) : nullptr;
    if (!IsValid(Material) || Material->IsA<UMaterialInstanceDynamic>()
        || Material->HasAnyFlags(RF_Transient) || Material->GetOutermost() == GetTransientPackage())
    {
        Message = FText::FromString(TEXT("配色材质路径无效或指向临时MID。请选择能够重新加载的材质资产；草稿未改变。"));
        return false;
    }
    return true;
}

TSharedRef<FJsonObject> ParameterObject(FYogAirFogParameters Parameters)
{
    Parameters.Sanitize();
    auto Object = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
    {
        if (auto* Bool = CastField<FBoolProperty>(*It)) Object->SetBoolField(It->GetName(), Bool->GetPropertyValue_InContainer(&Parameters));
        else if (auto* Float = CastField<FFloatProperty>(*It)) Object->SetNumberField(It->GetName(), Float->GetPropertyValue_InContainer(&Parameters));
        else if (auto* Int = CastField<FIntProperty>(*It)) Object->SetNumberField(It->GetName(), Int->GetPropertyValue_InContainer(&Parameters));
        else if (auto* Soft = CastField<FSoftObjectProperty>(*It))
            Object->SetStringField(It->GetName(), Soft->GetPropertyValue_InContainer(&Parameters).ToSoftObjectPath().ToString());
        else if (auto* Struct = CastField<FStructProperty>(*It); Struct && Struct->Struct == TBaseStructure<FLinearColor>::Get())
            Object->SetStringField(It->GetName(), Struct->ContainerPtrToValuePtr<FLinearColor>(&Parameters)->ToString());
    }
    return Object;
}

bool Read(const FString& File, FYogAirFogPresetData& Out, FText& Message)
{
    FString JSON;
    TSharedPtr<FJsonObject> Object;
    if (!FFileHelper::LoadFileToString(JSON, *File)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JSON), Object) || !Object.IsValid())
    {
        Message = FText::FromString(TEXT("草稿不存在或JSON损坏；当前草稿保持不变。"));
        return false;
    }
    double Version = 1;
    if ((Object->HasField(TEXT("version")) && !Object->TryGetNumberField(TEXT("version"), Version))
        || !FMath::IsFinite(Version) || Version < 1 || Version > 3 || Version != FMath::FloorToDouble(Version))
    {
        Message = FText::FromString(TEXT("草稿版本无效或高于当前支持的版本3；当前草稿保持不变。"));
        return false;
    }
    FYogAirFogPresetData Loaded;
    Loaded.PaletteMaterial = FSoftObjectPath(AirFogEditor::ArtistMaterial);
    TSharedPtr<FJsonObject> Parameters = Object;
    if (Version == 3)
    {
        const TSharedPtr<FJsonObject>* Inner = nullptr;
        FString Palette;
        if (!Object->TryGetObjectField(TEXT("parameters"), Inner) || !Inner || !Inner->IsValid()
            || !Object->TryGetBoolField(TEXT("finishEnabled"), Loaded.bFinishEnabled)
            || !Object->TryGetStringField(TEXT("paletteMaterial"), Palette))
        {
            Message = FText::FromString(TEXT("版本3草稿缺少参数、远景开关或配色材质身份；当前草稿保持不变。"));
            return false;
        }
        Parameters = *Inner;
        Loaded.PaletteMaterial = FSoftObjectPath(Palette);
    }
    for (TFieldIterator<FProperty> It(FYogAirFogParameters::StaticStruct()); It; ++It)
    {
        const FString Field = It->GetName();
        if (!Parameters->HasField(Field)) continue;
        double Number = 0;
        bool Value = false;
        bool bValid = true;
        if (auto* Bool = CastField<FBoolProperty>(*It))
        {
            bValid = Parameters->TryGetBoolField(Field, Value);
            if (bValid) Bool->SetPropertyValue_InContainer(&Loaded.Parameters, Value);
        }
        else if (auto* Float = CastField<FFloatProperty>(*It))
        {
            bValid = Parameters->TryGetNumberField(Field, Number) && FMath::IsFinite(Number) && FMath::IsFinite(static_cast<float>(Number));
            if (bValid) Float->SetPropertyValue_InContainer(&Loaded.Parameters, static_cast<float>(Number));
        }
        else if (auto* Int = CastField<FIntProperty>(*It))
        {
            bValid = Parameters->TryGetNumberField(Field, Number) && FMath::IsFinite(Number)
                && Number >= MIN_int32 && Number <= MAX_int32 && Number == FMath::FloorToDouble(Number);
            if (bValid) Int->SetPropertyValue_InContainer(&Loaded.Parameters, static_cast<int32>(Number));
        }
        else if (auto* Soft = CastField<FSoftObjectProperty>(*It))
        {
            FString Path;
            bValid = Parameters->TryGetStringField(Field, Path);
            UTexture2D* Texture = bValid ? Cast<UTexture2D>(FSoftObjectPath(Path).TryLoad()) : nullptr;
            bValid = IsValid(Texture) && !Texture->HasAnyFlags(RF_Transient)
                && Texture->GetOutermost() != GetTransientPackage();
            if (bValid) Soft->SetPropertyValue_InContainer(&Loaded.Parameters, FSoftObjectPtr(FSoftObjectPath(Path)));
        }
        else if (auto* Struct = CastField<FStructProperty>(*It); Struct && Struct->Struct == TBaseStructure<FLinearColor>::Get())
        {
            FString Colour;
            FLinearColor Parsed=FLinearColor::Black;
            bValid = Parameters->TryGetStringField(Field, Colour) && Parsed.InitFromString(Colour)
                && FMath::IsFinite(Parsed.R) && FMath::IsFinite(Parsed.G) && FMath::IsFinite(Parsed.B) && FMath::IsFinite(Parsed.A);
            if (bValid) *Struct->ContainerPtrToValuePtr<FLinearColor>(&Loaded.Parameters) = Parsed;
        }
        if (!bValid)
        {
            Message = FText::FromString(FString::Printf(TEXT("草稿参数“%s”的类型或值无效；当前草稿保持不变。"), *Field));
            return false;
        }
    }
    if (Version < 2)
    {
        const FYogAirFogParameters Defaults;
        Loaded.Parameters.NearOffset = Defaults.NearOffset;
        Loaded.Parameters.FarStart = Defaults.FarStart;
        Loaded.Parameters.FarEnd = Defaults.FarEnd;
    }
    Loaded.Parameters.Sanitize();
    if (!ValidPalette(Loaded.PaletteMaterial, Message)) return false;
    Out = Loaded;
    Message = FText::FromString(Version < 3 ? TEXT("已读入旧草稿，补齐远景开关与原美术配色身份；旧文件保持原样。") : TEXT("已读入命名美术草稿。"));
    return true;
}

bool Write(const FString& File, const FYogAirFogPresetData& Data, bool bAllowOverwrite, FText& Message)
{
    if (!ValidPalette(Data.PaletteMaterial, Message)) return false;
    FYogAirFogParameters SafeParameters = Data.Parameters;
    SafeParameters.Sanitize();
    for (const TSoftObjectPtr<UTexture2D>& Path : { SafeParameters.StrokeTexture, SafeParameters.FlowTexture })
    {
        UTexture2D* Texture = Path.LoadSynchronous();
        if (!IsValid(Texture) || Texture->HasAnyFlags(RF_Transient) || Texture->GetOutermost() == GetTransientPackage())
        {
            Message = FText::FromString(TEXT("笔触或方向流图必须是可重新加载的Texture2D资产；原草稿保持不变。"));
            return false;
        }
    }
    if (!bAllowOverwrite && IFileManager::Get().FileExists(*File))
    {
        Message = FText::FromString(TEXT("同名草稿已存在。请选择新名称另存，或先读入该文件再保存当前；未覆盖任何文件。"));
        return false;
    }
    auto Object = MakeShared<FJsonObject>();
    Object->SetNumberField(TEXT("version"), 3);
    Object->SetObjectField(TEXT("parameters"), ParameterObject(Data.Parameters));
    Object->SetBoolField(TEXT("finishEnabled"), Data.bFinishEnabled);
    Object->SetStringField(TEXT("paletteMaterial"), Data.PaletteMaterial.ToString());
    FString JSON;
    if (!FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&JSON)))
    {
        Message = FText::FromString(TEXT("草稿序列化失败；原文件保持不变。"));
        return false;
    }
    const FString Directory = FPaths::GetPath(File);
    if (!IFileManager::Get().MakeDirectory(*Directory, true))
    {
        Message = FText::FromString(TEXT("无法创建草稿目录；原文件保持不变。"));
        return false;
    }
    auto UniqueSibling = [&File](const TCHAR* Extension)
    {
        FString Candidate;
        do Candidate = File + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + Extension;
        while (IFileManager::Get().FileExists(*Candidate) || IFileManager::Get().DirectoryExists(*Candidate));
        return Candidate;
    };
    const FString Temporary = UniqueSibling(TEXT(".tmp"));
    if (!FFileHelper::SaveStringToFile(JSON, *Temporary, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(), FILEWRITE_NoReplaceExisting))
    {
        Message = FText::FromString(FString::Printf(TEXT("新草稿暂存写入未完成，未替换目标文件。请保留并核对临时路径：%s"), *Temporary));
        return false;
    }

    // IFileManager::Move(Replace=true) deletes the destination before its rename.
    // Retain a verified copy before entering that recoverable, non-atomic step.
    const bool bHadOriginal = IFileManager::Get().FileExists(*File);
    FString Backup;
    TArray<uint8> OriginalBytes;
    if (bHadOriginal)
    {
        if (!bAllowOverwrite)
        {
            Message = FText::FromString(FString::Printf(TEXT("同名文件在保存期间出现，未覆盖它。新内容暂存在：%s"), *Temporary));
            return false;
        }
        Backup = UniqueSibling(TEXT(".bak"));
        TArray<uint8> VerifiedBackup;
        const bool bBackupReady = FFileHelper::LoadFileToArray(OriginalBytes, *File)
            && FFileHelper::SaveArrayToFile(OriginalBytes, *Backup, &IFileManager::Get(), FILEWRITE_NoReplaceExisting)
            && FFileHelper::LoadFileToArray(VerifiedBackup, *Backup) && VerifiedBackup == OriginalBytes;
        if (!bBackupReady)
        {
            Message = FText::FromString(FString::Printf(TEXT("旧草稿备份未通过字节核对，未替换目标。原文件：%s\n新内容暂存：%s\n备份候选：%s"), *File, *Temporary, *Backup));
            return false;
        }
    }

    if (!IFileManager::Get().Move(*File, *Temporary, bHadOriginal && bAllowOverwrite, false, false, true))
    {
        if (bHadOriginal && !IFileManager::Get().FileExists(*File))
        {
            TArray<uint8> Restored;
            const bool bRestored = FFileHelper::SaveArrayToFile(OriginalBytes, *File, &IFileManager::Get(), FILEWRITE_NoReplaceExisting)
                && FFileHelper::LoadFileToArray(Restored, *File) && Restored == OriginalBytes;
            Message = FText::FromString(FString::Printf(TEXT("替换草稿失败，%s。恢复用旧内容备份：%s\n新内容暂存：%s\n目标文件：%s"),
                bRestored ? TEXT("已核对并恢复原文件") : TEXT("原文件自动恢复未通过核对，请从备份恢复"), *Backup, *Temporary, *File));
        }
        else
        {
            Message = FText::FromString(bHadOriginal
                ? FString::Printf(TEXT("替换草稿失败，目标文件仍存在，请核对。旧内容备份：%s\n新内容暂存：%s\n目标文件：%s"), *Backup, *Temporary, *File)
                : FString::Printf(TEXT("新草稿未能移至目标文件，未更新保存状态。新内容保留在：%s"), *Temporary));
        }
        // Never remove the only new contents or the recovery copy on failure.
        return false;
    }
    const bool bBackupRemoved = Backup.IsEmpty() || !IFileManager::Get().FileExists(*Backup)
        || IFileManager::Get().Delete(*Backup, false, false, true);
    Message = FText::FromString(bBackupRemoved
        ? FString::Printf(TEXT("已保存美术草稿：%s"), *File)
        : FString::Printf(TEXT("美术草稿已保存；旧内容备份未能清理，可稍后处理：%s\n当前文件：%s"), *Backup, *File));
    return true;
}
}

void FYogAirFogEditorService::SetPreviewPaletteMaterial(UMaterialInterface* Material)
{
    AirFogEditor::WorkbenchPalette.Reset(Material);
}

FString FYogAirFogEditorService::GetPresetDirectory()
{
    return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AirFog/Presets"));
}

FString FYogAirFogEditorService::GetLegacyPresetId() { return TEXT("__legacy_artdraft__"); }

bool FYogAirFogEditorService::ValidatePresetName(const FString& Name, FText& Message)
{
    bool bInvalid = Name.IsEmpty() || Name.Len() > 64 || Name != Name.TrimStartAndEnd() || Name.EndsWith(TEXT("."))
        || Name.Contains(TEXT("..")) || Name.Equals(GetLegacyPresetId(), ESearchCase::IgnoreCase);
    for (const TCHAR Character : Name)
        bInvalid |= Character < 32 || Character == 127 || FString(TEXT("<>:\"/\\|?*")).Contains(FString::Chr(Character));
    int32 Dot = INDEX_NONE;
    Name.FindChar(TCHAR('.'), Dot);
    const FString Device = (Dot == INDEX_NONE ? Name : Name.Left(Dot)).ToUpper();
    bInvalid |= Device == TEXT("CON") || Device == TEXT("PRN") || Device == TEXT("AUX") || Device == TEXT("NUL") || Device == TEXT("CLOCK$");
    if (Device.Len() == 4 && (Device.StartsWith(TEXT("COM")) || Device.StartsWith(TEXT("LPT"))))
        bInvalid |= Device[3] >= TCHAR('1') && Device[3] <= TCHAR('9');
    if (bInvalid)
    {
        Message = FText::FromString(TEXT("草稿名须为1–64个字符，可用中文。不能含路径、控制字符、首尾空格、末尾点或Windows保留名。"));
        return false;
    }
    return true;
}

bool FYogAirFogEditorService::ListNamedPresets(TArray<FYogAirFogNamedPresetInfo>& Out, FText& Message)
{
    Out.Reset();
    TArray<FString> Files;
    IFileManager::Get().FindFiles(Files, *(GetPresetDirectory() / TEXT("*.json")), true, false);
    for (const FString& File : Files)
    {
        const FString Name = FPaths::GetBaseFilename(File);
        FText Unused;
        if (!ValidatePresetName(Name, Unused)) continue;
        FYogAirFogNamedPresetInfo Info;
        Info.Id = Name; Info.DisplayName = Name; Info.FilePath = GetPresetDirectory() / File;
        Info.ModifiedTime = IFileManager::Get().GetTimeStamp(*Info.FilePath);
        Out.Add(MoveTemp(Info));
    }
    Out.Sort([](const FYogAirFogNamedPresetInfo& A, const FYogAirFogNamedPresetInfo& B) { return A.DisplayName.Compare(B.DisplayName, ESearchCase::IgnoreCase) < 0; });
    if (IFileManager::Get().FileExists(*AirFogPresets::LegacyFile()))
    {
        FYogAirFogNamedPresetInfo Legacy;
        Legacy.Id = GetLegacyPresetId(); Legacy.DisplayName = TEXT("旧版未命名草稿"); Legacy.FilePath = AirFogPresets::LegacyFile();
        Legacy.ModifiedTime = IFileManager::Get().GetTimeStamp(*Legacy.FilePath); Legacy.bLegacy = true;
        Out.Add(MoveTemp(Legacy));
    }
    Message = FText::FromString(FString::Printf(TEXT("已刷新，可选择%d个草稿。"), Out.Num()));
    return true;
}

bool FYogAirFogEditorService::SaveNamedPreset(const FString& Name, const FYogAirFogPresetData& Data, bool bAllowOverwrite, FText& Message)
{
    FString File;
    return AirFogPresets::FileForName(Name, File, Message) && AirFogPresets::Write(File, Data, bAllowOverwrite, Message);
}

bool FYogAirFogEditorService::LoadNamedPreset(const FString& Id, FYogAirFogPresetData& Out, FText& Message)
{
    if (Id == GetLegacyPresetId()) return AirFogPresets::Read(AirFogPresets::LegacyFile(), Out, Message);
    FString File;
    return AirFogPresets::FileForName(Id, File, Message) && AirFogPresets::Read(File, Out, Message);
}

bool FYogAirFogEditorService::ImportLegacyPreset(FString& OutName, FText& Message)
{
    FYogAirFogPresetData Loaded;
    if (!LoadNamedPreset(GetLegacyPresetId(), Loaded, Message)) return false;
    FString Name = TEXT("油画卷雾_继承参数");
    for (int32 Index = 2; IFileManager::Get().FileExists(*(GetPresetDirectory() / (Name + TEXT(".json")))); ++Index)
        Name = FString::Printf(TEXT("油画卷雾_继承参数_%d"), Index);
    if (!SaveNamedPreset(Name, Loaded, false, Message)) return false;
    OutName = Name;
    return true;
}

bool FYogAirFogEditorService::SavePreset(const FYogAirFogParameters& Parameters, FText& Message)
{
    FYogAirFogPresetData Data;
    Data.Parameters = Parameters;
    Data.PaletteMaterial = FSoftObjectPath(AirFogEditor::ArtistMaterial);
    return AirFogPresets::Write(AirFogPresets::LegacyFile(), Data, true, Message);
}

bool FYogAirFogEditorService::LoadPreset(FYogAirFogParameters& Out, FText& Message)
{
    FYogAirFogPresetData Loaded;
    if (!AirFogPresets::Read(AirFogPresets::LegacyFile(), Loaded, Message)) return false;
    Out = Loaded.Parameters;
    return true;
}
