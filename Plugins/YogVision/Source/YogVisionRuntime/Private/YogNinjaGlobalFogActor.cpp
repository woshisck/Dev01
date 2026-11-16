#include "YogNinjaGlobalFogActor.h"

#include "Components/PostProcessComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace YogGlobalFog
{
using FControllers=TArray<TWeakObjectPtr<AYogNinjaGlobalFogActor>>;
TMap<TWeakObjectPtr<UWorld>,FControllers> Controllers;
float Safe(float Value,float Default,float Min,float Max)
{
    return FMath::Clamp(FMath::IsFinite(Value)?Value:Default,Min,Max);
}
bool IsPostProcess(UMaterialInterface* Material)
{
    return Material && Material->GetMaterial() && Material->GetMaterial()->MaterialDomain==MD_PostProcess;
}
}

AYogNinjaGlobalFogActor::AYogNinjaGlobalFogActor()
{
    FogPostProcess=CreateDefaultSubobject<UPostProcessComponent>(TEXT("FogPostProcess"));
    FogPostProcess->SetupAttachment(SceneRoot);
    FogPostProcess->bUnbound=true;
    FogPostProcess->BlendWeight=0.f;
    FinishPostProcess=CreateDefaultSubobject<UPostProcessComponent>(TEXT("FinishPostProcess"));
    FinishPostProcess->SetupAttachment(SceneRoot);
    FinishPostProcess->bUnbound=true;
    FinishPostProcess->BlendWeight=0.f;
    GlobalFogMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/YogVision/Global/MI_NinjaGlobalFog.MI_NinjaGlobalFog")));
    InputMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_GroundMist_Input.MI_Corridor_GroundMist_Input")));
    FinishMaterial=TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Art/Rendering/Fog/Ninja/MI_Corridor_FogFinish.MI_Corridor_FogFinish")));
    bSpatialHeightEnabled=false;
    GroundBodyHeight=160.f;GroundAmount=2.5f;UpperAmount=.85f;ContactBoost=.18f;
    PlayerRadius=280.f;PlayerStrength=.72f;PlayerFeather=550.f;PlayerHeight=650.f;
    LightStrength=.6f;LightFeather=500.f;
    Tags.AddUnique(TEXT("YogNinjaGlobalFog"));
}

bool AYogNinjaGlobalFogActor::CanUpdate() const
{
    const UWorld* World=GetWorld();
    if (IsTemplate() || IsRunningCommandlet() || !World || World->bIsTearingDown
        || World->GetNetMode()==NM_DedicatedServer) return false;
#if WITH_EDITOR
    if (World->WorldType==EWorldType::Editor && GEditor
        && (GEditor->PlayWorld || GEditor->bIsSimulatingInEditor)) return false;
#endif
    return World->IsGameWorld() || World->WorldType==EWorldType::Editor;
}

void AYogNinjaGlobalFogActor::RegisterGlobalFog()
{
    if (IsTemplate() || !GetWorld()) return;
    for (auto It=YogGlobalFog::Controllers.CreateIterator();It;++It)
        if (!It.Key().IsValid()) It.RemoveCurrent();
    YogGlobalFog::Controllers.FindOrAdd(GetWorld()).AddUnique(this);
}

void AYogNinjaGlobalFogActor::UnregisterGlobalFog()
{
    if (YogGlobalFog::FControllers* Actors=YogGlobalFog::Controllers.Find(GetWorld()))
    {
        Actors->RemoveAll([this](const auto& Actor){return !Actor.IsValid() || Actor.Get()==this;});
        if (Actors->IsEmpty()) YogGlobalFog::Controllers.Remove(GetWorld());
    }
}

bool AYogNinjaGlobalFogActor::IsSelectedGlobalFog() const
{
    const YogGlobalFog::FControllers* Actors=YogGlobalFog::Controllers.Find(GetWorld());
    if (!Actors || !bFogEnabled) return false;
    for (const auto& Weak: *Actors)
    {
        const AYogNinjaGlobalFogActor* Other=Weak.Get();
        if (!Other || Other==this || !Other->bFogEnabled || Other->IsActorBeingDestroyed()) continue;
        if (Other->GlobalPriority>GlobalPriority
            || (Other->GlobalPriority==GlobalPriority && Other->GetPathName()<GetPathName())) return false;
    }
    return true;
}

