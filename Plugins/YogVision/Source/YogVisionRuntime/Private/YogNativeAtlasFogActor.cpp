#include "YogNativeAtlasFogActor.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

namespace NativeAtlasFog
{
float Safe(float Value, float Default, float Min, float Max)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? Value : Default, Min, Max);
}
FVector SafeExtent(const FVector& Value)
{
    return FVector(Safe(static_cast<float>(FMath::Abs(Value.X)), 2200.f, 1.f, 1000000.f),
        Safe(static_cast<float>(FMath::Abs(Value.Y)), 2400.f, 1.f, 1000000.f),
        Safe(static_cast<float>(FMath::Abs(Value.Z)), 1400.f, 1.f, 1000000.f));
}
FLinearColor Colour(const FLinearColor& Value, const FLinearColor& Default)
{
    return FLinearColor(Safe(Value.R, Default.R, 0.f, 10.f), Safe(Value.G, Default.G, 0.f, 10.f),
        Safe(Value.B, Default.B, 0.f, 10.f), 1.f);
}
FLinearColor Pack(const FVector& Value, float W = 0.f)
{
    return FLinearColor(static_cast<float>(Value.X), static_cast<float>(Value.Y), static_cast<float>(Value.Z), W);
}
FVector Direction(const FVector& Value)
{
    return Value.ContainsNaN() || Value.IsNearlyZero() ? FVector(.6, -.6, .6).GetSafeNormal() : Value.GetSafeNormal();
}
}

AYogNativeAtlasFogActor::AYogNativeAtlasFogActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    VolumeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VolumeMesh"));
    VolumeMesh->SetupAttachment(Root);
    VolumeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VolumeMesh->SetGenerateOverlapEvents(false);
    VolumeMesh->SetCanEverAffectNavigation(false);
    VolumeMesh->SetCastShadow(false);
    VolumeMesh->bAffectDistanceFieldLighting = false;
    VolumeMesh->bAffectDynamicIndirectLighting = false;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded()) VolumeMesh->SetStaticMesh(Cube.Object);
    VolumeMesh->SetRelativeScale3D(BoundsExtent / 50.);
    NativeFog = CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("NativeFog"));
    NativeFog->SetupAttachment(Root);
    NativeFog->FogDensity = .00001f;
    NativeFog->SecondFogData.FogDensity = 0.f;
    NativeFog->bEnableVolumetricFog = true;
    NativeFog->VolumetricFogDistance = ViewDistance;
    NativeFog->VolumetricFogScatteringDistribution = ScatteringDistribution;
    NativeFog->VolumetricFogAlbedo = FColor::White;
    NativeFog->VolumetricFogEmissive = FLinearColor::Black;
    NativeFog->bOverrideLightColorsWithFogInscatteringColors = false;
    NativeFog->bEnableFSSS = false;
}

void AYogNativeAtlasFogActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshMaterialParameters();
}

void AYogNativeAtlasFogActor::BeginPlay()
{
    Super::BeginPlay();
    // Never trust an editor MID or a serialized mesh override when entering the play world.
    EnsureMaterial(true);
    RefreshMaterialParameters();
}

void AYogNativeAtlasFogActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown) return;
    UpdateAnimation(DeltaSeconds);
    EnsureMaterial();
    if (!GetActorTransform().Equals(CachedActorTransform)) bStaticParametersDirty = true;
    if (!bPaletteCached || CachedPaletteMaterial.Get() != (IsValid(PaletteMaterial) ? PaletteMaterial.Get() : nullptr))
    {
        ResolvePalette(false);
        bStaticParametersDirty = true;
    }
    if (FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0.f) DirectionalCheckAccumulator += DeltaSeconds;
    if (DirectionalCheckAccumulator >= .5f)
    {
        DirectionalCheckAccumulator = 0.f;
        ResolveDirectional();
    }
    if (bStaticParametersDirty) PushStaticParameters();
    PushDynamicParameters();
}

#if WITH_EDITOR
void AYogNativeAtlasFogActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefreshMaterialParameters();
}
#endif

