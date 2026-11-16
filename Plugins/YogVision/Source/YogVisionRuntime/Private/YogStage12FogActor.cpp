#include "YogStage12FogActor.h"

#include "Components/BoxComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "Engine/Texture.h"
#include "Engine/VolumeTexture.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectIterator.h"

namespace
{
float SafeScalar(float Value, float Fallback, float Minimum, float Maximum)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? Value : Fallback, Minimum, Maximum);
}

FLinearColor SafeColour(const FLinearColor& Value, const FLinearColor& Fallback, float Maximum = 10000.f)
{
    return FLinearColor(SafeScalar(Value.R, Fallback.R, 0.f, Maximum),
        SafeScalar(Value.G, Fallback.G, 0.f, Maximum), SafeScalar(Value.B, Fallback.B, 0.f, Maximum), 1.f);
}

FLinearColor PackedVector(const FVector& V, float W = 0.f)
{
    return FLinearColor(static_cast<float>(V.X), static_cast<float>(V.Y), static_cast<float>(V.Z), W);
}

FVector SafeVector(const FVector& Value, const FVector& Fallback, float Minimum, float Maximum)
{
    return FVector(SafeScalar(static_cast<float>(Value.X), static_cast<float>(Fallback.X), Minimum, Maximum),
        SafeScalar(static_cast<float>(Value.Y), static_cast<float>(Fallback.Y), Minimum, Maximum),
        SafeScalar(static_cast<float>(Value.Z), static_cast<float>(Fallback.Z), Minimum, Maximum));
}

bool IsUsableLight(ULightComponent* Light, UWorld* World)
{
    if (!IsValid(Light) || Light->IsTemplate() || Light->GetWorld() != World || !Light->IsRegistered()
        || !Light->IsVisible() || !Light->bAffectsWorld || !IsValid(Light->GetOwner())
        || Light->GetOwner()->IsHidden() || !FMath::IsFinite(Light->ComputeLightBrightness())
        || Light->ComputeLightBrightness() <= 0.f || Light->GetComponentLocation().ContainsNaN()
        || Light->GetForwardVector().ContainsNaN()) return false;
#if WITH_EDITOR
    if (!World->IsGameWorld() && Light->GetOwner()->IsHiddenEd()) return false;
#endif
    return true;
}

FLinearColor LightTint(ULightComponent* Light)
{
    FLinearColor Colour = Light->GetLightColor();
    if (Light->bUseTemperature && FMath::IsFinite(Light->Temperature)) Colour *= Light->GetColorTemperature();
    return SafeColour(Colour, FLinearColor::Black, 10.f);
}
}

AYogStage12FogActor::AYogStage12FogActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    VolumeBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("VolumeBounds"));
    VolumeBounds->SetupAttachment(Root);
    VolumeBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    VolumeBounds->SetGenerateOverlapEvents(false);
    VolumeBounds->SetBoxExtent(BoundsExtent);
    VolumeBounds->SetHiddenInGame(true);
    VolumeBounds->ShapeColor = FColor(106, 186, 224);
    PostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcess"));
    PostProcess->SetupAttachment(Root);
    PostProcess->bUnbound = true;
    PostProcess->Priority = 100.f;
    PostProcess->BlendWeight = 1.f;
}

void AYogStage12FogActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RefreshMaterialParameters();
}

void AYogStage12FogActor::BeginPlay()
{
    Super::BeginPlay();
    RefreshMaterialParameters();
}

void AYogStage12FogActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetWorld() && !GetWorld()->bIsTearingDown)
    {
        UpdateFogAnimation(DeltaSeconds);
        RefreshMaterialParameters();
    }
}

#if WITH_EDITOR
void AYogStage12FogActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    RefreshMaterialParameters();
}
#endif

void AYogStage12FogActor::SetFogMaterial(UMaterialInterface* Material)
{
    if (FogMaterial != Material) FogMID = nullptr;
    FogMaterial = Material;
    bTriedDefaultFog = true;
    RefreshMaterialParameters();
}

void AYogStage12FogActor::SetFogEnabled(bool bEnabled)
{
    bFogEnabled = bEnabled;
    RefreshMaterialParameters();
}

