#include "YogFogSimulationComponent.h"

#include "Containers/StaticArray.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "HAL/ThreadSafeCounter.h"
#include "Misc/App.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIStaticStates.h"
#include "ShaderParameterStruct.h"
#include "TextureRenderTargetVolumeResource.h"

struct FYogFogSimulationRenderState
{
    TStaticArray<FTextureRHIRef, 7> Textures;
    FThreadSafeCounter ResourceStatus; // 0 pending, 1 ready, 2 resource unavailable.
    FThreadSafeCounter ExecutedGraphs;
};

namespace YogFogSim
{
constexpr float FixedStep = 1.f / 30.f;
constexpr int32 MaximumBodies = 8;
enum EField : int32 { DensityA, DensityB, VelocityA, VelocityB, PressureA, PressureB, Divergence, FieldCount };
float Safe(float Value, float Fallback, float Min, float Max)
{
    return FMath::Clamp(FMath::IsFinite(Value) ? Value : Fallback, Min, Max);
}
FVector SafeVector(FVector Value, FVector Fallback, float Min, float Max)
{
    return FVector(Safe(static_cast<float>(Value.X), static_cast<float>(Fallback.X), Min, Max),
        Safe(static_cast<float>(Value.Y), static_cast<float>(Fallback.Y), Min, Max),
        Safe(static_cast<float>(Value.Z), static_cast<float>(Fallback.Z), Min, Max));
}
FIntVector SafeResolution(FIntVector Value)
{
    return FIntVector(FMath::Clamp(Value.X, 8, 128), FMath::Clamp(Value.Y, 8, 128), FMath::Clamp(Value.Z, 8, 128));
}
struct FDispatch
{
    FIntVector Size;
    FVector3f Center, Extent, Shape, Wind, Metric;
    TStaticArray<FVector4f, MaximumBodies> Bodies;
    TStaticArray<FVector4f, MaximumBodies> BodyVelocities;
    uint32 BodyCount = 0;
    float Curl = 15.f, Replenishment = .04f, Damping = .3f, StartTime = 0.f;
    int32 Steps = 0, Iterations = 8;
    bool bReset = false, bProject = false;
};

class FInitializeCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FInitializeCS);
    SHADER_USE_PARAMETER_STRUCT(FInitializeCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(FVector3f, DomainCenter)
        SHADER_PARAMETER(FVector3f, DomainExtent)
        SHADER_PARAMETER(FVector3f, ShapeScaleXYZ)
        SHADER_PARAMETER(FVector3f, WindVelocity)
        SHADER_PARAMETER(float, CurlStrength)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputDensity)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputVelocity)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
class FAdvectCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FAdvectCS);
    SHADER_USE_PARAMETER_STRUCT(FAdvectCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(FVector3f, DomainCenter)
        SHADER_PARAMETER(FVector3f, DomainExtent)
        SHADER_PARAMETER(FVector3f, ShapeScaleXYZ)
        SHADER_PARAMETER(FVector3f, WindVelocity)
        SHADER_PARAMETER(float, CurlStrength)
        SHADER_PARAMETER(float, DeltaTime)
        SHADER_PARAMETER(float, SimulationTime)
        SHADER_PARAMETER(float, Replenishment)
        SHADER_PARAMETER(float, VelocityDamping)
        SHADER_PARAMETER(uint32, BodyCount)
        SHADER_PARAMETER_ARRAY(FVector4f, BodyCenterRadius, [8])
        SHADER_PARAMETER_ARRAY(FVector4f, BodyVelocityHalfHeight, [8])
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputDensity)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputVelocity)
        SHADER_PARAMETER_SAMPLER(SamplerState, LinearClampSampler)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputDensity)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputVelocity)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
class FDivergenceCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FDivergenceCS);
    SHADER_USE_PARAMETER_STRUCT(FDivergenceCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(FVector3f, InvCellMetric)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputVelocity)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputDivergence)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
class FJacobiCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FJacobiCS);
    SHADER_USE_PARAMETER_STRUCT(FJacobiCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(FVector3f, InvCellMetric)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputPressure)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputDivergence)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputPressure)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
class FProjectCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FProjectCS);
    SHADER_USE_PARAMETER_STRUCT(FProjectCS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER(FIntVector, GridSize)
        SHADER_PARAMETER(FVector3f, InvCellMetric)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputVelocity)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D<float4>, InputPressure)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>, OutputVelocity)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FInitializeCS, "/Plugin/YogVision/Private/YogFogSimulation.usf", "InitializeFogCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FAdvectCS, "/Plugin/YogVision/Private/YogFogSimulation.usf", "AdvectFogCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FDivergenceCS, "/Plugin/YogVision/Private/YogFogSimulation.usf", "DivergenceFogCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FJacobiCS, "/Plugin/YogVision/Private/YogFogSimulation.usf", "JacobiFogCS", SF_Compute);
IMPLEMENT_GLOBAL_SHADER(FProjectCS, "/Plugin/YogVision/Private/YogFogSimulation.usf", "ProjectFogCS", SF_Compute);

void Dispatch(FRHICommandListImmediate& RHICmdList, FYogFogSimulationRenderState& State, const FDispatch& Input)
{
    if (State.ResourceStatus.GetValue() != 1) return;
    FRDGBuilder GraphBuilder(RHICmdList, RDG_EVENT_NAME("YOGFogSim"));
    {
        RDG_EVENT_SCOPE(GraphBuilder, "YOGFogSim %dx%dx%d %s", Input.Size.X, Input.Size.Y, Input.Size.Z,
            Input.bProject ? TEXT("ProjectedNS") : TEXT("Advection"));
        TStaticArray<FRDGTextureRef, FieldCount> Fields;
        const TCHAR* Names[FieldCount] = { TEXT("YOGFogSim.DensityA"), TEXT("YOGFogSim.DensityB"),
            TEXT("YOGFogSim.VelocityA"), TEXT("YOGFogSim.VelocityB"), TEXT("YOGFogSim.PressureA"),
            TEXT("YOGFogSim.PressureB"), TEXT("YOGFogSim.Divergence") };
        for (int32 Index = 0; Index < FieldCount; ++Index)
            Fields[Index] = RegisterExternalTexture(GraphBuilder, State.Textures[Index], Names[Index]);
        const FIntVector Groups = FComputeShaderUtils::GetGroupCount(Input.Size, 4);
        const ERDGPassFlags Flags = ERDGPassFlags::Compute | ERDGPassFlags::NeverCull;
        FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        FRHICopyTextureInfo CopyInfo;
        CopyInfo.Size = Input.Size;
        if (Input.bReset)
        {
            auto* P = GraphBuilder.AllocParameters<FInitializeCS::FParameters>();
            P->GridSize = Input.Size; P->DomainCenter = Input.Center; P->DomainExtent = Input.Extent;
            P->ShapeScaleXYZ = Input.Shape; P->WindVelocity = Input.Wind; P->CurlStrength = Input.Curl;
            P->OutputDensity = GraphBuilder.CreateUAV(Fields[DensityA]);
            P->OutputVelocity = GraphBuilder.CreateUAV(Fields[VelocityA]);
            FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGFogSim.Initialize3D"), Flags,
                TShaderMapRef<FInitializeCS>(ShaderMap), P, Groups);
            AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Fields[PressureA]), FLinearColor::Black);
            AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Fields[PressureB]), FLinearColor::Black);
            AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Fields[Divergence]), FLinearColor::Black);
        }
        for (int32 Step = 0; Step < Input.Steps; ++Step)
        {
            auto* P = GraphBuilder.AllocParameters<FAdvectCS::FParameters>();
            P->GridSize = Input.Size; P->DomainCenter = Input.Center; P->DomainExtent = Input.Extent;
            P->ShapeScaleXYZ = Input.Shape; P->WindVelocity = Input.Wind; P->CurlStrength = Input.Curl;
            P->DeltaTime = FixedStep; P->SimulationTime = Input.StartTime + Step * FixedStep;
            P->Replenishment = Input.Replenishment; P->VelocityDamping = Input.Damping;
            P->BodyCount = Input.BodyCount;
            for (int32 Body = 0; Body < MaximumBodies; ++Body)
            { P->BodyCenterRadius[Body] = Input.Bodies[Body]; P->BodyVelocityHalfHeight[Body] = Input.BodyVelocities[Body]; }
            P->InputDensity = Fields[DensityA]; P->InputVelocity = Fields[VelocityA];
            P->LinearClampSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
            P->OutputDensity = GraphBuilder.CreateUAV(Fields[DensityB]);
            P->OutputVelocity = GraphBuilder.CreateUAV(Fields[VelocityB]);
            FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGFogSim.AdvectAndBodies"), Flags,
                TShaderMapRef<FAdvectCS>(ShaderMap), P, Groups);
            if (Input.bProject)
            {
                auto* D = GraphBuilder.AllocParameters<FDivergenceCS::FParameters>();
                D->GridSize = Input.Size; D->InvCellMetric = Input.Metric; D->InputVelocity = Fields[VelocityB];
                D->OutputDivergence = GraphBuilder.CreateUAV(Fields[Divergence]);
                FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGFogSim.Divergence"), Flags,
                    TShaderMapRef<FDivergenceCS>(ShaderMap), D, Groups);
                AddClearUAVPass(GraphBuilder, GraphBuilder.CreateUAV(Fields[PressureA]), FLinearColor::Black);
                FRDGTextureRef PressureRead = Fields[PressureA], PressureWrite = Fields[PressureB];
                for (int32 Iteration = 0; Iteration < Input.Iterations; ++Iteration)
                {
                    auto* J = GraphBuilder.AllocParameters<FJacobiCS::FParameters>();
                    J->GridSize = Input.Size; J->InvCellMetric = Input.Metric;
                    J->InputPressure = PressureRead; J->InputDivergence = Fields[Divergence];
                    J->OutputPressure = GraphBuilder.CreateUAV(PressureWrite);
                    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGFogSim.PressureJacobi %d", Iteration), Flags,
                        TShaderMapRef<FJacobiCS>(ShaderMap), J, Groups);
                    Swap(PressureRead, PressureWrite);
                }
                auto* Project = GraphBuilder.AllocParameters<FProjectCS::FParameters>();
                Project->GridSize = Input.Size; Project->InvCellMetric = Input.Metric;
                Project->InputPressure = PressureRead; Project->InputVelocity = Fields[VelocityB];
                Project->OutputVelocity = GraphBuilder.CreateUAV(Fields[VelocityA]);
                FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("YOGFogSim.PressureProject"), Flags,
                    TShaderMapRef<FProjectCS>(ShaderMap), Project, Groups);
            }
            else AddCopyTexturePass(GraphBuilder, Fields[VelocityB], Fields[VelocityA], CopyInfo);
            // Stable A is always the UTexture exposed to materials. No GT ping-pong texture swaps.
            AddCopyTexturePass(GraphBuilder, Fields[DensityB], Fields[DensityA], CopyInfo);
        }
        for (FRDGTextureRef Field : Fields) GraphBuilder.SetTextureAccessFinal(Field, ERHIAccess::SRVMask);
    } // Pop all graph event scopes before Execute validates the breadcrumb sentinel.
    GraphBuilder.Execute();
    State.ExecutedGraphs.Increment();
}
}