bool AYogNativeAtlasFogActor::EnsureMaterial(bool bForceNew)
{
    if (!VolumeMesh || IsTemplate()) return false;
    if (!IsValid(VolumeMaterial) && !bTriedDefaultMaterial)
    {
        bTriedDefaultMaterial = true;
        VolumeMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/YogVision/Stage12/Native/M_Stage12NativeAtlasFog.M_Stage12NativeAtlasFog"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    }
    if (!IsValid(VolumeMaterial) || !VolumeMaterial->GetMaterial() || VolumeMaterial->GetMaterial()->MaterialDomain != MD_Volume)
    {
        FogMID = nullptr;
        MaterialBindingStatus = TEXT("未绑定：需要有效Volume体积域材质 / missing volume material");
        VolumeMesh->SetVisibility(false, false);
        return false;
    }
    bool bBindingChanged = false;
    if (bForceNew || !IsValid(FogMID) || FogMID->Parent != VolumeMaterial || FogMID->GetOuter() != this)
    {
        FogMID = UMaterialInstanceDynamic::Create(VolumeMaterial, this);
        if (FogMID) FogMID->SetFlags(RF_Transient);
        bStaticParametersDirty = true;
        bBindingChanged = true;
    }
    if (FogMID && VolumeMesh->GetMaterial(0) != FogMID)
    {
        VolumeMesh->SetMaterial(0, FogMID);
        bStaticParametersDirty = true;
        bBindingChanged = true;
    }
    const bool bBound = IsFogMaterialBound();
    if (bBindingChanged || MaterialBindingStatus.IsEmpty() || !bBound)
        MaterialBindingStatus = bBound ? FString::Printf(TEXT("已绑定本Actor的MID / owned MID: %s"), *FogMID->GetPathName())
            : TEXT("绑定失败 / volume mesh material mismatch");
    if (VolumeMesh->IsVisible() != (bFogEnabled && bBound)) VolumeMesh->SetVisibility(bFogEnabled && bBound, false);
    return bBound;
}

bool AYogNativeAtlasFogActor::IsFogMaterialBound() const
{
    return IsValid(VolumeMesh) && IsValid(FogMID) && FogMID->GetOuter() == this
        && VolumeMesh->GetMaterial(0) == FogMID;
}

void AYogNativeAtlasFogActor::ApplyNativeFogSettings()
{
    if (!NativeFog) return;
    NativeFog->SetFogDensity(.00001f);
    NativeFog->SetVolumetricFog(true);
    NativeFog->SetVolumetricFogDistance(NativeAtlasFog::Safe(ViewDistance, 6000.f, 100.f, 50000.f));
    NativeFog->SetVolumetricFogScatteringDistribution(NativeAtlasFog::Safe(ScatteringDistribution, .2f, -.9f, .9f));
    NativeFog->SetVolumetricFogAlbedo(FColor::White);
    NativeFog->SetVisibility(bFogEnabled, false);
    if (NativeFog->bEnableFSSS)
    {
        NativeFog->bEnableFSSS = false;
        NativeFog->MarkRenderStateDirty();
    }
}

void AYogNativeAtlasFogActor::SetVolumeMaterial(UMaterialInterface* Material)
{
    VolumeMaterial = Material;
    bTriedDefaultMaterial = true;
    RefreshMaterialParameters();
}

void AYogNativeAtlasFogActor::SetFogEnabled(bool bEnabled)
{
    bFogEnabled = bEnabled;
    RefreshMaterialParameters();
}

void AYogNativeAtlasFogActor::SetObserverActor(AActor* Actor)
{
    ObserverActor = Actor;
    PushDynamicParameters();
}

void AYogNativeAtlasFogActor::RefreshMaterialParameters()
{
    if (IsTemplate() || !VolumeMesh) return;
    if (!VolumeMaterial) bTriedDefaultMaterial = false;
    EnsureMaterial();
    ApplyNativeFogSettings();
    ResolvePalette(true);
    ResolveDirectional();
    bStaticParametersDirty = true;
    PushStaticParameters();
    UpdateAnimation(0.f);
    PushDynamicParameters();
}

void AYogNativeAtlasFogActor::ResolvePalette(bool bForce)
{
    using namespace NativeAtlasFog;
    if (!IsValid(PaletteMaterial) && !bTriedDefaultPalette && bUseArtistPalette)
    {
        bTriedDefaultPalette = true;
        PaletteMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline"),
            nullptr, LOAD_NoWarn | LOAD_Quiet);
    }
    if (!bForce && bPaletteCached && CachedPaletteMaterial.Get() == PaletteMaterial) return;
    EffectiveNearColour = Colour(NearColour, FLinearColor(.112495f, .678705f, .744792f));
    EffectiveFarColour = Colour(FarColour, FLinearColor(.1106f, .1505f, .35f));
    EffectivePaletteMix = .07f;
    if (bUseArtistPalette && IsValid(PaletteMaterial))
    {
        FLinearColor Near, Far;
        float Mix;
        if (PaletteMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), Near)) EffectiveNearColour = Colour(Near, EffectiveNearColour);
        if (PaletteMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), Far)) EffectiveFarColour = Colour(Far, EffectiveFarColour);
        if (PaletteMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), Mix)) EffectivePaletteMix = Safe(Mix, .07f, 0.f, 1.f);
        PaletteSource = PaletteMaterial->GetPathName();
    }
    else PaletteSource = TEXT("手动近远色 / manual near-far colours");
    CachedPaletteMaterial = PaletteMaterial;
    bPaletteCached = true;
}