void AYogStage12FogActor::SetObserverActor(AActor* Actor)
{
    ObserverActor = Actor;
    RefreshMaterialParameters();
}

void AYogStage12FogActor::SetFogTimeForCapture(float Seconds)
{
    FogTimeOverrideSeconds = SafeScalar(Seconds, 0.f, 0.f, 1000000000.f);
    bHasFogTimeOverride = true;
    RefreshMaterialParameters();
}

void AYogStage12FogActor::ClearFogTimeOverride()
{
    bHasFogTimeOverride = false;
    RefreshMaterialParameters();
}

void AYogStage12FogActor::ResetFogAnimation()
{
    AccumulatedFogTimeSeconds = 0.;
    bHasFogTimeOverride = false;
    FogTimeOverrideSeconds = 0.f;
    RefreshMaterialParameters();
}

void AYogStage12FogActor::UpdateFogAnimation(float DeltaSeconds)
{
    const UWorld* World = GetWorld();
    const bool bGameWorld = World && World->IsGameWorld();
    const float Speed = SafeScalar(AnimationSpeed, 1.f, 0.f, 20.f);
    const bool bCanAdvance = World && bAnimateFog && (bGameWorld || bAnimateInEditor)
        && !bHasFogTimeOverride && Speed > 0.f;
    if (bCanAdvance && FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0.f)
    {
        // Own elapsed time keeps edits, pauses and capture overrides from causing wall-clock jumps.
        AccumulatedFogTimeSeconds = FMath::Min(AccumulatedFogTimeSeconds
            + static_cast<double>(DeltaSeconds) * Speed, 1000000000.);
    }
    FogTimeSeconds = bHasFogTimeOverride ? FogTimeOverrideSeconds : static_cast<float>(AccumulatedFogTimeSeconds);
    if (bHasFogTimeOverride) AnimationStatus = TEXT("固定截图时间 / capture time override");
    else if (!World) AnimationStatus = TEXT("等待场景 / waiting for world");
    else if (!bAnimateFog) AnimationStatus = TEXT("已冻结 / paused: animation disabled");
    else if (!bGameWorld && !bAnimateInEditor) AnimationStatus = TEXT("编辑器已暂停 / editor paused");
    else if (Speed <= 0.f) AnimationStatus = TEXT("速度为0 / paused: zero speed");
    else AnimationStatus = bGameWorld ? TEXT("游戏运行 / runtime running")
        : TEXT("编辑器运行；需实时视口 / editor running; realtime viewport required");
}

void AYogStage12FogActor::EnsureMaterial()
{
    if (!IsValid(FogMaterial) && !bTriedDefaultFog)
    {
        bTriedDefaultFog = true;
        FogMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/YogVision/Stage12/M_Stage12SpatialFog.M_Stage12SpatialFog"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    }
    if (!IsValid(FogMaterial))
    {
        FogMID = nullptr;
        PostProcess->Settings.WeightedBlendables.Array.Reset();
        PostProcess->bEnabled = false;
        return;
    }
    if (!IsValid(FogMID) || FogMID->Parent != FogMaterial)
    {
        FogMID = UMaterialInstanceDynamic::Create(FogMaterial, this);
        bHadExternalFieldOverride = false;
        bHadDensityShapeOverride = false;
        PostProcess->Settings.WeightedBlendables.Array.Reset();
        if (FogMID) PostProcess->Settings.WeightedBlendables.Array.Emplace(1.f, FogMID.Get());
    }
    PostProcess->bEnabled = bFogEnabled && IsValid(FogMID);
}

void AYogStage12FogActor::ResolvePalette()
{
    EffectiveNearColour = FLinearColor(.112495f, .678705f, .744792f, 1.f);
    EffectiveFarColour = FLinearColor(.1106f, .1505f, .35f, 1.f);
    EffectivePaletteMix = .07f;
    if (!IsValid(PaletteMaterial) && !bTriedDefaultPalette)
    {
        bTriedDefaultPalette = true;
        PaletteMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/Map/MainMap/L1_CLS_Corridor_01/MI_CL_Corridor_Goth_PencilPostOutline.MI_CL_Corridor_Goth_PencilPostOutline"),
            nullptr, LOAD_NoWarn | LOAD_Quiet);
    }
    FLinearColor Near, Far;
    float Mix = .07f;
    const bool bNearValid = IsValid(PaletteMaterial)
        && PaletteMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 01")), Near);
    const bool bFarValid = IsValid(PaletteMaterial)
        && PaletteMaterial->GetVectorParameterValue(FMaterialParameterInfo(TEXT("Fog Color 02")), Far);
    const bool bMixValid = IsValid(PaletteMaterial)
        && PaletteMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("lerp_fog")), Mix);
    if (bNearValid) EffectiveNearColour = SafeColour(Near, EffectiveNearColour, 10.f);
    if (bFarValid) EffectiveFarColour = SafeColour(Far, EffectiveFarColour, 10.f);
    if (bMixValid) EffectivePaletteMix = SafeScalar(Mix, .07f, 0.f, 1.f);
    PaletteSource = FString::Printf(TEXT("%s | near:%s far:%s mix:%s | %s"),
        IsValid(PaletteMaterial) ? *PaletteMaterial->GetPathName() : TEXT("fallback"),
        bNearValid ? TEXT("MI") : TEXT("fallback"), bFarValid ? TEXT("MI") : TEXT("fallback"),
        bMixValid ? TEXT("MI") : TEXT("fallback"), bUseArtistPalette ? TEXT("enabled") : TEXT("disabled"));
}