UYogFogSimulationComponent::UYogFogSimulationComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    bTickInEditor = true;
}

void UYogFogSimulationComponent::OnRegister()
{
    Super::OnRegister();
    // Property edits temporarily reregister native components. Keep their current GPU field.
    const FVector Shape = YogFogSim::SafeVector(ShapeScaleXYZ, FVector::OneVector, .05f, 20.f);
    if (!CachedShapeScale.Equals(Shape, .0001)) bResetPending = true;
    if (EnsureResources() && bResetPending) QueueSimulation(0, true);
}

void UYogFogSimulationComponent::OnUnregister()
{
    // Unregister is also used by editor property transactions; it is not destruction.
    // Discard partial elapsed time, but retain textures and queued simulation time for resume.
    TimeAccumulator = 0.f;
    bWasSimulationActive = false;
    Super::OnUnregister();
}

void UYogFogSimulationComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
    ReleaseRenderState();
    Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UYogFogSimulationComponent::BeginDestroy()
{
    ReleaseRenderState();
    Super::BeginDestroy();
}

void UYogFogSimulationComponent::BeginPlay()
{
    Super::BeginPlay();
    ResetSimulation();
}

#if WITH_EDITOR
void UYogFogSimulationComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    const FName PropertyName = PropertyChangedEvent.GetMemberPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(UYogFogSimulationComponent, GridResolution)
        || PropertyName == GET_MEMBER_NAME_CHECKED(UYogFogSimulationComponent, ShapeScaleXYZ))
    {
        // PreEditChange may already have unregistered us. OnRegister consumes the pending reset.
        ResetSimulation();
    }
    Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