void AYogNativeAtlasFogActor::ResolveDirectional()
{
    UWorld* World = GetWorld();
    bool bFound = false;
    FVector NewDirection = NativeAtlasFog::Direction(FallbackLightDirection);
    FString Source = TEXT("无场景方向光：微弱材质自发光塑形 / manual emissive fallback");
    if (World && bUseSceneDirectionalLight)
    {
        UDirectionalLightComponent* Selected = nullptr;
        double Brightness = 0.;
        for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
        {
            UDirectionalLightComponent* Light = *It;
            if (Light->IsTemplate() || Light->GetWorld() != World || !Light->IsRegistered() || !Light->IsVisible()
                || !Light->bAffectsWorld || !IsValid(Light->GetOwner()) || Light->GetOwner()->IsHidden()
                || !FMath::IsFinite(Light->ComputeLightBrightness()) || Light->ComputeLightBrightness() <= Brightness) continue;
#if WITH_EDITOR
            if (!World->IsGameWorld() && Light->GetOwner()->IsHiddenEd()) continue;
#endif
            Selected = Light;
            Brightness = Light->ComputeLightBrightness();
        }
        if (Selected)
        {
            bFound = true;
            NewDirection = NativeAtlasFog::Direction(-Selected->GetForwardVector());
            Source = TEXT("原生雾读取真实方向光；人工自发光关闭 / native directional lighting: ") + Selected->GetPathName();
        }
    }
    if (bFound != bSceneDirectionalFound || !NewDirection.Equals(EffectiveFallbackDirection, .0001)) bStaticParametersDirty = true;
    bSceneDirectionalFound = bFound;
    EffectiveFallbackDirection = NewDirection;
    DirectionalSource = Source;
}

