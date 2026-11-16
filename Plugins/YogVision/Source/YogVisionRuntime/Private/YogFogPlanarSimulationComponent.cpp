#include "YogFogPlanarSimulationComponent.h"
#include "YogPlanarFogActor.h"

#include "Containers/StaticArray.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GlobalRenderResources.h"
#include "GlobalShader.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/App.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIStaticStates.h"
#include "RHITextureReference.h"
#include "ShaderParameterStruct.h"
#include "TextureResource.h"
#include "UObject/UnrealType.h"

struct FYogFogPlanarRenderState
{
    TStaticArray<FTextureRHIRef, 2> Textures;
    FThreadSafeCounter ResourceStatus;
    FThreadSafeCounter ExecutedGraphs;
    FThreadSafeCounter ArtTextureReady;
    FThreadSafeCounter SeededFromArtwork;
};

namespace YogPlanarFog
{
constexpr float FixedStep = 1.f / 30.f;
constexpr int32 MaximumBodies = 8;
float Safe(double Value, float Default, float Min, float Max)
{ return FMath::Clamp(FMath::IsFinite(Value) ? static_cast<float>(Value) : Default, Min, Max); }
FVector2D Safe2(FVector2D Value, FVector2D Default, float Min, float Max)
{ return FVector2D(Safe(Value.X, Default.X, Min, Max), Safe(Value.Y, Default.Y, Min, Max)); }
FIntPoint SafeResolution(FIntPoint Value)
{ return FIntPoint(FMath::Clamp(Value.X, 16, 1024), FMath::Clamp(Value.Y, 16, 1024)); }
FTextureReferenceRHIRef ArtReference(const UTexture2D* Texture)
{ return IsValid(Texture) ? Texture->TextureReference.TextureReferenceRHI : FTextureReferenceRHIRef(); }

struct FDispatch
{
    FIntPoint Size;
    FVector3f Center;
    FVector2f Extent, TileSize, Offset, Wind;
    float SeedScale = 1.f, SeedBias = .05f, Curl = 15.f, Replenishment = .04f, Damping = .3f;
    float WakeDecay = 3.f, StartTime = 0.f;
    TStaticArray<FVector4f, MaximumBodies> Bodies;
    TStaticArray<FVector4f, MaximumBodies> BodyVelocities;
    FTextureReferenceRHIRef ArtTextureReference;
    uint32 BodyCount = 0;
    int32 Steps = 0;
    bool bReset = false;
};

BEGIN_SHADER_PARAMETER_STRUCT(FSeedParameters, )
    SHADER_PARAMETER(FIntPoint, GridSize)
    SHADER_PARAMETER(FVector3f, DomainCenter)
    SHADER_PARAMETER(FVector2f, DomainHalfExtent)
    SHADER_PARAMETER(FVector2f, DensityTileSize)
    SHADER_PARAMETER(FVector2f, ShapeOffsetUV)
    SHADER_PARAMETER(float, SeedDensityScale)
    SHADER_PARAMETER(float, SeedDensityBias)
    SHADER_PARAMETER(float, UseDensityTexture)
    SHADER_PARAMETER(FVector2f, WindVelocity)
    SHADER_PARAMETER(float, CurlStrength)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, ArtDensity)
    SHADER_PARAMETER_SAMPLER(SamplerState, LinearWrapSampler)
END_SHADER_PARAMETER_STRUCT()

class FInitializeCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FInitializeCS);
    SHADER_USE_PARAMETER_STRUCT(FInitializeCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FSeedParameters, Seed)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputField)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};

class FAdvectCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FAdvectCS);
    SHADER_USE_PARAMETER_STRUCT(FAdvectCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_INCLUDE(FSeedParameters, Seed)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, SimulationTime)
        SHADER_PARAMETER(float, Replenishment)
        SHADER_PARAMETER(float, VelocityDamping)
        SHADER_PARAMETER(float, WakeDecaySeconds)
        SHADER_PARAMETER(uint32, BodyCount)
        SHADER_PARAMETER_ARRAY(FVector4f, BodyCenterRadius, [8])
        SHADER_PARAMETER_ARRAY(FVector4f, BodyVelocityUnused, [8])
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D<float4>, InputField)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputField)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FInitializeCS, "/Plugin/YogVision/Private/YogFogPlanarSimulation.usf", "InitializePlanarFogCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAdvectCS, "/Plugin/YogVision/Private/YogFogPlanarSimulation.usf", "AdvectPlanarFogCS", SF_Compute);

void Dispatch(FRHICommandListImmediate& RHICmdList, FYogFogPlanarRenderState& State, const FDispatch& Input)
{
    if (State.ResourceStatus.GetValue() != 1) return;
    FRDGBuilder GraphBuilder(RHICmdList, RDG_EVENT_NAME("YOGPlanarFog"));
    {
        RDG_EVENT_SCOPE(GraphBuilder, "YOGPlanarFog %dx%d", Input.Size.X, Input.Size.Y);
        FRDGTextureRef A = RegisterExternalTexture(GraphBuilder, State.Textures[0], TEXT("YOGPlanarFog.StableA"));
        FRDGTextureRef B = RegisterExternalTexture(GraphBuilder, State.Textures[1], TEXT("YOGPlanarFog.ScratchB"));
        FRHITexture* Source = Input.ArtTextureReference ? Input.ArtTextureReference->GetReferencedTexture() : nullptr;
        const bool bArtReady = Source && Source != FRHITextureReference::GetDefaultTexture()
            && Source->GetDesc().Dimension == ETextureDimension::Texture2D;
        State.ArtTextureReady.Set(bArtReady ? 1 : 0);
        if (!bArtReady) Source = GWhiteTexture->TextureRHI;
        FRDGTextureRef Art = RegisterExternalTexture(GraphBuilder, Source, TEXT("YOGPlanarFog.ArtDensity"));
        FSeedParameters Seed;
        Seed.GridSize = Input.Size; Seed.DomainCenter = Input.Center; Seed.DomainHalfExtent = Input.Extent;
        Seed.DensityTileSize = Input.TileSize; Seed.ShapeOffsetUV = Input.Offset;
        Seed.SeedDensityScale = Input.SeedScale; Seed.SeedDensityBias = Input.SeedBias;
        Seed.UseDensityTexture = bArtReady ? 1.f : 0.f;
        Seed.WindVelocity = Input.Wind; Seed.CurlStrength = Input.Curl; Seed.ArtDensity = Art;
        Seed.LinearWrapSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap>::GetRHI();
        FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        const FIntVector Groups = FComputeShaderUtils::GetGroupCount(Input.Size, 8);
        const ERDGPassFlags Flags = ERDGPassFlags::Compute | ERDGPassFlags::NeverCull;
        if (Input.bReset)
        {
            State.SeededFromArtwork.Set(bArtReady ? 1 : 0);
            auto* P = GraphBuilder.AllocParameters<FInitializeCS::FParameters>();
            P->Seed = Seed; P->OutputField = GraphBuilder.CreateUAV(A);
            FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGPlanarFog.InitializeArtwork"), Flags,
                TShaderMapRef<FInitializeCS>(ShaderMap), P, Groups);
        }
        for (int32 Step = 0; Step < Input.Steps; ++Step)
        {
            auto* P = GraphBuilder.AllocParameters<FAdvectCS::FParameters>();
            P->Seed = Seed; P->DeltaTime = FixedStep;
            P->SimulationTime = Input.StartTime + Step * FixedStep;
            P->Replenishment = Input.Replenishment; P->VelocityDamping = Input.Damping;
            P->WakeDecaySeconds = Input.WakeDecay; P->BodyCount = Input.BodyCount;
            for (int32 Body = 0; Body < MaximumBodies; ++Body)
            { P->BodyCenterRadius[Body] = Input.Bodies[Body]; P->BodyVelocityUnused[Body] = Input.BodyVelocities[Body]; }
            P->InputField = A; P->OutputField = GraphBuilder.CreateUAV(B);
            P->LinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp>::GetRHI();
            FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGPlanarFog.AdvectAndBodies"), Flags,
                TShaderMapRef<FAdvectCS>(ShaderMap), P, Groups);
            FRHICopyTextureInfo Copy; Copy.Size = FIntVector(Input.Size.X, Input.Size.Y, 1);
            AddCopyTexturePass(GraphBuilder, B, A, Copy);
        }
        GraphBuilder.SetTextureAccessFinal(A, ERHIAccess::SRVMask);
        GraphBuilder.SetTextureAccessFinal(B, ERHIAccess::SRVMask);
        GraphBuilder.SetTextureAccessFinal(Art, ERHIAccess::SRVMask);
    }
    GraphBuilder.Execute();
    State.ExecutedGraphs.Increment();
}
}