void UYogFogSimulationComponent::SetDomain(FVector Center, FVector Extent)
{
    using namespace YogFogSim;
    Center = SafeVector(Center, FVector::ZeroVector, -10000000.f, 10000000.f);
    Extent = SafeVector(Extent.GetAbs(), FVector(2200.f, 2400.f, 1400.f), 1.f, 50000.f);
    if (!DomainCenter.Equals(Center, .01) || !DomainExtent.Equals(Extent, .01)) bResetPending = true;
    DomainCenter = ActualDomainCenter = Center;
    DomainExtent = ActualDomainExtent = Extent;
    const FIntVector Size = SafeResolution(GridResolution);
    CellSize = Extent * 2. / FVector(Size.X, Size.Y, Size.Z);
}

void UYogFogSimulationComponent::SetBodies(const TArray<FYogFogSimBody>& Bodies)
{
    InteractionBodies.Reset(YogFogSim::MaximumBodies);
    for (const FYogFogSimBody& Input : Bodies)
    {
        if (InteractionBodies.Num() == YogFogSim::MaximumBodies) break;
        if (Input.Center.ContainsNaN() || !FMath::IsFinite(Input.Radius) || Input.Radius <= 0.f) continue;
        FYogFogSimBody Body = Input;
        Body.Velocity = YogFogSim::SafeVector(Input.Velocity, FVector::ZeroVector, -1500.f, 1500.f).GetClampedToMaxSize(1500.);
        Body.Radius = YogFogSim::Safe(Input.Radius, 45.f, 1.f, 2000.f);
        Body.HalfHeight = YogFogSim::Safe(Input.HalfHeight, 90.f, Body.Radius, 5000.f);
        InteractionBodies.Add(Body);
    }
}

