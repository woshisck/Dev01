#include "YogPlanarFogActor.h"
#include "YogFogPlanarSimulationComponent.h"
#include "YogAirFogRenderBridge.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Engine/PostProcessVolume.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectIterator.h"

namespace PlanarFog
{
FLinearColor Pack(FVector V){return FLinearColor(V.X,V.Y,V.Z,0);}
float Safe(float V,float D,float Lo,float Hi){return FMath::Clamp(FMath::IsFinite(V)?V:D,Lo,Hi);}
}
AYogPlanarFogActor::AYogPlanarFogActor()
{
    PlanarSimulation=CreateDefaultSubobject<UYogFogPlanarSimulationComponent>(TEXT("PlanarSimulation"));
    PlanarSimulation->AddTickPrerequisiteActor(this);
    bAnimateFog=true; bAnimateInEditor=true;
}
void AYogPlanarFogActor::ConfigureParent()
{
    bFogEnabled=bMistEnabled;BoundsExtent=MistExtent.GetAbs().ComponentMax(FVector(1));Density=PlanarFog::Safe(MistDensity,.0016f,0,.03f);
    bEnableVision=bClearNearObserver;ClearRadius=VisionRadius;ClearFeather=VisionFeather;ClearResidual=VisionResidual;
    bUseExplicitPreviewFocus=bPinEditorFocus;PreviewFocusPosition=EditorFocus;
    bUseArtistPalette=bReadArtistPalette;PaletteMaterial=ArtistPalette;NearColour=MistNearColour;FarColour=MistFarColour;
    FogAlbedo=MistAlbedo;PaletteDistance=ColourDistance;bUseSceneDirectionalLight=bReadSceneDirectional;FallbackLightDirection=MistLightDirection;
    ViewDistance=8000;FallbackLightStrength=0;
    if(NativeFog)
    {
        NativeFog->SetVolumetricFogStartDistance(PlanarFog::Safe(CameraFogStartDistance,1200.f,0.f,50000.f));
        NativeFog->SetVolumetricFogNearFadeInDistance(PlanarFog::Safe(CameraFogFadeDistance,700.f,0.f,50000.f));
    }
    if(!DensityPattern&&!bTriedDensityPattern){bTriedDensityPattern=true;DensityPattern=LoadObject<UTexture2D>(nullptr,TEXT("/YogVision/Planar/T_PlanarFogDensity.T_PlanarFogDensity"),nullptr,LOAD_NoWarn|LOAD_Quiet);}
    if(!BrushPattern&&!bTriedBrushPattern){bTriedBrushPattern=true;BrushPattern=LoadObject<UTexture2D>(nullptr,TEXT("/YogVision/Planar/T_StaticCharcoal.T_StaticCharcoal"),nullptr,LOAD_NoWarn|LOAD_Quiet);}
    if(!VolumeMaterial&&!bTriedPlanarMaterial){bTriedPlanarMaterial=true;VolumeMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/YogVision/Planar/M_PlanarVolumeFog.M_PlanarVolumeFog"),nullptr,LOAD_NoWarn|LOAD_Quiet);}
    // Preserve the inherited palette cache: never overwrite its loaded default with null every tick.
    if(bReadArtistPalette&&!ArtistPalette&&!bTriedArtistPalette)
    {
        bTriedArtistPalette=true;
        ArtistPalette=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline"),nullptr,LOAD_NoWarn|LOAD_Quiet);
        PaletteMaterial=ArtistPalette;
    }
    if(PlanarSimulation)
    {
        // The component is the sole playback authority; the hidden legacy atlas controls are mirrors.
        bAnimateFog=PlanarSimulation->bSimulationEnabled;
        bAnimateInEditor=PlanarSimulation->bSimulateInEditor;
        PlanarSimulation->DensityTexture=DensityPattern;
        PlanarSimulation->DensityTileSize=FVector2D(FMath::Max(PatternWorldSize,100.f));
        // Simulation is fixed to world XY. Cover the transformed volume with its world AABB,
        // rather than rotating velocity coordinates or dropping actor scale from the field.
        const FVector ScaledExtent=BoundsExtent*GetActorScale3D().GetAbs();
        const FQuat Q=GetActorQuat();
        const FVector WorldExtent=Q.GetAxisX().GetAbs()*ScaledExtent.X
            +Q.GetAxisY().GetAbs()*ScaledExtent.Y+Q.GetAxisZ().GetAbs()*ScaledExtent.Z;
        const FVector2D HalfExtent(WorldExtent.X,WorldExtent.Y);
        const FVector DomainCenter=GetActorLocation()+FVector(0,0,HeightCenter);
        if(!PlanarSimulation->ActualDomainCenter.Equals(DomainCenter,.01)
            ||!PlanarSimulation->ActualDomainHalfExtent.Equals(HalfExtent,.01))
            PlanarSimulation->SetDomain(DomainCenter,HalfExtent);
    }
}
void AYogPlanarFogActor::OnConstruction(const FTransform& T)
{ConfigureParent();Super::OnConstruction(T);BindField();}
void AYogPlanarFogActor::BeginPlay(){ConfigureParent();Super::BeginPlay();RefreshPlanarFog();}
void AYogPlanarFogActor::RefreshPlanarFog()
{
    bTriedDensityPattern=bTriedBrushPattern=bTriedPlanarMaterial=bTriedArtistPalette=false;
    ConfigureParent();RefreshMaterialParameters();
    if(bRequestNativeFog&&bMistEnabled&&!IsTemplate())
    {
        if(IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog")))
        {
            if(!bOwnFogLease){PreviousFogValue=V->GetInt();PreviousFogPriority=V->GetFlags()&ECVF_SetByMask;bOwnFogLease=true;}
            V->Set(1,ECVF_SetByConsole);
        }
    }
    BindField();
}
void AYogPlanarFogActor::ReleaseFogLease()
{
    if(!bOwnFogLease)return;
    if(IConsoleVariable* V=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog")))
        if(V->GetInt()==1&&(V->GetFlags()&ECVF_SetByMask)==ECVF_SetByConsole)
        {V->Unset(ECVF_SetByConsole);V->Set(PreviousFogValue,static_cast<EConsoleVariableFlags>(PreviousFogPriority));}
    bOwnFogLease=false;
}
void AYogPlanarFogActor::EndPlay(const EEndPlayReason::Type R){RestoreLegacyFog();YogAirFogRenderBridge::UnregisterPlanarVolume(this);bTransportRegistered=false;FogTransport=nullptr;FogBrushLayer=nullptr;ReleaseFogLease();Super::EndPlay(R);}
void AYogPlanarFogActor::Destroyed(){RestoreLegacyFog();YogAirFogRenderBridge::UnregisterPlanarVolume(this);ReleaseFogLease();Super::Destroyed();}
void AYogPlanarFogActor::PausePlanarFog()
{
    if(PlanarSimulation)PlanarSimulation->bSimulationEnabled=false;
    ConfigureParent();SynchronizeClock();
}
void AYogPlanarFogActor::ResumePlanarFog()
{
    if(PlanarSimulation)
    {
        PlanarSimulation->bSimulationEnabled=true;
        if(GetWorld()&&!GetWorld()->IsGameWorld())PlanarSimulation->bSimulateInEditor=true;
    }
    ConfigureParent();SynchronizeClock();
}
void AYogPlanarFogActor::SynchronizeClock()
{
    if(!PlanarSimulation)return;
    FogTimeSeconds=PlanarSimulation->SimulationTime;
    const bool bGame=GetWorld()&&GetWorld()->IsGameWorld();
    AnimationStatus=!PlanarSimulation->bSimulationEnabled?TEXT("平面流动已暂停 / planar paused")
        :(!bGame&&!PlanarSimulation->bSimulateInEditor?TEXT("编辑器流动已暂停 / editor paused")
        :TEXT("世界XY固定步进 / world XY fixed-step simulation"));
}
#if WITH_EDITOR
void AYogPlanarFogActor::PostEditChangeProperty(FPropertyChangedEvent& E)
{ConfigureParent();Super::PostEditChangeProperty(E);BindField();if(!bMistEnabled)ReleaseFogLease();}
void AYogPlanarFogActor::SetEditorLegacyFogOverrideSuspended(bool bSuspend)
{
    if(bEditorLegacyFogOverrideSuspended==bSuspend)return;
    bEditorLegacyFogOverrideSuspended=bSuspend;
    if(bSuspend)RestoreLegacyFog();
    else if(GetWorld()&&!GetWorld()->bIsTearingDown)UpdateLegacyFogOverride();
}
#endif
void AYogPlanarFogActor::BindField()
{
    if(!bMistEnabled&&bTransportRegistered)
    {YogAirFogRenderBridge::UnregisterPlanarVolume(this);bTransportRegistered=false;FogTransport=nullptr;}
    if(!bTransportRegistered&&!IsTemplate()&&GetWorld()&&bMistEnabled)
    {YogAirFogRenderBridge::RegisterPlanarVolume(this);bTransportRegistered=true;}
    UMaterialInstanceDynamic* M=GetFogMID();if(!M||!PlanarSimulation)return;
    using namespace PlanarFog;
    const bool bReady=IsValid(PlanarSimulation->PlanarField)&&PlanarSimulation->PlanarField->GetResource();
    M->SetScalarParameterValue(TEXT("SimulationReady"),bReady?1.f:0.f);
    if(bReady)M->SetTextureParameterValue(TEXT("PlanarField"),PlanarSimulation->PlanarField);
    if(DensityPattern)M->SetTextureParameterValue(TEXT("DensityPattern"),DensityPattern);
    M->SetVectorParameterValue(TEXT("SimulationCenter"),Pack(PlanarSimulation->ActualDomainCenter));
    const FVector2D E=PlanarSimulation->ActualDomainHalfExtent;
    M->SetVectorParameterValue(TEXT("SimulationExtent"),FLinearColor(E.X,E.Y,1,0));
    M->SetScalarParameterValue(TEXT("FlowTime"),PlanarSimulation->SimulationTime);
    SynchronizeClock();
    M->SetScalarParameterValue(TEXT("FogTimeSeconds"),FogTimeSeconds);
    const auto Set=[M](const TCHAR* N,float V){M->SetScalarParameterValue(N,FMath::IsFinite(V)?V:0);};
    Set(TEXT("AmbientStrength"),AmbientStrength);Set(TEXT("HeightCenter"),GetActorLocation().Z+HeightCenter);
    Set(TEXT("HeightHalfSize"),FMath::Max(HeightHalfSize,1.f));Set(TEXT("HeightSoftness"),FMath::Max(HeightSoftness,1.f));
    Set(TEXT("DensityFloor"),DensityFloor);Set(TEXT("DensityPower"),DensityPower);Set(TEXT("PatternWorldSize"),FMath::Max(PatternWorldSize,100.f));
    Set(TEXT("NoiseWorldSize"),FMath::Max(NoiseWorldSize,10.f));Set(TEXT("NoiseStrength"),NoiseStrength);
    Set(TEXT("FlowDisplacement"),FlowDisplacement);
    UpdateSurfaceFinish();
}
void AYogPlanarFogActor::UpdateSurfaceFinish()
{
    if(!SurfaceFinishMID&&!bTriedSurfaceFinish)
    {
        bTriedSurfaceFinish=true;
        if(auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/YogVision/Planar/M_PlanarFogFinish.M_PlanarFogFinish"),nullptr,LOAD_NoWarn|LOAD_Quiet))
        {SurfaceFinishMID=UMaterialInstanceDynamic::Create(Base,this);SurfaceFinishMID->SetFlags(RF_Transient);}
    }
    if(SurfaceFinishMID&&GetFogMID())
    {
        // Architecture only; brush mapping consumes native fog scattering before the artist PP chain.
        FLinearColor Direction;
        if(GetFogMID()->GetVectorParameterValue(FMaterialParameterInfo(TEXT("FallbackLightDirection")),Direction))
            SurfaceFinishMID->SetVectorParameterValue(TEXT("FallbackLightDirection"),Direction);
        SurfaceFinishMID->SetScalarParameterValue(TEXT("FormStrength"),FMath::Clamp(FormStrength,0.f,2.f));
    }
}
void AYogPlanarFogActor::RestoreLegacyFog()
{
    for(const auto& S:ArtistOverrides)
        if(auto* V=S.Volume.Get())
            if(V->Settings.WeightedBlendables.Array.IsValidIndex(S.Index)&&V->Settings.WeightedBlendables.Array[S.Index].Object==S.Applied.Get())
                V->Settings.WeightedBlendables.Array[S.Index]=S.Original;
    ArtistOverrides.Empty();ArtistOriginalReferences.Empty();bLegacyChecked=false;
    for(auto Entry:SuppressedHeightFog)if(auto* C=Entry.Get())if(!C->IsVisible())C->SetVisibility(true,false);
    SuppressedHeightFog.Empty();
}
void AYogPlanarFogActor::UpdateLegacyFogOverride()
{
#if WITH_EDITOR
    // Saving serializes other actors too, and PIE duplicates their current properties.
    // Keep both authored EHF visibility and PP blendables restored for that whole interval.
    if(bEditorLegacyFogOverrideSuspended){if(bLegacyChecked)RestoreLegacyFog();return;}
#endif
    if(!bSuppressLegacyFog||!bMistEnabled){if(bLegacyChecked)RestoreLegacyFog();return;}
    if(bLegacyChecked||!GetWorld()||IsTemplate())return;
    bLegacyChecked=true;
    for(TObjectIterator<UExponentialHeightFogComponent> It;It;++It)
        if(!It->IsTemplate()&&It->GetWorld()==GetWorld()&&*It!=NativeFog&&It->IsVisible())
        {SuppressedHeightFog.Add(*It);It->SetVisibility(false,false);}
    for(TActorIterator<APostProcessVolume> It(GetWorld());It;++It)
        for(int32 I=0;I<It->Settings.WeightedBlendables.Array.Num();++I)
        {
            FWeightedBlendable& B=It->Settings.WeightedBlendables.Array[I];
            auto* Original=Cast<UMaterialInterface>(B.Object);if(!Original)continue;
            UMaterialInterface* Source=Original;
            if(auto* Dynamic=Cast<UMaterialInstanceDynamic>(Source))Source=Dynamic->Parent;
            if(!Source||!Source->GetPathName().Contains(TEXT("PencilPostOutline")))continue;
            float Distance=0;if(!Source->GetScalarParameterValue(FMaterialParameterInfo(TEXT("scene_depth_fog")),Distance))continue;
            auto* Override=UMaterialInstanceDynamic::Create(Source,this);Override->SetFlags(RF_Transient);
            Override->SetScalarParameterValue(TEXT("scene_depth_fog"),100000000.f);
            FPlanarFogArtistOverride S;S.Volume=*It;S.Index=I;S.Original=B;S.Applied=Override;
            ArtistOriginalReferences.Add(Original);ArtistOverrides.Add(S);B.Object=Override;
        }
}
void AYogPlanarFogActor::Tick(float Dt)
{
    ConfigureParent();Super::Tick(Dt);
    UpdateLegacyFogOverride();
    if(PlanarSimulation)
    {
        TArray<FYogFogSimBody> Bodies;
        UWorld* W=GetWorld();
        if(W&&W->IsGameWorld())for(auto It=W->GetPlayerControllerIterator();It;++It)
        {
            APlayerController* PC=It->Get();
            APawn* P=PC&&PC->IsLocalController()?PC->GetPawn():nullptr;if(!P)continue;
            FYogFogSimBody B;B.Center=P->GetActorLocation();B.Velocity=P->GetVelocity();
            if(const UCapsuleComponent* C=P->FindComponentByClass<UCapsuleComponent>()){B.Radius=C->GetScaledCapsuleRadius();B.HalfHeight=C->GetScaledCapsuleHalfHeight();}
            Bodies.Add(B);break;
        }
        PlanarSimulation->SetBodies(Bodies);
    }
    MaterialRetry+=FMath::Max(0.f,Dt);
    if(MaterialRetry>1.f){MaterialRetry=0;if(!IsFogMaterialBound()){VolumeMaterial=nullptr;bTriedPlanarMaterial=false;ConfigureParent();RefreshMaterialParameters();}}
    if(!bMistEnabled)ReleaseFogLease();
    BindField();
}
FString AYogPlanarFogActor::GetPlanarFogStatus() const
{
    static const IConsoleVariable* FogSwitch=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"));
    return FString::Printf(TEXT("世界XY密度/速度 → Z体积重建 → UE原生雾照明 → 雾散射笔触映射\nmaterial=%s; owned-MID-bound=%s; r.VolumetricFog=%d\nobserver=%s\npalette=%s\n%s\n控制：PlanarSimulation播放开关；纹理来源：Actor雾团形态\n笔触仅修饰原生散射光，不改变雾密度/透射率；建筑后处理不画笔触\nFogTransport=RGB预曝光L / A到场景深度的T；FogBrushLayer=雾散射差值RGB/裁剪深度A"),
        IsValid(VolumeMaterial)?*VolumeMaterial->GetPathName():TEXT("missing"),IsFogMaterialBound()?TEXT("true"):TEXT("false"),
        FogSwitch?FogSwitch->GetInt():-1,*ObserverSource,*PaletteSource,
        PlanarSimulation?*PlanarSimulation->GetSimulationStatus():TEXT("缺少二维组件"));
}