void AYogStage12FogActor::ResolveDirectional(FVector& TowardLight, FLinearColor& Radiance)
{
    TowardLight = FRotator(SafeScalar(ManualDirectionalPitch, 35.f, -89.f, 89.f),
        SafeScalar(ManualDirectionalYaw, -45.f, -180.f, 180.f), 0.f).Vector();
    Radiance = SafeColour(DirectionalColour, FLinearColor(.08f, .1f, .14f));
    DirectionalSource = bUseSceneDirectionalLight ? TEXT("manual fallback: no active scene directional light") : TEXT("manual direction");
    UWorld* World = GetWorld();
    if (!World || !bUseSceneDirectionalLight) return;
    UDirectionalLightComponent* Selected = nullptr;
    int32 SelectedSunIndex = MAX_int32;
    double SelectedBrightness = -1.;
    FString SelectedPath;
    for (TObjectIterator<UDirectionalLightComponent> It; It; ++It)
    {
        UDirectionalLightComponent* Light = *It;
        if (!IsUsableLight(Light, World)) continue;
        const double Brightness = static_cast<double>(Light->ComputeLightBrightness()) * LightTint(Light).GetLuminance();
        if (!FMath::IsFinite(Brightness) || Brightness <= 0.) continue;
        const int32 SunIndex = Light->bAtmosphereSunLight ? FMath::Max(0, Light->AtmosphereSunLightIndex) : MAX_int32;
        const FString Path = Light->GetPathName();
        if (!Selected || SunIndex < SelectedSunIndex || (SunIndex == SelectedSunIndex
            && (Brightness > SelectedBrightness || (Brightness == SelectedBrightness && Path < SelectedPath))))
        {
            Selected = Light;
            SelectedSunIndex = SunIndex;
            SelectedBrightness = Brightness;
            SelectedPath = Path;
        }
    }
    if (Selected)
    {
        TowardLight = -Selected->GetForwardVector();
        Radiance = SafeColour(LightTint(Selected) * Selected->ComputeLightBrightness()
            * SafeScalar(Selected->VolumetricScatteringIntensity, 0.f, 0.f, 10000.f)
            * SafeScalar(SceneDirectionalLightScale, .01f, 0.f, 10.f), FLinearColor::Black);
        DirectionalSource = Selected->GetPathName();
    }
}

void AYogStage12FogActor::ResolveObserver()
{
    EffectiveGameplayObserver = FLinearColor::Transparent;
    ObserverSource = bUseExplicitPreviewFocus ? TEXT("editor: explicit preview focus") : TEXT("editor: per-view centre projected to world height");
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld()) return;
    AActor* Observer = ObserverActor.Get();
    if (Observer && Observer->GetWorld() != World) Observer = nullptr;
    if (!Observer)
    {
        for (auto It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && PC->IsLocalController() && PC->GetPawn())
            {
                Observer = PC->GetPawn();
                break;
            }
        }
    }
    if (IsValid(Observer) && !Observer->GetActorLocation().ContainsNaN())
    {
        EffectiveGameplayObserver = PackedVector(Observer->GetActorLocation(), 1.f);
        ObserverSource = FString::Printf(TEXT("gameplay: %s"), *Observer->GetPathName());
    }
    else ObserverSource = TEXT("gameplay: no observer, per-view centre fallback");
}