bool UYogFogSimulationComponent::EnsureResources()
{
    if (IsTemplate() || !IsRegistered() || !FApp::CanEverRender() || GUsingNullRHI || IsRunningDedicatedServer()) return false;
    const FIntVector Size = YogFogSim::SafeResolution(GridResolution);
    if (RenderState.IsValid() && Size == AllocatedResolution && DensityField && VelocityField) return true;
    ReleaseRenderState();
    TArray<FTextureRenderTargetResource*> Resources;
    auto MakeVolume = [this, Size, &Resources](const TCHAR* Label)
    {
        auto* Texture = NewObject<UTextureRenderTargetVolume>(this,
            MakeUniqueObjectName(this, UTextureRenderTargetVolume::StaticClass(), FName(Label)), RF_Transient);
        Texture->ClearColor = FLinearColor::Black;
        Texture->bSupportsUAV = true;
        Texture->bForceLinearGamma = true;
        Texture->SRGB = false;
        Texture->Init(Size.X, Size.Y, Size.Z, PF_FloatRGBA);
        // Remove its one-shot deferred clear before the simulation can populate it.
        Texture->UpdateResourceImmediate(true);
        Resources.Add(Texture->GameThread_GetRenderTargetResource());
        return Texture;
    };
    DensityField = MakeVolume(TEXT("FogDensityA")); DensityScratch = MakeVolume(TEXT("FogDensityB"));
    VelocityField = MakeVolume(TEXT("FogVelocityA")); VelocityScratch = MakeVolume(TEXT("FogVelocityB"));
    PressureA = MakeVolume(TEXT("FogPressureA")); PressureB = MakeVolume(TEXT("FogPressureB"));
    Divergence = MakeVolume(TEXT("FogDivergence"));
    AllocatedResolution = ActualGridResolution = Size;
    SetDomain(DomainCenter, DomainExtent);
    bResetPending = true;
    RenderState = MakeShared<FYogFogSimulationRenderState, ESPMode::ThreadSafe>();
    const auto State = RenderState;
    // Resource initialization was enqueued by Init before this command. No UObject is touched on RT.
    ENQUEUE_RENDER_COMMAND(YogFogSimInitializeResources)([State, Resources = MoveTemp(Resources)](FRHICommandListImmediate&)
    {
        bool bValid = Resources.Num() == YogFogSim::FieldCount;
        for (int32 Index = 0; Index < Resources.Num(); ++Index)
        {
            State->Textures[Index] = Resources[Index] ? Resources[Index]->GetRenderTargetTexture() : nullptr;
            bValid &= State->Textures[Index].IsValid();
        }
        State->ResourceStatus.Set(bValid ? 1 : 2);
    });
    return true;
}

void UYogFogSimulationComponent::ReleaseRenderState()
{
    if (RenderState)
    {
        const auto State = MoveTemp(RenderState);
        ENQUEUE_RENDER_COMMAND(YogFogSimReleaseResources)([State](FRHICommandListImmediate&)
        {
            for (auto& Texture : State->Textures) Texture.SafeRelease();
            State->ResourceStatus.Set(0);
        });
    }
    AllocatedResolution = FIntVector::ZeroValue;
    bWasSimulationActive = false;
    TimeAccumulator = 0.f;
}

void UYogFogSimulationComponent::ResetSimulation()
{
    SimulationTime = 0.f;
    SubmittedStepCount = 0;
    TimeAccumulator = 0.f;
    bResetPending = true;
    if (EnsureResources()) QueueSimulation(0, true);
}

void UYogFogSimulationComponent::QueueSimulation(int32 StepCount, bool bReset)
{
    using namespace YogFogSim;
    if (!RenderState) return;
    FDispatch Input;
    Input.Size = AllocatedResolution;
    Input.Center = FVector3f(DomainCenter); Input.Extent = FVector3f(DomainExtent);
    Input.Shape = FVector3f(SafeVector(ShapeScaleXYZ, FVector::OneVector, .05f, 20.f));
    CachedShapeScale = FVector(Input.Shape);
    Input.Wind = FVector3f(SafeVector(WindVelocity, FVector(3.f, -1.f, .5f), -200.f, 200.f));
    Input.Curl = Safe(CurlStrength, 15.f, 0.f, 100.f);
    Input.Replenishment = Safe(DensityReplenishment, .04f, 0.f, 1.f);
    Input.Damping = Safe(VelocityDamping, .3f, 0.f, 5.f);
    Input.StartTime = bReset ? 0.f : SimulationTime;
    Input.Steps = FMath::Clamp(StepCount, 0, 2);
    Input.Iterations = FMath::Clamp(PressureIterations, 1, 32);
    Input.bProject = SolverMode == EYogFogSolverMode::ProjectedNS;
    Input.bReset = bReset;
    // Pressure is scaled by the smallest physical cell size. This preserves anisotropic
    // derivatives while keeping pressure representable in half-float persistent textures.
    const double ReferenceCell = FMath::Min3(CellSize.X, CellSize.Y, CellSize.Z);
    Input.Metric = FVector3f(static_cast<float>(ReferenceCell / CellSize.X),
        static_cast<float>(ReferenceCell / CellSize.Y), static_cast<float>(ReferenceCell / CellSize.Z));
    Input.BodyCount = FMath::Min(InteractionBodies.Num(), MaximumBodies);
    for (int32 Index = 0; Index < MaximumBodies; ++Index)
    {
        Input.Bodies[Index] = FVector4f(0, 0, 0, 1);
        Input.BodyVelocities[Index] = FVector4f(0, 0, 0, 1);
        if (Index < static_cast<int32>(Input.BodyCount))
        {
            const auto& Body = InteractionBodies[Index];
            Input.Bodies[Index] = FVector4f(FVector3f(Body.Center), Body.Radius);
            Input.BodyVelocities[Index] = FVector4f(FVector3f(Body.Velocity), Body.HalfHeight);
        }
    }
    const auto State = RenderState;
    ENQUEUE_RENDER_COMMAND(YogFogSimCompute)([State, Input](FRHICommandListImmediate& RHICmdList)
    {
        Dispatch(RHICmdList, *State, Input);
    });
    if (bReset) { SimulationTime = 0.f; SubmittedStepCount = 0; }
    SimulationTime += Input.Steps * FixedStep;
    SubmittedStepCount += Input.Steps;
    bResetPending = false;
}

void UYogFogSimulationComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    UWorld* World = GetWorld();
    const bool bActive = World && !World->bIsTearingDown && bSimulationEnabled && (World->IsGameWorld() || bSimulateInEditor);
    if (!bActive || !EnsureResources())
    {
        TimeAccumulator = 0.f;
        bWasSimulationActive = false;
        return;
    }
    const FVector Shape = YogFogSim::SafeVector(ShapeScaleXYZ, FVector::OneVector, .05f, 20.f);
    if (!CachedShapeScale.Equals(Shape, .0001)) bResetPending = true;
    if (!bWasSimulationActive) TimeAccumulator = 0.f;
    bWasSimulationActive = true;
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f)
    {
        if (bResetPending) QueueSimulation(0, true);
        return;
    }
    // Drop lost time instead of queuing catch-up work after pause, debugger stops or low frame rates.
    TimeAccumulator += FMath::Min(DeltaTime, YogFogSim::FixedStep * 2.f);
    const int32 Steps = FMath::Min(2, FMath::FloorToInt((TimeAccumulator + .000001f) / YogFogSim::FixedStep));
    TimeAccumulator = FMath::Max(0.f, TimeAccumulator - Steps * YogFogSim::FixedStep);
    if (Steps > 0 || bResetPending) QueueSimulation(Steps, bResetPending);
}

FString UYogFogSimulationComponent::GetSimulationStatus() const
{
    const int32 State = RenderState ? RenderState->ResourceStatus.GetValue() : 0;
    const int32 Graphs = RenderState ? RenderState->ExecutedGraphs.GetValue() : 0;
    return FString::Printf(TEXT("solver=%s; enabled=%s; editor=%s\nRT-resources=%s; render-graphs-executed=%d (not GPU readback)\ngrid=%dx%dx%d; cell-cm=(%.2f,%.2f,%.2f)\ncenter=%s; half-extent=%s; shape-stretch=%s\nqueued-time=%.3fs; fixed-steps=%d; bodies=%d/8; pressure-iterations=%d\noutput R=density, G=temporary signed wake; velocity RGB=world cm/s; stable DensityA"),
        SolverMode == EYogFogSolverMode::ProjectedNS ? TEXT("ProjectedNS") : TEXT("Advection"),
        bSimulationEnabled ? TEXT("true") : TEXT("false"), bSimulateInEditor ? TEXT("true") : TEXT("false"),
        State == 1 ? TEXT("ready") : (State == 2 ? TEXT("unavailable") : TEXT("pending/unregistered")), Graphs,
        ActualGridResolution.X, ActualGridResolution.Y, ActualGridResolution.Z, CellSize.X, CellSize.Y, CellSize.Z,
        *ActualDomainCenter.ToString(), *ActualDomainExtent.ToString(), *ShapeScaleXYZ.ToString(),
        SimulationTime, SubmittedStepCount, InteractionBodies.Num(), FMath::Clamp(PressureIterations, 1, 32));
}
