#include "Tools/CharacterLookDev/DevKitLookDevLightingBuilder.h"

#include "Tools/CharacterLookDev/DevKitCharacterLookDevPresets.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/LocalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PointLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/RectLight.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/WorldSettings.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "YogLookDevLightingBuilder"

namespace
{
const FName RigTag(TEXT("Dev01.CharacterLookDev.Rig"));

template<typename ActorT>
ActorT* Spawn(UWorld* World, const TCHAR* Label, const FVector& Location = FVector::ZeroVector,
    const FRotator& Rotation = FRotator::ZeroRotator)
{
    FActorSpawnParameters Params;
    Params.ObjectFlags = RF_Transactional;
    Params.OverrideLevel = World->PersistentLevel;
    ActorT* Actor = World->SpawnActor<ActorT>(ActorT::StaticClass(), Location, Rotation, Params);
    if (Actor)
    {
        Actor->Tags.Add(RigTag);
        Actor->SetActorLabel(Label);
        for (UActorComponent* Component : Actor->GetComponents())
        {
            Component->ClearFlags(RF_Transient);
            Component->SetFlags(RF_Transactional);
        }
    }
    return Actor;
}

void ConfigureLight(ALight* Actor, float Intensity, FLinearColor Color, float AttenuationRadiusCm)
{
    Actor->SetMobility(EComponentMobility::Movable);
    ULightComponent* Light = Actor->GetLightComponent();
    if (ULocalLightComponent* Local = Cast<ULocalLightComponent>(Light))
    {
        Local->SetIntensityUnits(ELightUnits::Lumens);
        Local->SetAttenuationRadius(AttenuationRadiusCm);
        Local->InverseExposureBlend = 0.f;
    }
    if (UPointLightComponent* Point = Cast<UPointLightComponent>(Light)) Point->SetUseInverseSquaredFalloff(true);
    Light->SetIntensity(Intensity);
    Light->SetDiffuseScale(1.f);
    Light->SetSpecularScale(1.f);
    Light->SetIndirectLightingIntensity(1.f);
    Light->SetLightColor(Color, true);
    Light->MarkRenderStateDirty();
}
}

bool YogLookDev::BuildLightingLevel(UWorld* World, const FString& PresetId, FText& OutMessage)
{
    const FDevKitCharacterLookDevPreset* Preset = FDevKitCharacterLookDevPresets::Find(PresetId);
    if (!World || !World->PersistentLevel || !World->GetWorldSettings() || !Preset
        || World->WorldType == EWorldType::PIE
        || !World->GetOutermost()->GetName().StartsWith(TEXT("/YogLookDev/LightDev/")))
    {
        OutMessage = LOCTEXT("InvalidLightingLevel", "灯光构建需要 YogLookDev/LightDev 下的新关卡和有效灯光方案。");
        return false;
    }
    for (TActorIterator<AActor> It(World); It; ++It)
    {
        if (It->ActorHasTag(RigTag))
        {
            OutMessage = LOCTEXT("ExistingRig", "灯光关卡已包含灯光方案，保留当前可编辑内容。");
            return false;
        }
    }
    ALight* Key = nullptr;
    switch (Preset->KeyType)
    {
    case EDevKitLookDevKeyType::Point:
        Key = Spawn<APointLight>(World, TEXT("LookDev Candidate Key (lumens)"), Preset->KeyLocation, Preset->KeyRotation);
        break;
    case EDevKitLookDevKeyType::Rect:
        Key = Spawn<ARectLight>(World, TEXT("LookDev Candidate Key (lumens)"), Preset->KeyLocation, Preset->KeyRotation);
        break;
    default:
        Key = Spawn<ADirectionalLight>(World, TEXT("LookDev Candidate Key (lux)"), Preset->KeyLocation, Preset->KeyRotation);
        break;
    }
    ARectLight* Fill = Spawn<ARectLight>(World, TEXT("LookDev Candidate Broad Fill (lumens)"), Preset->FillLocation, Preset->FillRotation);
    APostProcessVolume* Post = Spawn<APostProcessVolume>(World, TEXT("LookDev Candidate Fixed Exposure"));
    if (!Key || !Fill || !Post)
    {
        TArray<AActor*> IncompleteActors;
        for (TActorIterator<AActor> It(World); It; ++It) if (It->ActorHasTag(RigTag)) IncompleteActors.Add(*It);
        for (AActor* Actor : IncompleteActors) World->DestroyActor(Actor);
        OutMessage = LOCTEXT("RigFailed", "创建灯光舞台失败；请查看编辑器日志。");
        return false;
    }
    Key->SetActorTransform(FTransform(Preset->KeyRotation, Preset->KeyLocation, FVector::OneVector));
    Fill->SetActorTransform(FTransform(Preset->FillRotation, Preset->FillLocation, FVector::OneVector));
    ConfigureLight(Key, Preset->KeyIntensity, Preset->KeyColor, Preset->AttenuationRadiusCm);
    ConfigureLight(Fill, Preset->FillLumens, Preset->FillColor, Preset->AttenuationRadiusCm);
    if (UPointLightComponent* Point = Cast<UPointLightComponent>(Key->GetLightComponent())) Point->SetSourceRadius(Preset->SourceRadiusCm);
    if (UDirectionalLightComponent* Directional = Cast<UDirectionalLightComponent>(Key->GetLightComponent())) Directional->SetLightSourceAngle(Preset->SourceAngleDegrees);
    if (URectLightComponent* Rect = Cast<URectLightComponent>(Key->GetLightComponent()))
    {
        Rect->SetSourceWidth(Preset->KeySourceSizeCm.X);
        Rect->SetSourceHeight(Preset->KeySourceSizeCm.Y);
    }
    Fill->RectLightComponent->SetSourceWidth(Preset->FillSourceSizeCm.X);
    Fill->RectLightComponent->SetSourceHeight(Preset->FillSourceSizeCm.Y);
    Post->bUnbound = true;
    Post->Priority = 100.f;
    FPostProcessSettings& Settings = Post->Settings;
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = AEM_Manual;
    Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Settings.AutoExposureApplyPhysicalCameraExposure = true;
    Settings.bOverride_CameraISO = true;
    Settings.CameraISO = 100.f;
    Settings.bOverride_DepthOfFieldFstop = true;
    Settings.DepthOfFieldFstop = 4.f;
    Settings.bOverride_CameraShutterSpeed = true;
    Settings.CameraShutterSpeed = FMath::Pow(2.f, Preset->EV100) / 16.f;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = 0.f;
    if (Preset->FogDensity > 0.f)
    {
        if (AExponentialHeightFog* Fog = Spawn<AExponentialHeightFog>(World, TEXT("LookDev Candidate Atmosphere")))
        {
            Fog->GetComponent()->SetFogDensity(Preset->FogDensity);
            Fog->GetComponent()->SetFogInscatteringColor(Preset->FogColor);
        }
    }
    World->GetWorldSettings()->Tags.Add(FName(*(FString(TEXT("Dev01.CharacterLookDev.Preset=")) + Preset->Id)));
    World->MarkPackageDirty();
    OutMessage = FText::FromString(Preset->Label);
    return true;
}

#undef LOCTEXT_NAMESPACE