UYogFogPlanarSimulationComponent::UYogFogPlanarSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    bTickInEditor = true;
}

void UYogFogPlanarSimulationComponent::OnRegister()
{
    Super::OnRegister();
    if (HasSeedChanged()) bResetPending = true;
    if (EnsureResources() && bResetPending) QueueSimulation(0, true);
}
void UYogFogPlanarSimulationComponent::OnUnregister()
{
    // Editor property changes reregister the component. Preserve its current GPU field.
    TimeAccumulator = 0.f; bWasSimulationActive = false;
    Super::OnUnregister();
}
void UYogFogPlanarSimulationComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{ ReleaseRenderState(); Super::OnComponentDestroyed(bDestroyingHierarchy); }
void UYogFogPlanarSimulationComponent::BeginDestroy()
{ ReleaseRenderState(); Super::BeginDestroy(); }
void UYogFogPlanarSimulationComponent::BeginPlay()
{ Super::BeginPlay(); ResetSimulation(); }

#if WITH_EDITOR
bool UYogFogPlanarSimulationComponent::CanEditChange(const FProperty* InProperty) const
{
    if (!Super::CanEditChange(InProperty)) return false;
    if (InProperty && GetOwner() && GetOwner()->IsA<AYogPlanarFogActor>())
    {
        const FName Name = InProperty->GetFName();
        if (Name == GET_MEMBER_NAME_CHECKED(UYogFogPlanarSimulationComponent, DensityTexture)
            || Name == GET_MEMBER_NAME_CHECKED(UYogFogPlanarSimulationComponent, DensityTileSize)) return false;
    }
    return true;
}

void UYogFogPlanarSimulationComponent::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    if (Event.GetMemberPropertyName() == GET_MEMBER_NAME_CHECKED(UYogFogPlanarSimulationComponent, GridResolution)
        || HasSeedChanged()) ResetSimulation();
    Super::PostEditChangeProperty(Event);
}
#endif

bool UYogFogPlanarSimulationComponent::HasSeedChanged() const
{
    using namespace YogPlanarFog;
    return CachedDensityTexture.Get() != DensityTexture.Get() || CachedArtReference != ArtReference(DensityTexture).GetReference()
        || (IsValid(DensityTexture) && RenderState && RenderState->ArtTextureReady.GetValue() == 1
            && RenderState->SeededFromArtwork.GetValue() == 0)
        || !CachedTileSize.Equals(Safe2(DensityTileSize.GetAbs(), FVector2D(1800., 1800.), 1.f, 100000.f), .001)
        || !CachedOffsetUV.Equals(Safe2(ShapeOffsetUV, FVector2D::ZeroVector, -10000.f, 10000.f), .0001)
        || !FMath::IsNearlyEqual(CachedSeedScale, Safe(SeedDensityScale, 1.f, 0.f, 3.f))
        || !FMath::IsNearlyEqual(CachedSeedBias, Safe(SeedDensityBias, .05f, -1.f, 1.f));
}