UMaterialInstanceDynamic* AYogNinjaGlobalFogActor::ResolveOutputMaterial() const
{
    return bFogEnabled && IsSelectedGlobalFog()?DynamicMaterialInstance.Get():nullptr;
}

bool AYogNinjaGlobalFogActor::ResolveMaskDomain(UMaterialInstanceDynamic* Material,
    const FVector& ObserverPosition,FVector& Minimum,FVector& Size) const
{
    if (ObserverPosition.ContainsNaN()) return false;
    const double Radius=YogGlobalFog::Safe(InteractionRadius,6000.f,1000.f,30000.f);
    const double Grid=FMath::Min(static_cast<double>(YogGlobalFog::Safe(CacheSnapSize,1000.f,100.f,4000.f)),Radius*.5);
    const FVector Center(FMath::GridSnap(ObserverPosition.X,Grid),FMath::GridSnap(ObserverPosition.Y,Grid),0);
    const double Ground=YogGlobalFog::Safe(ManualGroundHeight,0.f,-1000000.f,1000000.f);
    // Cache only the interaction-rich lower atmosphere, never the extent of
    // the rendered medium. The shader blends to analytic factors outside it.
    const double Top=FMath::Clamp(3.*YogGlobalFog::Safe(FogHeight,450.f,50.f,50000.f),1000.,3000.);
    Minimum=FVector(Center.X-Radius,Center.Y-Radius,Ground-100.);
    Size=FVector(Radius*2.,Radius*2.,Top+100.);
    return true;
}

