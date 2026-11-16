#include "YogNinjaFogActor.h"

#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

namespace YogNinjaFog
{
float Safe(float Value,float Fallback,float Min,float Max)
{
    return FMath::Clamp(FMath::IsFinite(Value)?Value:Fallback,Min,Max);
}
}

AYogNinjaFogActor::AYogNinjaFogActor()
{
    PrimaryActorTick.bCanEverTick=false;
    SceneRoot=CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);
    NinjaVolume=CreateDefaultSubobject<UNiagaraComponent>(TEXT("NinjaVolume"));
    NinjaVolume->SetupAttachment(SceneRoot);
    NinjaVolume->SetAutoActivate(false);
    NinjaVolume->SetCanEverAffectNavigation(false);
    NinjaVolume->bTickInEditor=true;
    NativeFog=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("NativeFog"));
    NativeFog->SetupAttachment(SceneRoot);
    NativeFog->ComponentTags.AddUnique(TEXT("YogNinjaFogCarrier"));
    Tags.AddUnique(TEXT("YogNinjaFVOL"));
    Tags.AddUnique(TEXT("YogNinjaFogCarrier"));
}

bool AYogNinjaFogActor::HasValidNinjaMaterial() const
{
    return IsValid(NinjaSystem)&&IsValid(FogMaterial)&&FogMaterial->GetMaterial()
        &&FogMaterial->GetMaterial()->MaterialDomain==MD_Volume;
}

void AYogNinjaFogActor::PostRegisterAllComponents()
{
    Super::PostRegisterAllComponents();
    if(!IsTemplate())RefreshNinjaFog();
}

void AYogNinjaFogActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshNinjaFog();
}

#if WITH_EDITOR
void AYogNinjaFogActor::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);
    RefreshNinjaFog();
}
#endif

void AYogNinjaFogActor::RefreshNinjaFog()
{
    if(IsTemplate())return;
    const bool bShow=bFogEnabled&&HasValidNinjaMaterial();
    if(NativeFog)
    {
        NativeFog->SetFogDensity(.00001f);
        NativeFog->SetVolumetricFog(true);
        NativeFog->SetVolumetricFogDistance(YogNinjaFog::Safe(NativeViewDistance,12000.f,100.f,50000.f));
        NativeFog->SetVolumetricFogStartDistance(YogNinjaFog::Safe(FogStartDistance,1200.f,0.f,50000.f));
        NativeFog->SetVolumetricFogNearFadeInDistance(YogNinjaFog::Safe(FadeDistance,700.f,0.f,50000.f));
        NativeFog->SetVolumetricFogScatteringDistribution(YogNinjaFog::Safe(ScatteringDistribution,.1f,-.9f,.9f));
        NativeFog->SetVisibility(bShow,false);
    }
    if(NinjaVolume)
    {
        if(bShow)
        {
            if(NinjaVolume->GetAsset()!=NinjaSystem)NinjaVolume->SetAsset(NinjaSystem);
            NinjaVolume->SetVariableMaterial(TEXT("User.FogMaterial"),FogMaterial);
            NinjaVolume->SetVariableMaterial(TEXT("User.VolumeMaterial"),FogMaterial);
            const FVector Extents=EmitterExtents.ContainsNaN()?FVector(6200,7200,2800):EmitterExtents.GetAbs().ComponentMax(FVector(1));
            NinjaVolume->SetVariableVec3(TEXT("User.EmitterExtents"),Extents);
            NinjaVolume->SetVariableVec3(TEXT("User.SimulationExtents"),FVector::ZeroVector);
            NinjaVolume->SetVisibility(true,true);
            if(!NinjaVolume->IsActive())NinjaVolume->Activate(false);
        }
        else
        {
            NinjaVolume->Deactivate();
            NinjaVolume->SetVisibility(false,true);
        }
    }
    if(bShow&&bRequestNativeFog)RequestNativeFog();
    else
    {
        ClearStartupRetry();
        ReleaseNativeFogLease();
    }
}

void AYogNinjaFogActor::RequestNativeFog()
{
    if(IsTemplate()||!GetWorld()||!bFogEnabled||!bRequestNativeFog||!HasValidNinjaMaterial())return;
    if(IConsoleVariable* Fog=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog")))
    {
        if(!bOwnFogLease)
        {
            PreviousFogValue=Fog->GetInt();
            PreviousFogPriority=Fog->GetFlags()&ECVF_SetByMask;
            bOwnFogLease=true;
        }
        Fog->Set(1,ECVF_SetByConsole);
    }
}

void AYogNinjaFogActor::ReleaseNativeFogLease()
{
    if(!bOwnFogLease)return;
    if(IConsoleVariable* Fog=IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog")))
    {
        if(Fog->GetInt()==1&&(Fog->GetFlags()&ECVF_SetByMask)==ECVF_SetByConsole)
        {
            Fog->Unset(ECVF_SetByConsole);
            Fog->Set(PreviousFogValue,static_cast<EConsoleVariableFlags>(PreviousFogPriority));
        }
    }
    bOwnFogLease=false;
}

void AYogNinjaFogActor::ClearStartupRetry()
{
    if(UWorld* World=GetWorld())World->GetTimerManager().ClearTimer(StartupFogRetry);
}

void AYogNinjaFogActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshNinjaFog();
    if(bFogEnabled&&bRequestNativeFog&&HasValidNinjaMaterial())
        StartupFogRetry=GetWorld()->GetTimerManager().SetTimerForNextTick(this,&AYogNinjaFogActor::RequestNativeFog);
}

void AYogNinjaFogActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearStartupRetry();
    if(NinjaVolume)NinjaVolume->Deactivate();
    ReleaseNativeFogLease();
    Super::EndPlay(EndPlayReason);
}

void AYogNinjaFogActor::Destroyed()
{
    ClearStartupRetry();
    ReleaseNativeFogLease();
    Super::Destroyed();
}

void AYogNinjaFogActor::BeginDestroy()
{
    ReleaseNativeFogLease();
    Super::BeginDestroy();
}