void UYogFogPlanarSimulationComponent::SetDomain(FVector Center, FVector2D HalfExtent)
{
    using namespace YogPlanarFog;
    Center = FVector(Safe(Center.X, 0.f, -10000000.f, 10000000.f), Safe(Center.Y, 0.f, -10000000.f, 10000000.f), Safe(Center.Z, 0.f, -10000000.f, 10000000.f));
    HalfExtent = Safe2(HalfExtent.GetAbs(), FVector2D(2200., 2400.), 1.f, 100000.f);
    // Z only selects the interaction height band; it never changes this XY texture's mapping.
    if (!FVector2D(DomainCenter.X, DomainCenter.Y).Equals(FVector2D(Center.X, Center.Y), .01)
        || !DomainHalfExtent.Equals(HalfExtent, .01)) bResetPending = true;
    DomainCenter = ActualDomainCenter = Center;
    DomainHalfExtent = ActualDomainHalfExtent = HalfExtent;
    const FIntPoint Size = SafeResolution(GridResolution);
    CellSize = HalfExtent * 2. / FVector2D(Size.X, Size.Y);
}

void UYogFogPlanarSimulationComponent::SetBodies(const TArray<FYogFogSimBody>& Bodies)
{
    InteractionBodies.Reset(YogPlanarFog::MaximumBodies);
    for (const auto& Input : Bodies)
    {
        if (InteractionBodies.Num() == YogPlanarFog::MaximumBodies) break;
        if (Input.Center.ContainsNaN() || !FMath::IsFinite(Input.Radius) || Input.Radius <= 0.f) continue;
        FYogFogSimBody Body = Input;
        Body.Radius = YogPlanarFog::Safe(Body.Radius, 45.f, 1.f, 2000.f);
        Body.HalfHeight = YogPlanarFog::Safe(Body.HalfHeight, 90.f, Body.Radius, 5000.f);
        Body.Velocity = FVector(YogPlanarFog::Safe(Body.Velocity.X, 0.f, -1500.f, 1500.f), YogPlanarFog::Safe(Body.Velocity.Y, 0.f, -1500.f, 1500.f), 0.).GetClampedToMaxSize(1500.);
        InteractionBodies.Add(Body);
    }
}

bool UYogFogPlanarSimulationComponent::EnsureResources()
{
    if (IsTemplate() || !IsRegistered() || !FApp::CanEverRender() || GUsingNullRHI || IsRunningDedicatedServer()) return false;
    const FIntPoint Size = YogPlanarFog::SafeResolution(GridResolution);
    if (RenderState && Size == AllocatedResolution && PlanarField && ScratchField) return true;
    ReleaseRenderState();
    TArray<FTextureRenderTargetResource*> Resources;
    const auto MakeTarget = [this, Size, &Resources](const TCHAR* Label)
    {
        auto* T = NewObject<UTextureRenderTarget2D>(this, MakeUniqueObjectName(this, UTextureRenderTarget2D::StaticClass(), FName(Label)), RF_Transient);
        T->ClearColor = FLinearColor::Black; T->bSupportsUAV = true; T->bAutoGenerateMips = false; T->SRGB = false;
        T->InitCustomFormat(Size.X, Size.Y, PF_FloatRGBA, true);
        T->UpdateResourceImmediate(true);
        Resources.Add(T->GameThread_GetRenderTargetResource());
        return T;
    };
    PlanarField = MakeTarget(TEXT("PlanarFogStableA")); ScratchField = MakeTarget(TEXT("PlanarFogScratchB"));
    AllocatedResolution = ActualGridResolution = Size;
    SetDomain(DomainCenter, DomainHalfExtent);
    bResetPending = true;
    RenderState = MakeShared<FYogFogPlanarRenderState, ESPMode::ThreadSafe>();
    const auto State = RenderState;
    ENQUEUE_RENDER_COMMAND(YogPlanarFogInitializeResources)([State, Resources = MoveTemp(Resources)](FRHICommandListImmediate&)
    {
        bool bValid = Resources.Num() == 2;
        for (int32 Index = 0; Index < Resources.Num(); ++Index)
        { State->Textures[Index] = Resources[Index] ? Resources[Index]->GetRenderTargetTexture() : nullptr; bValid &= State->Textures[Index].IsValid(); }
        State->ResourceStatus.Set(bValid ? 1 : 2);
    });
    return true;
}

