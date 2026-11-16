#include "YogComputeFogActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UnrealType.h"

namespace
{
constexpr int32 MaxInteractionBodies = 8;

float FiniteScalar(double Value, float Fallback, float Minimum, float Maximum)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? static_cast<float>(Value) : Fallback, Minimum, Maximum);
}

FVector FiniteVector(const FVector& Value, const FVector& Fallback, float Minimum, float Maximum)
{
    return FVector(FiniteScalar(Value.X, static_cast<float>(Fallback.X), Minimum, Maximum),
        FiniteScalar(Value.Y, static_cast<float>(Fallback.Y), Minimum, Maximum),
        FiniteScalar(Value.Z, static_cast<float>(Fallback.Z), Minimum, Maximum));
}

FLinearColor Packed(const FVector& V)
{
    return FLinearColor(static_cast<float>(V.X), static_cast<float>(V.Y), static_cast<float>(V.Z), 0.f);
}

bool IsFieldReady(const UTextureRenderTargetVolume* Field)
{
    return IsValid(Field) && Field->SizeX > 0 && Field->SizeY > 0 && Field->SizeZ > 0 && Field->GetResource();
}
}

AYogComputeFogActor::AYogComputeFogActor()
{
    Simulation = CreateDefaultSubobject<UYogFogSimulationComponent>(TEXT("Simulation"));
    Simulation->GridResolution = FIntVector(96, 96, 64);
    Simulation->AddTickPrerequisiteActor(this);
    bEnableVision = false;
    Density = .0025f;
    FallbackLightStrength = .012f;
    FrameBodies.Reserve(MaxInteractionBodies);
}

void AYogComputeFogActor::OnConstruction(const FTransform& Transform)
{
    TryResolveDefaultMaterial(0.f, true);
    Super::OnConstruction(Transform);
    if (Simulation) Simulation->AddTickPrerequisiteActor(this);
    UpdateSimulationInputs(0.f);
    BindSimulationFields();
}

void AYogComputeFogActor::BeginPlay()
{
    TryResolveDefaultMaterial(0.f, true);
    Super::BeginPlay();
    DemoClockSeconds = 0.;
    DemoTimeSeconds = 0.f;
    PreviousBodyPositions.Reset();
    if (Simulation) Simulation->AddTickPrerequisiteActor(this);
    UpdateSimulationInputs(0.f);
    // The parent creates a fresh owned MID for PIE; never retain its editor binding.
    BindSimulationFields();
}

void AYogComputeFogActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GetWorld() || GetWorld()->bIsTearingDown) return;
    TryResolveDefaultMaterial(DeltaSeconds, false);
    UpdateSimulationInputs(DeltaSeconds);
    // The component consumes these inputs after this Actor. Newly created RTs bind next tick.
    BindSimulationFields();
}

#if WITH_EDITOR
bool AYogComputeFogActor::CanEditChange(const FProperty* InProperty) const
{
    if (InProperty)
    {
        const FName PropertyName = InProperty->GetFName();
        if (PropertyName == GET_MEMBER_NAME_CHECKED(AYogNativeAtlasFogActor, ShapeScale)
            || PropertyName == GET_MEMBER_NAME_CHECKED(AYogNativeAtlasFogActor, DetailStrength)) return false;
    }
    return Super::CanEditChange(InProperty);
}

void AYogComputeFogActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    if (IsTemplate()) return;
    TryResolveDefaultMaterial(0.f, true);
    UpdateSimulationInputs(0.f);
    BindSimulationFields();
}
#endif

void AYogComputeFogActor::TryResolveDefaultMaterial(float DeltaSeconds, bool bForceAttempt)
{
    if (IsTemplate() || !bLoadDefaultComputeMaterial) return;
    // A missing compute asset must not silently settle on the parent's atlas fallback.
    const bool bParentFallback = IsValid(VolumeMaterial) && VolumeMaterial->GetPathName()
        == TEXT("/YogVision/Stage12/Native/M_Stage12NativeAtlasFog.M_Stage12NativeAtlasFog");
    if (IsValid(VolumeMaterial) && !bParentFallback) return;
    DefaultMaterialRetrySeconds += FiniteScalar(DeltaSeconds, 0.f, 0.f, 1.f);
    if (!bForceAttempt && DefaultMaterialRetrySeconds < .5f) return;
    DefaultMaterialRetrySeconds = 0.f;
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/YogVision/Stage12/Compute/M_ComputeVolumeFog.M_ComputeVolumeFog"), nullptr, LOAD_NoWarn | LOAD_Quiet))
    {
        SetVolumeMaterial(Material);
    }
}