void AYogNinjaGlobalFogActor::UpdateGlobalFog(float DeltaSeconds,bool bForce)
{
    if (!CanUpdate()) return;
    // The inherited local height-cap mode must never truncate a global
    // atmosphere, including instances loaded with an older serialized value.
    bSpatialHeightEnabled=false;
    RegisterGlobalFog();
    const bool bActive=bFogEnabled && IsSelectedGlobalFog();
    FogPostProcess->bUnbound=true;FinishPostProcess->bUnbound=true;
    if (!bActive)
    {
        FogPostProcess->BlendWeight=0.f;FinishPostProcess->BlendWeight=0.f;
        GlobalStatus=bFogEnabled?TEXT("待机：使用同一世界中更高优先级的全局雾"):TEXT("全局雾已关闭");
        return;
    }
    if (!bTriedResources || bForce)
    {
        bTriedResources=true;
        UMaterialInterface* Fog=GlobalFogMaterial.LoadSynchronous();
        UMaterialInterface* Input=InputMaterial.LoadSynchronous();
        UMaterialInterface* Finish=bEnableFinish?FinishMaterial.LoadSynchronous():nullptr;
        if (YogGlobalFog::IsPostProcess(Fog) && (!DynamicMaterialInstance || DynamicMaterialInstance->Parent!=Fog))
        {
            FogPostProcess->Settings.WeightedBlendables.Array.Reset();
            DynamicMaterialInstance=UMaterialInstanceDynamic::Create(Fog,this);
            DynamicMaterialInstance->SetFlags(RF_Transient);
            FogPostProcess->AddOrUpdateBlendable(DynamicMaterialInstance,1.f);
        }
        if (Input && Input->GetMaterial() && Input->GetMaterial()->MaterialDomain==MD_Surface
            && (!InputMID || InputMID->Parent!=Input))
        {
            InputMID=UMaterialInstanceDynamic::Create(Input,this);InputMID->SetFlags(RF_Transient);bHasInputFrame=false;
        }
        if (YogGlobalFog::IsPostProcess(Finish) && (!FinishMID || FinishMID->Parent!=Finish))
        {
            FinishPostProcess->Settings.WeightedBlendables.Array.Reset();
            FinishMID=UMaterialInstanceDynamic::Create(Finish,this);
            FinishMID->SetFlags(RF_Transient);
            FinishPostProcess->AddOrUpdateBlendable(FinishMID,1.f);
        }
        // Invalid explicit assets must stop this component rather than leave
        // a stale previous material active after a Details edit.
        if (!YogGlobalFog::IsPostProcess(Fog)) DynamicMaterialInstance=nullptr;
        if (!Input || !Input->GetMaterial() || Input->GetMaterial()->MaterialDomain!=MD_Surface) InputMID=nullptr;
        if (!YogGlobalFog::IsPostProcess(Finish)) FinishMID=nullptr;
    }
    const int32 Resolution=FMath::Clamp(InputResolution,64,1024);
    if (!DensityInput || DensityInput->SizeX!=Resolution || DensityInput->SizeY!=Resolution)
    {
        DensityInput=NewObject<UTextureRenderTarget2D>(this,NAME_None,RF_Transient);
        DensityInput->ClearColor=FLinearColor::Black;DensityInput->Filter=TF_Bilinear;
        DensityInput->AddressX=TA_Wrap;DensityInput->AddressY=TA_Wrap;
        DensityInput->InitCustomFormat(Resolution,Resolution,PF_FloatRGBA,true);
        DensityInput->UpdateResourceImmediate(true);bHasInputFrame=false;
    }
    if (!DynamicMaterialInstance || !InputMID)
    {
        FogPostProcess->BlendWeight=0.f;FinishPostProcess->BlendWeight=0.f;
        GlobalStatus=TEXT("等待有效全局后处理雾材质和Ninja输入材质；构建素材后点刷新全局雾");
        return;
    }
    InputElapsed+=FMath::Max(DeltaSeconds,0.f);
    if (!bHasInputFrame || bForce || (bAnimateInput && InputElapsed>=1.f/30.f))
    {
        UKismetRenderingLibrary::DrawMaterialToRenderTarget(this,DensityInput,InputMID);
        bHasInputFrame=true;InputElapsed=0.f;
    }
    auto Scalar=[this](const TCHAR* Name,float Value,float Default,float Min,float Max)
    {DynamicMaterialInstance->SetScalarParameterValue(Name,YogGlobalFog::Safe(Value,Default,Min,Max));};
    DynamicMaterialInstance->SetTextureParameterValue(TEXT("GlobalInput"),DensityInput);
    DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlobalFogEnabled"),1.f);
    Scalar(TEXT("Density"),Density,5,0,30);
    // Ground remains an atmosphere input when environment interactions are
    // disabled (the base cache updater then returns before writing it).
    Scalar(TEXT("EnvGroundHeight"),ManualGroundHeight,0,-1000000,1000000);
    Scalar(TEXT("GlobalFogHeight"),FogHeight,450,50,50000);
    Scalar(TEXT("GlobalMaxDistance"),MaxDistance,10000,100,100000);
    Scalar(TEXT("GlobalStartDistance"),StartDistance,0,0,50000);
    Scalar(TEXT("GlobalNearFade"),NearFade,300,1,10000);
    Scalar(TEXT("HybridNoiseSize"),CloudSize,1000,50,20000);
    Scalar(TEXT("HybridAtmosphereVariation"),CloudVariation,.78f,0,1);
    Scalar(TEXT("HybridNoiseFlow"),FlowStrength,.45f,0,2);
    Scalar(TEXT("HybridNoiseSpeed"),FlowSpeed,.3f,0,2);
    Scalar(TEXT("HybridBoundaryDetail"),BoundaryDetail,.8f,0,1);
    Scalar(TEXT("MaxViewSteps"),ViewSteps,32,8,128);
    Scalar(TEXT("HybridShadowSamples"),ShadowSamples,6,1,20);
    Scalar(TEXT("FogHatchStrength"),FogBrushStrength,.3f,0,2);
    Scalar(TEXT("FogHatchDebugMode"),PreviewMode,0,0,3);
    Scalar(TEXT("LightIntensity"),VolumeLightIntensity,1.8f,0,20);
    Scalar(TEXT("Shadow"),ShadowStrength,1.8f,0,10);
    Scalar(TEXT("LightAttenuationRadius"),LightRadius,5500,1,100000);
    Scalar(TEXT("GlobalInputWorldSize"),InputWorldSize,4375,50,50000);
    DynamicMaterialInstance->SetScalarParameterValue(TEXT("EnvSpatialHeightEnabled"),bSpatialHeightEnabled?1.f:0.f);
    Scalar(TEXT("EnvNearFogHeight"),NearFogHeight,35,1,10000);
    Scalar(TEXT("EnvFarFogHeight"),FarFogHeight,350,1,10000);
    Scalar(TEXT("EnvHeightFeather"),HeightFeather,100,1,5000);
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("GlobalInputOrigin"),FLinearColor(InputOrigin.ContainsNaN()?FVector::ZeroVector:InputOrigin));
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("Color1"),Color1.GetClamped(0,100));
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("Color2"),Color2.GetClamped(0,100));
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("PointLightColor"),VolumeLightColor.GetClamped(0,100));
    const FVector Direction=VolumeLightDirection.ContainsNaN()?FVector(.25,-.35,.9).GetSafeNormal():VolumeLightDirection.GetSafeNormal(SMALL_NUMBER,FVector(.25,-.35,.9).GetSafeNormal());
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("GlobalLightDirection"),FLinearColor(Direction));
    const bool bPoint=IsValid(LightingActor) && LightingActor->GetWorld()==GetWorld() && !LightingActor->GetActorLocation().ContainsNaN();
    const FVector Light=bPoint?LightingActor->GetActorLocation():FVector::ZeroVector;
    DynamicMaterialInstance->SetVectorParameterValue(TEXT("GlobalLightPosition"),FLinearColor(Light.X,Light.Y,Light.Z,bPoint?1.f:-1.f));
    if (FinishMID)
    {
        FinishMID->SetScalarParameterValue(TEXT("OutlineStrength"),YogGlobalFog::Safe(OutlineStrength,.5,0,2));
        FinishMID->SetScalarParameterValue(TEXT("OuterBrushStrength"),YogGlobalFog::Safe(OuterBrushStrength,.75,0,2));
        FinishMID->SetScalarParameterValue(TEXT("OuterBrushDensity"),YogGlobalFog::Safe(OuterBrushDensity,.56f,0,1));
        FinishPostProcess->AddOrUpdateBlendable(FinishMID,1.f);
    }
    // Re-registering/undo may restore the Actor without its transient blend
    // list. Reassert only our own component entries, never another PP volume.
    FogPostProcess->AddOrUpdateBlendable(DynamicMaterialInstance,1.f);
    FogPostProcess->BlendWeight=1.f;
    FinishPostProcess->BlendWeight=bEnableFinish && FinishMID && PreviewMode==0?1.f:0.f;
    GlobalStatus=TEXT("全屏世界空间雾生效；无需Volume范围；交互缓存自动跟随观察者");
}