void UYogFogPlanarSimulationComponent::ReleaseRenderState()
{
    if (RenderState)
    {
        const auto State = MoveTemp(RenderState);
        ENQUEUE_RENDER_COMMAND(YogPlanarFogReleaseResources)([State](FRHICommandListImmediate&)
        { for (auto& T : State->Textures) T.SafeRelease(); State->ResourceStatus.Set(0); });
    }
    AllocatedResolution = FIntPoint::ZeroValue;
    TimeAccumulator = 0.f; bWasSimulationActive = false;
}

void UYogFogPlanarSimulationComponent::ResetSimulation()
{
    SimulationTime = 0.f; SubmittedStepCount = 0; TimeAccumulator = 0.f; bResetPending = true;
    SubmittedBodyCount = 0;
    if (EnsureResources()) QueueSimulation(0, true);
}

void UYogFogPlanarSimulationComponent::QueueSimulation(int32 Steps, bool bReset)
{
    using namespace YogPlanarFog;
    if (!RenderState) return;
    FDispatch Input;
    Input.Size = AllocatedResolution; Input.Center = FVector3f(DomainCenter); Input.Extent = FVector2f(DomainHalfExtent);
    Input.TileSize = FVector2f(Safe2(DensityTileSize.GetAbs(), FVector2D(1800., 1800.), 1.f, 100000.f));
    Input.Offset = FVector2f(Safe2(ShapeOffsetUV, FVector2D::ZeroVector, -10000.f, 10000.f));
    Input.SeedScale = Safe(SeedDensityScale, 1.f, 0.f, 3.f); Input.SeedBias = Safe(SeedDensityBias, .05f, -1.f, 1.f);
    Input.Wind = FVector2f(Safe2(WindVelocity, FVector2D(3., -1.), -300.f, 300.f));
    Input.Curl = Safe(CurlStrength, 15.f, 0.f, 100.f); Input.Replenishment = Safe(DensityReplenishment, .04f, 0.f, 1.f);
    Input.Damping = Safe(VelocityDamping, .3f, 0.f, 5.f); Input.WakeDecay = Safe(WakeDecaySeconds, 3.f, .1f, 30.f);
    Input.StartTime = bReset ? 0.f : SimulationTime; Input.Steps = FMath::Clamp(Steps, 0, 2); Input.bReset = bReset;
    Input.ArtTextureReference = ArtReference(DensityTexture);
    CachedDensityTexture = DensityTexture; CachedArtReference = Input.ArtTextureReference.GetReference();
    CachedTileSize = FVector2D(Input.TileSize); CachedOffsetUV = FVector2D(Input.Offset);
    CachedSeedScale = Input.SeedScale; CachedSeedBias = Input.SeedBias;
    for (int32 Index = 0; Index < MaximumBodies; ++Index)
    { Input.Bodies[Index] = FVector4f(0, 0, 0, 1); Input.BodyVelocities[Index] = FVector4f(0, 0, 0, 0); }
    const double Band = Safe(HeightBandHalfHeight, 600.f, 1.f, 50000.f);
    const double Kernel = CellSize.GetMax() * 1.5;
    for (const auto& Body : InteractionBodies)
    {
        const double DZ = FMath::Max(0., FMath::Abs(Body.Center.Z - DomainCenter.Z) - FMath::Max(0.f, Body.HalfHeight - Body.Radius) - Band);
        if (DZ >= Body.Radius) continue;
        const float Radius = static_cast<float>(FMath::Max(Kernel, FMath::Sqrt(FMath::Max(0., Body.Radius * Body.Radius - DZ * DZ))));
        const FVector2D Offset(Body.Center.X - DomainCenter.X, Body.Center.Y - DomainCenter.Y);
        if (FMath::Abs(Offset.X) > DomainHalfExtent.X + Radius || FMath::Abs(Offset.Y) > DomainHalfExtent.Y + Radius) continue;
        const uint32 Index = Input.BodyCount++;
        Input.Bodies[Index] = FVector4f(FVector3f(Body.Center), Radius);
        Input.BodyVelocities[Index] = FVector4f(FVector3f(Body.Velocity), 0.f);
        if (Input.BodyCount == MaximumBodies) break;
    }
    const auto State = RenderState;
    SubmittedBodyCount = static_cast<int32>(Input.BodyCount);
    ENQUEUE_RENDER_COMMAND(YogPlanarFogCompute)([State, Input](FRHICommandListImmediate& RHICmdList) { Dispatch(RHICmdList, *State, Input); });
    if (bReset) { SimulationTime = 0.f; SubmittedStepCount = 0; }
    SimulationTime += Input.Steps * FixedStep; SubmittedStepCount += Input.Steps;
    bResetPending = false;
}

void UYogFogPlanarSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UWorld* World = GetWorld();
    if (!World || World->bIsTearingDown || !EnsureResources()) { TimeAccumulator = 0.f; bWasSimulationActive = false; return; }
    if (HasSeedChanged()) bResetPending = true;
    const bool bActive = bSimulationEnabled && (World->IsGameWorld() || bSimulateInEditor);
    if (!bActive)
    {
        TimeAccumulator = 0.f; bWasSimulationActive = false;
        // Explicit artwork/domain edits remain visible while playback is paused. Merely
        // toggling playback never dirties the seed and therefore preserves the current field.
        if (bResetPending) QueueSimulation(0, true);
        return;
    }
    if (!bWasSimulationActive) TimeAccumulator = 0.f;
    bWasSimulationActive = true;
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f)
    { if (bResetPending) QueueSimulation(0, true); return; }
    TimeAccumulator += FMath::Min(DeltaTime, YogPlanarFog::FixedStep * 2.f);
    const int32 Steps = FMath::Min(2, FMath::FloorToInt((TimeAccumulator + .000001f) / YogPlanarFog::FixedStep));
    TimeAccumulator = FMath::Max(0.f, TimeAccumulator - Steps * YogPlanarFog::FixedStep);
    if (Steps > 0 || bResetPending) QueueSimulation(Steps, bResetPending);
}

FString UYogFogPlanarSimulationComponent::GetSimulationStatus() const
{
    return FString::Printf(TEXT("World XY advection; RG=velocity cm/s, B=density, A=signed wake\nresources=%d; executed-graphs=%d (not GPU readback); artwork-ready=%d\ngrid=%dx%d; cell-cm=%s; center=%s; half-extent=%s\nqueued-time=%.3fs; fixed-steps=%d; bodies=%d/8\nartwork=%s; texture-tile-cm=%s; buffers=2 RGBA16F; height reconstructed by material"),
        RenderState ? RenderState->ResourceStatus.GetValue() : 0, RenderState ? RenderState->ExecutedGraphs.GetValue() : 0,
        RenderState ? RenderState->ArtTextureReady.GetValue() : 0, ActualGridResolution.X, ActualGridResolution.Y,
        *CellSize.ToString(), *ActualDomainCenter.ToString(), *ActualDomainHalfExtent.ToString(), SimulationTime, SubmittedStepCount,
        SubmittedBodyCount, IsValid(DensityTexture) ? *DensityTexture->GetPathName() : TEXT("none: uniform base"), *DensityTileSize.ToString());
}