void AYogStage12FogActor::UpdateLocalLights()
{
    struct FCandidate { ULocalLightComponent* Light; double Score; FString Path; };
    TArray<FCandidate> Candidates;
    UWorld* World = GetWorld();
    if (World)
    {
        for (TObjectIterator<ULocalLightComponent> It; It; ++It)
        {
            ULocalLightComponent* Light = *It;
            // Rect-light shape integration is outside this candidate's point/spot contract.
            if (!Cast<UPointLightComponent>(Light) || !IsUsableLight(Light, World)
                || !FMath::IsFinite(Light->AttenuationRadius) || Light->AttenuationRadius <= 0.f
                || !FMath::IsFinite(Light->VolumetricScatteringIntensity) || Light->VolumetricScatteringIntensity <= 0.f) continue;
            const double Range = FMath::Max(1.f, Light->AttenuationRadius);
            const double Distance = FVector::Distance(GetActorLocation(), Light->GetComponentLocation());
            Candidates.Add({Light, Distance / Range, Light->GetPathName()});
        }
    }
    Candidates.Sort([](const FCandidate& A, const FCandidate& B)
    {
        return A.Score == B.Score ? A.Path < B.Path : A.Score < B.Score;
    });
    LocalLightCount = FMath::Min(4, Candidates.Num());
    const float LightScale = SafeScalar(LocalLightScale, .015f, 0.f, 10.f);
    for (int32 Index = 0; Index < 4; ++Index)
    {
        FLinearColor Position = FLinearColor::Transparent;
        FLinearColor Direction(0.f, 0.f, 0.f, -2.f);
        FLinearColor Colour = FLinearColor::Transparent;
        float InnerCos = -2.f;
        if (Index < LocalLightCount)
        {
            ULocalLightComponent* Light = Candidates[Index].Light;
            Position = PackedVector(Light->GetComponentLocation(), Light->AttenuationRadius);
            Direction = PackedVector(Light->GetForwardVector(), -2.f);
            if (USpotLightComponent* Spot = Cast<USpotLightComponent>(Light))
            {
                Direction.A = FMath::Cos(FMath::DegreesToRadians(SafeScalar(Spot->OuterConeAngle, 45.f, .1f, 89.f)));
                InnerCos = FMath::Cos(FMath::DegreesToRadians(SafeScalar(Spot->InnerConeAngle, 0.f, 0.f, 89.f)));
                InnerCos = FMath::Max(InnerCos, Direction.A + .0001f);
            }
            Colour = SafeColour(LightTint(Light) * Light->ComputeLightBrightness()
                * Light->VolumetricScatteringIntensity * LightScale, FLinearColor::Black);
            Colour.A = 1.f;
        }
        FogMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("LightPosition%d"), Index)), Position);
        FogMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("LightDirection%d"), Index)), Direction);
        FogMID->SetVectorParameterValue(FName(*FString::Printf(TEXT("LightColour%d"), Index)), Colour);
        FogMID->SetScalarParameterValue(FName(*FString::Printf(TEXT("LightInnerCos%d"), Index)), InnerCos);
    }
}