void AYogNativeAtlasFogActor::PushStaticParameters()
{
    using namespace NativeAtlasFog;
    if (!VolumeMesh) return;
    const FVector Extent = SafeExtent(BoundsExtent);
    if (!Extent.Equals(CachedBoundsExtent))
    {
        VolumeMesh->SetRelativeScale3D(Extent / 50.);
        CachedBoundsExtent = Extent;
    }
    CachedActorTransform = GetActorTransform();
    if (!IsFogMaterialBound()) return;
    if (!bTriedDefaultTextures)
    {
        bTriedDefaultTextures = true;
        if (!IsValid(DensityAtlas)) DensityAtlas = LoadObject<UTexture2D>(nullptr,
            TEXT("/YogVision/Stage12/Native/RadiSw7_density.RadiSw7_density"), nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (!IsValid(VelocityAtlas)) VelocityAtlas = LoadObject<UTexture2D>(nullptr,
            TEXT("/YogVision/Stage12/Native/RadiSw7_velocity.RadiSw7_velocity"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    }
    const FQuat Rotation = GetActorQuat();
    FogMID->SetVectorParameterValue(TEXT("VolumeCenter"), Pack(GetActorLocation()));
    FogMID->SetVectorParameterValue(TEXT("VolumeExtent"), Pack(Extent * GetActorScale3D().GetAbs()));
    FogMID->SetVectorParameterValue(TEXT("VolumeAxisX"), Pack(Rotation.GetAxisX()));
    FogMID->SetVectorParameterValue(TEXT("VolumeAxisY"), Pack(Rotation.GetAxisY()));
    FogMID->SetVectorParameterValue(TEXT("VolumeAxisZ"), Pack(Rotation.GetAxisZ()));
    FogMID->SetVectorParameterValue(TEXT("FogAlbedo"), Colour(FogAlbedo, FLinearColor::White));
    FogMID->SetVectorParameterValue(TEXT("NearColour"), EffectiveNearColour);
    FogMID->SetVectorParameterValue(TEXT("FarColour"), EffectiveFarColour);
    FogMID->SetVectorParameterValue(TEXT("FallbackLightDirection"), Pack(EffectiveFallbackDirection));
    FogMID->SetScalarParameterValue(TEXT("Density"), Safe(Density, .0012f, 0.f, .1f));
    FogMID->SetScalarParameterValue(TEXT("AtlasFPS"), Safe(AtlasFPS, 4.f, 0.f, 60.f));
    FogMID->SetScalarParameterValue(TEXT("FlowStrength"), Safe(FlowStrength, .05f, 0.f, 1.f));
    FogMID->SetScalarParameterValue(TEXT("ShapeScale"), Safe(ShapeScale, .0006f, .00001f, .1f));
    FogMID->SetScalarParameterValue(TEXT("DetailStrength"), Safe(DetailStrength, .5f, 0.f, 2.f));
    FogMID->SetScalarParameterValue(TEXT("ClearRadius"), Safe(ClearRadius, 500.f, 0.f, 20000.f));
    FogMID->SetScalarParameterValue(TEXT("ClearFeather"), Safe(ClearFeather, 650.f, 1.f, 20000.f));
    FogMID->SetScalarParameterValue(TEXT("ClearResidual"), Safe(ClearResidual, .15f, 0.f, 1.f));
    FogMID->SetScalarParameterValue(TEXT("PaletteDistance"), Safe(PaletteDistance, 3000.f, 1.f, 50000.f));
    FogMID->SetScalarParameterValue(TEXT("PaletteMix"), EffectivePaletteMix);
    FogMID->SetScalarParameterValue(TEXT("FallbackLightStrength"), bSceneDirectionalFound ? 0.f : Safe(FallbackLightStrength, .02f, 0.f, 1.f));
    const auto SetAtlas = [this](const TCHAR* Name, UTexture2D* Override)
    {
        UTexture* Value = Override;
        if (!IsValid(Value)) VolumeMaterial->GetTextureParameterValue(FMaterialParameterInfo(Name), Value);
        if (IsValid(Value)) FogMID->SetTextureParameterValue(Name, Value);
    };
    SetAtlas(TEXT("DensityAtlas"), DensityAtlas);
    SetAtlas(TEXT("VelocityAtlas"), VelocityAtlas);
    ++StaticParameterUploadCount;
    bStaticParametersDirty = false;
}

void AYogNativeAtlasFogActor::ResolveObserver()
{
    using namespace NativeAtlasFog;
    EffectiveObserverPosition = FLinearColor::Transparent;
    ObserverSource = TEXT("清雾关闭 / disabled");
    UWorld* World = GetWorld();
    if (!bEnableVision || !World) return;
    if (World->IsGameWorld())
    {
        AActor* Observer = ObserverActor.Get();
        if (!IsValid(Observer) || Observer->GetWorld() != World) Observer = nullptr;
        if (!Observer)
        {
            for (auto It = World->GetPlayerControllerIterator(); It; ++It)
            {
                APlayerController* PC = It->Get();
                if (PC && PC->IsLocalController() && PC->GetPawn()) { Observer = PC->GetPawn(); break; }
            }
        }
        if (IsValid(Observer) && !Observer->GetActorLocation().ContainsNaN())
        {
            EffectiveObserverPosition = Pack(Observer->GetActorLocation(), 1.f);
            ObserverSource = TEXT("游戏观察者 / gameplay: ") + Observer->GetPathName();
            return;
        }
    }
    else if (bUseExplicitPreviewFocus && !PreviewFocusPosition.ContainsNaN())
    {
        EffectiveObserverPosition = Pack(PreviewFocusPosition, 1.f);
        ObserverSource = TEXT("编辑器固定中心 / explicit editor focus");
        return;
    }
    FVector Camera, Forward;
    bool bHasCamera = false;
    if (World->IsGameWorld())
    {
        for (auto It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (!PC || !PC->IsLocalController()) continue;
            FRotator Rotation;
            PC->GetPlayerViewPoint(Camera, Rotation);
            Forward = Rotation.Vector();
            bHasCamera = true;
            break;
        }
    }
    if (!bHasCamera && !World->CachedViewInfoRenderedLastFrame.IsEmpty())
    {
        const FMatrix& ViewToWorld = World->CachedViewInfoRenderedLastFrame[0].ViewToWorld;
        Camera = ViewToWorld.GetOrigin();
        Forward = ViewToWorld.GetUnitAxis(EAxis::Z);
        bHasCamera = true;
    }
    if (bHasCamera && !Camera.ContainsNaN() && !Forward.ContainsNaN() && !Forward.IsNearlyZero())
    {
        const float Maximum = Safe(PreviewFocusMaxDistance, 7000.f, 100.f, 50000.f);
        const double Height = Safe(PreviewFocusHeight, 150.f, -100000.f, 100000.f);
        const double Distance = FMath::Abs(Forward.Z) > .01 ? (Height - Camera.Z) / Forward.Z : Maximum;
        EffectiveObserverPosition = Pack(Camera + Forward * FMath::Clamp(Distance, 100., static_cast<double>(Maximum)), 1.f);
        ObserverSource = TEXT("镜头中心投射 / camera centre focus");
    }
    else ObserverSource = TEXT("等待首个有效镜头；清雾暂未启用 / waiting for camera");
}

void AYogNativeAtlasFogActor::PushDynamicParameters()
{
    ResolveObserver();
    if (!IsFogMaterialBound()) return;
    FogMID->SetScalarParameterValue(TEXT("FogTimeSeconds"), FogTimeSeconds);
    FogMID->SetVectorParameterValue(TEXT("ObserverPosition"), EffectiveObserverPosition);
}

void AYogNativeAtlasFogActor::UpdateAnimation(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    const bool bGameWorld = World && World->IsGameWorld();
    const float Speed = NativeAtlasFog::Safe(AnimationSpeed, 1.f, 0.f, 20.f);
    if (World && bAnimateFog && (bGameWorld || bAnimateInEditor) && !bHasTimeOverride
        && FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0.f)
        AccumulatedFogSeconds = FMath::Min(AccumulatedFogSeconds + static_cast<double>(DeltaSeconds) * Speed, 1000000000.);
    FogTimeSeconds = bHasTimeOverride ? OverrideFogSeconds : static_cast<float>(AccumulatedFogSeconds);
    if (bHasTimeOverride) AnimationStatus = TEXT("固定截图时间 / capture override");
    else if (!bAnimateFog || Speed == 0.f) AnimationStatus = TEXT("动画已暂停 / paused");
    else if (!bGameWorld && !bAnimateInEditor) AnimationStatus = TEXT("编辑器已暂停 / editor paused");
    else AnimationStatus = bGameWorld ? TEXT("原生雾图集运行 / runtime atlas running") : TEXT("编辑器图集运行；需实时视口 / editor atlas running");
}

void AYogNativeAtlasFogActor::SetFogTimeForCapture(float Seconds)
{
    OverrideFogSeconds = NativeAtlasFog::Safe(Seconds, 0.f, 0.f, 1000000000.f);
    bHasTimeOverride = true;
    UpdateAnimation(0.f);
    PushDynamicParameters();
}

void AYogNativeAtlasFogActor::ClearFogTimeOverride()
{
    bHasTimeOverride = false;
    UpdateAnimation(0.f);
    PushDynamicParameters();
}

void AYogNativeAtlasFogActor::ResetFogAnimation()
{
    AccumulatedFogSeconds = 0.;
    bHasTimeOverride = false;
    OverrideFogSeconds = 0.f;
    UpdateAnimation(0.f);
    PushDynamicParameters();
}

FString AYogNativeAtlasFogActor::GetSourceStatus() const
{
    const IConsoleVariable* FogSwitch = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"));
    const int32 FogSwitchValue = FogSwitch ? FogSwitch->GetInt() : -1;
    return FString::Printf(TEXT("material=%s\nmesh-bound=%s\nmesh-material=%s\nnative-component=%s; r.VolumetricFog=%d; FSSS=%s\nclock=%.3fs; %s\nobserver=%s\ndirectional=%s\npalette=%s\nstatic-uniform-uploads=%d"),
        IsValid(VolumeMaterial) ? *VolumeMaterial->GetPathName() : TEXT("missing"), IsFogMaterialBound() ? TEXT("true") : TEXT("false"),
        VolumeMesh && VolumeMesh->GetMaterial(0) ? *VolumeMesh->GetMaterial(0)->GetPathName() : TEXT("missing"),
        NativeFog && NativeFog->bEnableVolumetricFog && NativeFog->IsVisible() ? TEXT("enabled") : TEXT("disabled"), FogSwitchValue,
        NativeFog && NativeFog->bEnableFSSS ? TEXT("on") : TEXT("off"), FogTimeSeconds, *AnimationStatus,
        *ObserverSource, *DirectionalSource, *PaletteSource, StaticParameterUploadCount);
}