void AYogComputeFogActor::UpdateSimulationInputs(float DeltaSeconds)
{
    if (!Simulation || IsTemplate()) return;
    const FVector ActorScale = FiniteVector(GetActorScale3D().GetAbs(), FVector::OneVector, .0001f, 1000.f);
    EffectiveSimulationCenter = FiniteVector(GetActorLocation(), FVector::ZeroVector, -1000000.f, 1000000.f)
        + FiniteVector(SimulationCenterOffset, FVector::ZeroVector, -1000000.f, 1000000.f);
    EffectiveSimulationExtent = FiniteVector(SimulationHalfExtent.GetAbs(), FVector(900.f, 900.f, 600.f), 1.f, 1000000.f) * ActorScale;
    EffectiveShapeScaleXYZ = FiniteVector(Simulation->ShapeScaleXYZ, FVector::OneVector, .05f, 20.f);
    Simulation->SetDomain(EffectiveSimulationCenter, EffectiveSimulationExtent);
    // Use the component's sanitized domain and metric for bodies AND material sampling.
    EffectiveSimulationCenter = Simulation->ActualDomainCenter;
    EffectiveSimulationExtent = Simulation->ActualDomainExtent;
    const FVector Cell = Simulation->CellSize;
    const float MinimumRadius = static_cast<float>(Cell.GetMax()) * FiniteScalar(MinimumKernelCells, 1.5f, .5f, 5.f);
    FrameBodies.Reset(MaxInteractionBodies);
    TArray<AActor*, TInlineAllocator<MaxInteractionBodies>> SelectedActors;

    const auto OverlapsDomain = [this](const FYogFogSimBody& Body)
    {
        const FVector D = (Body.Center - EffectiveSimulationCenter).GetAbs();
        return D.X <= EffectiveSimulationExtent.X + Body.Radius
            && D.Y <= EffectiveSimulationExtent.Y + Body.Radius
            && D.Z <= EffectiveSimulationExtent.Z + Body.HalfHeight;
    };
    const auto AppendActor = [this, DeltaSeconds, MinimumRadius, &SelectedActors, &OverlapsDomain](AActor* Source)
    {
        if (FrameBodies.Num() >= MaxInteractionBodies || !IsValid(Source) || Source == this
            || Source->IsTemplate() || Source->GetWorld() != GetWorld() || SelectedActors.Contains(Source)) return;
        FYogFogSimBody Body;
        Body.Center = Source->GetActorLocation();
        Body.Radius = FiniteScalar(DefaultBodyRadius, 45.f, 1.f, 10000.f);
        Body.HalfHeight = FiniteScalar(DefaultBodyHalfHeight, 90.f, 1.f, 10000.f);
        if (bUseSourceCapsuleSize)
        {
            if (const UCapsuleComponent* Capsule = Source->FindComponentByClass<UCapsuleComponent>())
            {
                Body.Center = Capsule->GetComponentLocation();
                Body.Radius = FiniteScalar(Capsule->GetScaledCapsuleRadius(), Body.Radius, 1.f, 10000.f);
                Body.HalfHeight = FiniteScalar(Capsule->GetScaledCapsuleHalfHeight(), Body.HalfHeight, 1.f, 10000.f);
            }
        }
        if (Body.Center.ContainsNaN()) return;
        Body.Radius = FMath::Max(Body.Radius, MinimumRadius);
        Body.HalfHeight = FMath::Max(Body.HalfHeight, Body.Radius);
        if (!OverlapsDomain(Body)) return;
        Body.Velocity = Source->GetVelocity();
        // Plain editor Actors and transform-driven sources may not publish component velocity.
        const TWeakObjectPtr<AActor> Key(Source);
        if (Body.Velocity.IsNearlyZero() && FMath::IsFinite(DeltaSeconds) && DeltaSeconds > .0001f)
        {
            if (const FVector* Previous = PreviousBodyPositions.Find(Key)) Body.Velocity = (Body.Center - *Previous) / DeltaSeconds;
        }
        Body.Velocity = FiniteVector(Body.Velocity, FVector::ZeroVector, -10000.f, 10000.f)
            * FiniteScalar(SourceVelocityStrength, 1.f, 0.f, 20.f);
        Body.Velocity = Body.Velocity.GetClampedToMaxSize(10000.);
        PreviousBodyPositions.Add(Key, Body.Center);
        SelectedActors.Add(Source);
        FrameBodies.Add(Body);
    };

    UWorld* World = GetWorld();
    if (bUsePlayerBody && World && World->IsGameWorld())
    {
        for (auto It = World->GetPlayerControllerIterator(); It; ++It)
        {
            APlayerController* PC = It->Get();
            if (PC && PC->IsLocalController() && PC->GetPawn()) { AppendActor(PC->GetPawn()); break; }
        }
    }
    for (AActor* Source : Bodies)
    {
        if (FrameBodies.Num() >= MaxInteractionBodies) break;
        AppendActor(Source);
    }
    for (auto It = PreviousBodyPositions.CreateIterator(); It; ++It)
    {
        if (!SelectedActors.Contains(It.Key().Get())) It.RemoveCurrent();
    }

    const bool bSimulationCanAdvance = Simulation->bSimulationEnabled && Simulation->IsComponentTickEnabled()
        && World && (World->IsGameWorld() || Simulation->bSimulateInEditor);
    const bool bAdvanceDemo = bEnableDemoBody && bAnimateDemo && World
        && (!bPauseDemoWithSimulation || bSimulationCanAdvance);
    if (bAdvanceDemo) DemoClockSeconds = FMath::Min(DemoClockSeconds + FiniteScalar(DeltaSeconds, 0.f, 0.f, .25f), 1000000000.);
    DemoTimeSeconds = static_cast<float>(DemoClockSeconds);
    const double Period = FiniteScalar(DemoPeriod, 8.f, .1f, 600.f);
    const double Turns = DemoClockSeconds / Period + FiniteScalar(DemoPhaseOffset, 0.f, 0.f, 1.f);
    DemoNormalizedPhase = static_cast<float>(Turns - FMath::FloorToDouble(Turns));
    const double Angle = DemoNormalizedPhase * 2. * UE_PI;
    const double AmplitudeX = FiniteScalar(DemoOrbitAmplitude.X, 350.f, 0.f, 100000.f);
    const double AmplitudeY = FiniteScalar(DemoOrbitAmplitude.Y, 220.f, 0.f, 100000.f);
    DemoBodyWorldPosition = EffectiveSimulationCenter + FiniteVector(DemoCenterOffset, FVector::ZeroVector, -100000.f, 100000.f)
        + FVector(AmplitudeX * FMath::Cos(Angle), AmplitudeY * FMath::Sin(Angle), 0.);
    DemoBodyWorldVelocity = bAdvanceDemo ? FVector(-AmplitudeX * FMath::Sin(Angle), AmplitudeY * FMath::Cos(Angle), 0.)
        * (2. * UE_PI / Period) * FiniteScalar(DemoVelocityStrength, 1.f, 0.f, 20.f) : FVector::ZeroVector;
    DemoBodyWorldVelocity = DemoBodyWorldVelocity.GetClampedToMaxSize(10000.);
    bDemoBodySubmitted = false;
    if (bEnableDemoBody && FrameBodies.Num() < MaxInteractionBodies)
    {
        FYogFogSimBody Demo;
        Demo.Center = DemoBodyWorldPosition;
        Demo.Velocity = DemoBodyWorldVelocity;
        Demo.Radius = FMath::Max(MinimumRadius, FiniteScalar(DemoRadius, 65.f, 1.f, 10000.f));
        Demo.HalfHeight = FMath::Max(Demo.Radius, FiniteScalar(DemoHalfHeight, 140.f, 1.f, 10000.f));
        if (OverlapsDomain(Demo)) { FrameBodies.Add(Demo); bDemoBodySubmitted = true; }
    }
    ActiveBodyCount = FrameBodies.Num();
    Simulation->SetBodies(FrameBodies);
}