void AYogNinjaGlobalFogActor::RefreshGlobalFog()
{
    bTriedResources=false;
    UpdateGlobalFog(0.f,true);
    RefreshEnvironment();
}

void AYogNinjaGlobalFogActor::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();RegisterGlobalFog();
    UpdateGlobalFog(0.f,false);
}
void AYogNinjaGlobalFogActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);RefreshGlobalFog();
}
void AYogNinjaGlobalFogActor::BeginPlay()
{
    Super::BeginPlay();RefreshGlobalFog();
}
void AYogNinjaGlobalFogActor::Tick(float DeltaSeconds)
{
    UpdateGlobalFog(DeltaSeconds,false);
    Super::Tick(DeltaSeconds);
    BindingStatus=ResolveOutputMaterial()?TEXT("全局雾自动绑定，无需容器"):GlobalStatus;
}
#if WITH_EDITOR
void AYogNinjaGlobalFogActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);RefreshGlobalFog();
}
void AYogNinjaGlobalFogActor::PostEditUndo()
{
    Super::PostEditUndo();RefreshGlobalFog();
}
#endif
void AYogNinjaGlobalFogActor::ClearOwnedRendering()
{
    UnregisterGlobalFog();
    bTriedResources=false;
    if (FogPostProcess) {FogPostProcess->BlendWeight=0.f;FogPostProcess->Settings.WeightedBlendables.Array.Reset();}
    if (FinishPostProcess) {FinishPostProcess->BlendWeight=0.f;FinishPostProcess->Settings.WeightedBlendables.Array.Reset();}
}
void AYogNinjaGlobalFogActor::EndPlay(const EEndPlayReason::Type Reason)
{
    ClearOwnedRendering();Super::EndPlay(Reason);
}
void AYogNinjaGlobalFogActor::Destroyed()
{
    ClearOwnedRendering();Super::Destroyed();
}
void AYogNinjaGlobalFogActor::BeginDestroy()
{
    UnregisterGlobalFog();Super::BeginDestroy();
}