void AYogStage12FogActor::RefreshMaterialParameters()
{
    if (!PostProcess || IsTemplate()) return;
    UpdateFogAnimation(0.f);
    const FVector Extent(
        SafeScalar(static_cast<float>(FMath::Abs(BoundsExtent.X)), 1700.f, 1.f, 1000000.f),
        SafeScalar(static_cast<float>(FMath::Abs(BoundsExtent.Y)), 2000.f, 1.f, 1000000.f),
        SafeScalar(static_cast<float>(FMath::Abs(BoundsExtent.Z)), 1200.f, 1.f, 1000000.f));
    if (VolumeBounds) VolumeBounds->SetBoxExtent(Extent, false);
    EnsureMaterial();
    if (!FogMID) return;
    if ((bHadExternalFieldOverride && !IsValid(ExternalFieldTexture))
        || (bHadDensityShapeOverride && !IsValid(DensityShapeTexture)))
    {
        // Restore the parent texture default when the optional override is cleared.
        // All other actor-owned uniforms are repopulated below in this refresh.
        FogMID->ClearParameterValues();
        bHadExternalFieldOverride = false;
        bHadDensityShapeOverride = false;
    }
    ResolvePalette();
    ResolveObserver();
    FVector TowardLight;
    FLinearColor DirectionalRadiance;
    ResolveDirectional(TowardLight, DirectionalRadiance);
    const FQuat Rotation = GetActorQuat();
    FogMID->SetVectorParameterValue(TEXT("VolumeCenter"), PackedVector(GetActorLocation()));
    FogMID->SetVectorParameterValue(TEXT("VolumeExtent"), PackedVector(Extent * GetActorScale3D().GetAbs()));
    FogMID->SetVectorParameterValue(TEXT("VolumeAxisX"), PackedVector(Rotation.GetAxisX()));
    FogMID->SetVectorParameterValue(TEXT("VolumeAxisY"), PackedVector(Rotation.GetAxisY()));
    FogMID->SetVectorParameterValue(TEXT("VolumeAxisZ"), PackedVector(Rotation.GetAxisZ()));
    FogMID->SetVectorParameterValue(TEXT("FogAlbedo"), SafeColour(FogAlbedo, FLinearColor(.85f, .9f, .95f), 1.f));
    FogMID->SetVectorParameterValue(TEXT("AmbientRadiance"), SafeColour(AmbientRadiance, FLinearColor(.035f, .045f, .06f)));
    FogMID->SetVectorParameterValue(TEXT("NearColour"), EffectiveNearColour);
    FogMID->SetVectorParameterValue(TEXT("FarColour"), EffectiveFarColour);
    FogMID->SetVectorParameterValue(TEXT("DirectionalDirection"), PackedVector(TowardLight));
    FogMID->SetVectorParameterValue(TEXT("DirectionalColour"), DirectionalRadiance);
    FogMID->SetVectorParameterValue(TEXT("GameplayObserver"), EffectiveGameplayObserver);
    FogMID->SetVectorParameterValue(TEXT("ExplicitPreviewFocus"), PackedVector(PreviewFocusPosition,
        bUseExplicitPreviewFocus && !PreviewFocusPosition.ContainsNaN() ? 1.f : 0.f));
    FogMID->SetVectorParameterValue(TEXT("ShapeStretch"), PackedVector(SafeVector(ShapeStretch, FVector(1.f, 1.f, .7f), .05f, 10.f)));
    FogMID->SetVectorParameterValue(TEXT("FlowVelocity"), PackedVector(SafeVector(FlowVelocity, FVector(5.f, -2.f, 1.f), -10000.f, 10000.f)));
    auto Scalar = [this](const TCHAR* Name, float Value) { FogMID->SetScalarParameterValue(FName(Name), Value); };
    Scalar(TEXT("Density"), SafeScalar(Density, .12f, 0.f, 5.f));
    Scalar(TEXT("DensityVariation"), SafeScalar(DensityVariation, .3f, 0.f, 1.f));
    Scalar(TEXT("NoiseScale"), SafeScalar(NoiseScale, .002f, .00001f, .1f));
    Scalar(TEXT("EdgeWidth"), SafeScalar(EdgeWidth, 350.f, 1.f, 10000.f));
    Scalar(TEXT("UseDensityShape"), bUseDensityShape ? 1.f : 0.f);
    Scalar(TEXT("ShapeStrength"), SafeScalar(ShapeStrength, 1.f, 0.f, 1.f));
    Scalar(TEXT("ShapeScale"), SafeScalar(ShapeScale, .0005f, .00001f, .1f));
    Scalar(TEXT("DetailStrength"), SafeScalar(DetailStrength, .35f, 0.f, 1.f));
    Scalar(TEXT("DetailScale"), SafeScalar(DetailScale, .0025f, .00001f, .1f));
    Scalar(TEXT("ShapeEdgeSoftness"), SafeScalar(ShapeEdgeSoftness, .2f, .01f, 1.f));
    Scalar(TEXT("BodyCoreDensity"), SafeScalar(BodyCoreDensity, .15f, 0.f, 1.f));
    Scalar(TEXT("BodyRoundness"), SafeScalar(BodyRoundness, 1.f, 0.f, 1.f));
    Scalar(TEXT("FlowWarpStrength"), SafeScalar(FlowWarpStrength, 35.f, 0.f, 1000.f));
    Scalar(TEXT("FlowPeriod"), SafeScalar(FlowPeriod, 24.f, .1f, 600.f));
    Scalar(TEXT("FogTimeSeconds"), FogTimeSeconds);
    Scalar(TEXT("AmbientStrength"), SafeScalar(AmbientStrength, 1.f, 0.f, 10.f));
    Scalar(TEXT("DirectionalStrength"), SafeScalar(DirectionalStrength, 1.f, 0.f, 10.f));
    Scalar(TEXT("LocalLightScale"), SafeScalar(LocalLightScale, .015f, 0.f, 10.f));
    Scalar(TEXT("LightClearStrength"), SafeScalar(LightClearStrength, 0.f, 0.f, 1.f));
    Scalar(TEXT("StepCount"), FMath::Clamp(StepCount, 8, 128));
    Scalar(TEXT("ShadowSteps"), FMath::Clamp(ShadowSteps, 1, 32));
    Scalar(TEXT("MaxDistance"), SafeScalar(MaxDistance, 15000.f, 100.f, 100000.f));
    Scalar(TEXT("UseGeometryShadows"), bUseGeometryShadows ? 1.f : 0.f);
    Scalar(TEXT("GeometryShadowBias"), SafeScalar(GeometryShadowBias, 8.f, 0.f, 100.f));
    Scalar(TEXT("PaletteStrength"), bUseArtistPalette ? SafeScalar(PaletteStrength, .5f, 0.f, 1.f) : 0.f);
    Scalar(TEXT("PaletteDistance"), SafeScalar(PaletteDistance, 3000.f, 1.f, 50000.f));
    Scalar(TEXT("PaletteMix"), EffectivePaletteMix);
    Scalar(TEXT("VisionEnabled"), bEnableVision ? 1.f : 0.f);
    Scalar(TEXT("ObserverClearRadius"), SafeScalar(ObserverClearRadius, 500.f, 0.f, 20000.f));
    Scalar(TEXT("ObserverFeather"), SafeScalar(ObserverFeather, 650.f, 1.f, 20000.f));
    Scalar(TEXT("ClearDensityFraction"), SafeScalar(ClearDensityFraction, .1f, 0.f, 1.f));
    Scalar(TEXT("PreviewFocusHeight"), SafeScalar(PreviewFocusHeight, 150.f, -100000.f, 100000.f));
    Scalar(TEXT("PreviewFocusMaxDistance"), SafeScalar(PreviewFocusMaxDistance, 7000.f, 1.f, 50000.f));
    Scalar(TEXT("DebugMode"), FMath::Clamp(DebugMode, 0, 8));
    Scalar(TEXT("ExternalFieldStrength"), SafeScalar(ExternalFieldStrength, 0.f, 0.f, 10.f));
    if (IsValid(ExternalFieldTexture))
    {
        FogMID->SetTextureParameterValue(TEXT("FieldTexture"), ExternalFieldTexture);
        bHadExternalFieldOverride = true;
    }
    if (IsValid(DensityShapeTexture))
    {
        FogMID->SetTextureParameterValue(TEXT("DensityShapeTexture"), DensityShapeTexture);
        bHadDensityShapeOverride = true;
    }
    UpdateLocalLights();
}

FString AYogStage12FogActor::GetSourceStatus() const
{
    return FString::Printf(TEXT("material=%s\nobserver=%s\ndirectional=%s\nlocal point/spot=%d\npalette=%s\nshape=%s\nanimation=%s; time=%.3fs"),
        IsValid(FogMaterial) ? *FogMaterial->GetPathName() : TEXT("missing"), *ObserverSource,
        *DirectionalSource, LocalLightCount, *PaletteSource,
        IsValid(DensityShapeTexture) ? *DensityShapeTexture->GetPathName() : TEXT("material default volume texture"),
        *AnimationStatus, FogTimeSeconds);
}