void AYogComputeFogActor::BindSimulationFields()
{
    if (IsTemplate()) return;
    DensityFieldPreview = Simulation ? Simulation->DensityField.Get() : nullptr;
    VelocityFieldPreview = Simulation ? Simulation->VelocityField.Get() : nullptr;
    UMaterialInstanceDynamic* MID = GetFogMID();
    const bool bNewMID = CachedFieldMID.Get() != MID;
    const bool bPreviouslyBound = bDensityFieldBound;
    if (!IsValid(MID))
    {
        if (VolumeMesh && VolumeMesh->IsVisible()) VolumeMesh->SetVisibility(false, false);
        if (NativeFog && NativeFog->IsVisible()) NativeFog->SetVisibility(false, false);
        CachedFieldMID.Reset();
        CachedDensityField.Reset();
        bDensityFieldBound = false;
        bMaterialAcceptsDensityField = false;
        bCachedSimulationReady = false;
        FieldBindingStatus = TEXT("等待计算雾Volume材质；尚未绑定三维密度场");
        return;
    }
    if (bNewMID)
    {
        CachedFieldMID = MID;
        CachedDensityField.Reset();
        UTexture* ParentField = nullptr;
        bMaterialAcceptsDensityField = IsValid(MID->Parent)
            && MID->Parent->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityField")), ParentField);
    }
    // Reject the inherited atlas fallback rather than presenting it as a compute-field result.
    const bool bRenderVolume = bFogEnabled && bMaterialAcceptsDensityField;
    if (VolumeMesh && VolumeMesh->IsVisible() != bRenderVolume) VolumeMesh->SetVisibility(bRenderVolume, false);
    if (NativeFog && NativeFog->IsVisible() != bRenderVolume) NativeFog->SetVisibility(bRenderVolume, false);

    UTextureRenderTargetVolume* DensityRT = DensityFieldPreview.Get();
    const bool bReady = bMaterialAcceptsDensityField && IsFieldReady(DensityRT);
    const bool bNewDensity = CachedDensityField.Get() != DensityRT;
    UTexture* ActualField = nullptr;
    bool bActualFieldMatches = bReady && MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityField")), ActualField)
        && ActualField == DensityRT;
    if (bReady && (bNewMID || bNewDensity || !bCachedSimulationReady || !bActualFieldMatches))
    {
        MID->SetTextureParameterValue(TEXT("DensityField"), DensityRT);
        CachedDensityField = DensityRT;
        bActualFieldMatches = MID->GetTextureParameterValue(FMaterialParameterInfo(TEXT("DensityField")), ActualField)
            && ActualField == DensityRT;
    }
    if (bNewMID || bReady != bCachedSimulationReady) MID->SetScalarParameterValue(TEXT("SimulationReady"), bReady ? 1.f : 0.f);

    FIntVector Grid = Simulation ? Simulation->GridResolution : FIntVector(8, 8, 8);
    Grid = FIntVector(FMath::Clamp(Grid.X, 8, 128), FMath::Clamp(Grid.Y, 8, 128), FMath::Clamp(Grid.Z, 8, 128));
    if (bReady) Grid = FIntVector(DensityRT->SizeX, DensityRT->SizeY, DensityRT->SizeZ);
    const bool bDomainChanged = bNewMID || !CachedSimulationCenter.Equals(EffectiveSimulationCenter)
        || !CachedSimulationExtent.Equals(EffectiveSimulationExtent) || !CachedShapeScaleXYZ.Equals(EffectiveShapeScaleXYZ)
        || CachedGridResolution != Grid;
    if (bDomainChanged)
    {
        MID->SetVectorParameterValue(TEXT("SimulationCenter"), Packed(EffectiveSimulationCenter));
        MID->SetVectorParameterValue(TEXT("SimulationExtent"), Packed(EffectiveSimulationExtent));
        MID->SetVectorParameterValue(TEXT("ShapeScaleXYZ"), Packed(EffectiveShapeScaleXYZ));
        MID->SetVectorParameterValue(TEXT("GridResolution"), FLinearColor(Grid.X, Grid.Y, Grid.Z, 0.f));
        CachedSimulationCenter = EffectiveSimulationCenter;
        CachedSimulationExtent = EffectiveSimulationExtent;
        CachedShapeScaleXYZ = EffectiveShapeScaleXYZ;
        CachedGridResolution = Grid;
    }
    bDensityFieldBound = bActualFieldMatches;
    if (bNewMID || bNewDensity || bReady != bCachedSimulationReady || bDomainChanged || bPreviouslyBound != bDensityFieldBound || FieldBindingStatus.IsEmpty())
    {
        FieldBindingStatus = !bMaterialAcceptsDensityField ? TEXT("当前材质未声明DensityField；计算雾载体已隐藏，等待正确材质")
            : FString::Printf(TEXT("DensityField=%s；实际绑定=%s；SimulationReady=%d；网格=%dx%dx%d；模拟域为世界轴AABB"),
                IsValid(DensityRT) ? *DensityRT->GetName() : TEXT("等待GPU资源"), bDensityFieldBound ? TEXT("是") : TEXT("否"),
                bReady ? 1 : 0, Grid.X, Grid.Y, Grid.Z);
    }
    bCachedSimulationReady = bReady;
}

void AYogComputeFogActor::RefreshComputeFog()
{
    if (IsTemplate()) return;
    TryResolveDefaultMaterial(0.f, true);
    RefreshMaterialParameters();
    UpdateSimulationInputs(0.f);
    BindSimulationFields();
}

void AYogComputeFogActor::ResetComputeFogSimulation()
{
    if (Simulation) Simulation->ResetSimulation();
    PreviousBodyPositions.Reset();
    RefreshComputeFog();
}

void AYogComputeFogActor::ResetDemoMotion()
{
    DemoClockSeconds = 0.;
    DemoTimeSeconds = 0.f;
    UpdateSimulationInputs(0.f);
}

FString AYogComputeFogActor::GetComputeFogStatus() const
{
    const UWorld* World = GetWorld();
    const TCHAR* AdvanceStatus = TEXT("模拟组件缺失");
    if (Simulation)
    {
        if (!World || World->bIsTearingDown || !Simulation->IsRegistered()) AdvanceStatus = TEXT("未注册或世界已停止");
        else if (!Simulation->bSimulationEnabled) AdvanceStatus = TEXT("三维模拟已暂停；保留当前GPU场");
        else if (!Simulation->IsComponentTickEnabled()) AdvanceStatus = TEXT("模拟组件Tick已关闭");
        else if (!World->IsGameWorld() && !Simulation->bSimulateInEditor) AdvanceStatus = TEXT("编辑器三维模拟已暂停；保留当前GPU场");
        else AdvanceStatus = World->IsGameWorld() ? TEXT("游戏三维模拟运行") : TEXT("编辑器三维模拟运行；需实时视口");
    }
    const IConsoleVariable* FogSwitch = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog"));
    const int32 FogSwitchValue = FogSwitch ? FogSwitch->GetInt() : -1;
    return FString::Printf(TEXT("密度来源=持久GPU Texture3D；动画控制=Simulation组件\n三轴形态请在Simulation.ShapeScaleXYZ调整；旧世界形态频率/密度细节强度不参与计算雾\n%s\n材质=%s；owned MID绑定=%s\n原生雾组件=%s；r.VolumetricFog=%d\n%s\n%s\n观察者=%s\n方向光=%s\n配色=%s\n交互胶囊=%d/8；演示提交=%s；演示独立时间=%.3fs；相位=%.3f"),
        AdvanceStatus, IsValid(VolumeMaterial) ? *VolumeMaterial->GetPathName() : TEXT("缺失"),
        IsFogMaterialBound() ? TEXT("是") : TEXT("否"),
        NativeFog && NativeFog->bEnableVolumetricFog && NativeFog->IsVisible() ? TEXT("启用") : TEXT("关闭"), FogSwitchValue,
        *FieldBindingStatus, Simulation ? *Simulation->GetSimulationStatus() : TEXT("模拟组件缺失"),
        *ObserverSource, *DirectionalSource, *PaletteSource,
        ActiveBodyCount, bDemoBodySubmitted ? TEXT("是") : TEXT("否"), DemoTimeSeconds, DemoNormalizedPhase);
}
