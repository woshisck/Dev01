#include "YogAirFogRenderBridge.h"

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
#include "YogAirFogVolume.h"
#include "YogPlanarFogActor.h"
#include "YogFogPlanarSimulationComponent.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "Engine/Engine.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GlobalShader.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "PostProcess/PostProcessInputs.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RenderingThread.h"
#include "RHIStaticStates.h"
#include "RHITextureReference.h"
#include "SceneInterface.h"
#include "SceneRendering.h"
#include "SceneViewExtension.h"
#include "ScreenPass.h"
#include "ShaderParameterStruct.h"
#include "TextureResource.h"
#include "UObject/StrongObjectPtr.h"
#endif

namespace
{
// Capability reporting only: changing this cannot enable an interface omitted
// from the installed engine. Keep the bridge's existing void lifetime API.
TAutoConsoleVariable<int32> CVarYogNativeAirFogBridgeAvailable(
    TEXT("r.Yog.AirFog.NativeBridgeAvailable"),
    YOG_WITH_NATIVE_AIR_FOG_BRIDGE,
    TEXT("Read-only: exact native fog finish bridge is compiled in (1), or unavailable in this engine build (0). Native volume density and simulation are independent."),
    ECVF_ReadOnly);
}

#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
namespace
{
class FYogAirFogTransmittancePS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FYogAirFogTransmittancePS);
    SHADER_USE_PARAMETER_STRUCT(FYogAirFogTransmittancePS, FGlobalShader);
    class FBrushMapping : SHADER_PERMUTATION_BOOL("YOG_FOG_BRUSH_MAPPING");
    using FPermutationDomain = TShaderPermutationDomain<FBrushMapping>;
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D, IntegratedLightScattering)
        SHADER_PARAMETER_SAMPLER(SamplerState, FogSampler)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)
        SHADER_PARAMETER(FVector2f, DepthInvExtent)
        SHADER_PARAMETER(FVector2f, InputRectMin)
        SHADER_PARAMETER(FVector2f, InputRectSize)
        SHADER_PARAMETER(float, FogStartDistance)
        SHADER_PARAMETER(float, UpsampleJitter)
        SHADER_PARAMETER(uint32, OnlyRenderedOpaque)
        SHADER_PARAMETER(uint32, FilteringQuality)
        SHADER_PARAMETER_TEXTURE(Texture2D, BrushPattern)
        SHADER_PARAMETER_SAMPLER(SamplerState, BrushSampler)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, PlanarField)
        SHADER_PARAMETER_SAMPLER(SamplerState, FieldSampler)
        SHADER_PARAMETER(FMatrix44f, UnjitteredClipToTranslatedWorld)
        SHADER_PARAMETER(FVector4f, SimulationDomain)
        SHADER_PARAMETER(float, BrushStrength)
        SHADER_PARAMETER(float, BrushCoverage)
        SHADER_PARAMETER(float, BrushContrast)
        SHADER_PARAMETER(float, BrushWorldSize)
        SHADER_PARAMETER(float, FlowDisplacement)
        SHADER_PARAMETER(float, FlowTime)
        SHADER_PARAMETER(uint32, BrushSamples)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FYogAirFogTransmittancePS, "/Plugin/YogVision/Private/YogAirFogTransmittance.usf", "MainPS", SF_Pixel);

// Composite before any artist PP. Copying the view rectangle avoids SRV/RT
// aliasing and does not depend on signed-source blending support of SceneColor.
class FYogAirFogBrushCompositePS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FYogAirFogBrushCompositePS);
    SHADER_USE_PARAMETER_STRUCT(FYogAirFogBrushCompositePS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, FogBrushLayer)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColorCopy)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepthTexture)
        SHADER_PARAMETER_SAMPLER(SamplerState, DepthSampler)
        SHADER_PARAMETER(FVector2f, DepthInvExtent)
        SHADER_PARAMETER(FVector2f, SceneColorInvExtent)
        SHADER_PARAMETER(FVector2f, BrushExtent)
        SHADER_PARAMETER(FVector2f, InputRectMin)
        SHADER_PARAMETER(FVector2f, InputRectSize)
        SHADER_PARAMETER(float, FogStartDistance)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FYogAirFogBrushCompositePS, "/Plugin/YogVision/Private/YogAirFogTransmittance.usf", "CompositePS", SF_Pixel);

// View state and stereo index identify one persistent camera; Scene separates
// editor/PIE worlds. Stateless captures are deliberately left untouched.
struct FViewKey
{
    const FSceneViewStateInterface* State = nullptr;
    const FSceneInterface* Scene = nullptr;
    int32 StereoIndex = INDEX_NONE;
    bool operator==(const FViewKey& Other) const
    {
        return State == Other.State && Scene == Other.Scene && StereoIndex == Other.StereoIndex;
    }
    friend uint32 GetTypeHash(const FViewKey& Key)
    {
        return HashCombine(HashCombine(PointerHash(Key.State), PointerHash(Key.Scene)), ::GetTypeHash(Key.StereoIndex));
    }
};
FViewKey KeyFor(const FSceneView& View)
{
    return {View.State, View.Family ? View.Family->Scene : nullptr, View.StereoViewIndex};
}
struct FGameView
{
    // Keep the owner independently of the actor: editor map unload need not call
    // Destroyed(), and a stale actor weak pointer must not hide its world's MIDs.
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> Volume;
    TWeakObjectPtr<UMaterialInstanceDynamic> Source;
    TStrongObjectPtr<UTextureRenderTarget2D> Target;
    TStrongObjectPtr<UTextureRenderTarget2D> BrushTarget;
    TStrongObjectPtr<UTexture2D> BrushPattern;
    TStrongObjectPtr<UTextureRenderTarget2D> Field;
    TStrongObjectPtr<UMaterialInstanceDynamic> Material;
    FIntPoint Extent = FIntPoint::ZeroValue;
    uint64 LastFrame = 0;
};
struct FRenderView
{
    FTextureRenderTargetResource* Resource = nullptr;
    FTextureRenderTargetResource* BrushResource = nullptr;
    FTextureRenderTargetResource* FieldResource = nullptr;
    FTextureReferenceRHIRef BrushReference;
    FVector4f SimulationDomain = FVector4f(0, 0, 1, 1);
    float BrushStrength = 0;
    float BrushCoverage = 0;
    float BrushContrast = 1;
    float BrushWorldSize = 900;
    float FlowDisplacement = 0;
    float FlowTime = 0;
    uint32 BrushSamples = 8;
};

class FYogAirFogViewExtension : public FSceneViewExtensionBase
{
public:
    explicit FYogAirFogViewExtension(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {}

    void Register(AYogAirFogVolume* Volume)
    {
        check(IsInGameThread());
        if (!CanUseWorld(Volume->GetWorld())) return;
        Volumes.AddUnique(Volume);
    }
    void RegisterPlanar(AYogPlanarFogActor* Volume)
    {
        check(IsInGameThread());
        if (!CanUseWorld(Volume->GetWorld())) return;
        PlanarVolumes.AddUnique(Volume);
    }
    void UnregisterPlanar(AYogPlanarFogActor* Volume)
    {
        check(IsInGameThread());
        PlanarVolumes.Remove(Volume);
        for (auto It = GameViews.CreateIterator(); It; ++It)
            if (!It.Value().Volume.IsValid() || It.Value().Volume.Get() == Volume)
            {
                ReleaseViewResources(It.Key(), It.Value());
                It.RemoveCurrent();
            }
        if (Volume)
        {
            Volume->SetLatestFogTransport(nullptr);
            Volume->SetLatestFogBrushLayer(nullptr);
        }
    }
    void Unregister(AYogAirFogVolume* Volume)
    {
        check(IsInGameThread());
        Volumes.Remove(Volume);
        for (auto It = GameViews.CreateIterator(); It; ++It)
            if (!It.Value().Volume.IsValid() || It.Value().Volume.Get() == Volume)
            {
                ReleaseViewResources(It.Key(), It.Value());
                It.RemoveCurrent();
            }
    }
    void CleanupWorld(UWorld* World)
    {
        check(IsInGameThread());
        if (!World) return;
        Volumes.RemoveAll([World](const TWeakObjectPtr<AYogAirFogVolume>& Entry)
        {
            const AYogAirFogVolume* Volume = Entry.Get();
            return !Volume || Volume->GetWorld() == World;
        });
        PlanarVolumes.RemoveAll([World](const TWeakObjectPtr<AYogPlanarFogActor>& Entry)
        {
            AYogPlanarFogActor* Volume = Entry.Get();
            if (!Volume) return true;
            if (Volume->GetWorld() != World) return false;
            Volume->SetLatestFogTransport(nullptr);
            Volume->SetLatestFogBrushLayer(nullptr);
            return true;
        });

        TArray<FViewKey> RemovedViews;
        for (auto& Pair : GameViews)
            if (Pair.Value.World.Get() == World)
            {
                ReleaseViewResources(Pair.Key, Pair.Value);
                RemovedViews.Add(Pair.Key);
            }
        if (!RemovedViews.IsEmpty())
        {
            // The RT addresses and material render proxies remain owned until
            // earlier draws, parameter clears and map removals have completed.
            FlushRenderingCommands();
            for (const FViewKey& Key : RemovedViews) GameViews.Remove(Key);
        }
    }
    void Clear()
    {
        check(IsInGameThread());
        Volumes.Reset();
        PlanarVolumes.Reset();
        for (auto& Pair : GameViews) ReleaseViewResources(Pair.Key, Pair.Value);
        FlushRenderingCommands();
        GameViews.Reset();
    }

    virtual void SetupView(FSceneViewFamily& Family, FSceneView& View) override
    {
        check(IsInGameThread());
        if (!View.State || !Family.Scene || View.bIsSceneCapture || View.bIsReflectionCapture
            || !Family.EngineShowFlags.PostProcessing || View.GetFeatureLevel() < ERHIFeatureLevel::SM5) return;
        UWorld* World = Family.Scene->GetWorld();
        if (!CanUseWorld(World)) return;
        AYogAirFogVolume* Volume = nullptr;
        for (int32 Index = Volumes.Num() - 1; Index >= 0; --Index)
        {
            AYogAirFogVolume* Candidate = Volumes[Index].Get();
            if (!Candidate) { Volumes.RemoveAtSwap(Index); continue; }
            if (Candidate->GetWorld() == Family.Scene->GetWorld() && Candidate->IsAirFogEnabled() && Candidate->GetFinishMID())
                Volume = Candidate;
        }
        AYogPlanarFogActor* Planar=nullptr;
        for(int32 Index=PlanarVolumes.Num()-1;Index>=0;--Index)
        {
            auto* Candidate=PlanarVolumes[Index].Get();
            if(!Candidate){PlanarVolumes.RemoveAtSwap(Index);continue;}
            if(Candidate->GetWorld()==Family.Scene->GetWorld()&&Candidate->bMistEnabled)Planar=Candidate;
        }
        if (!Volume&&!Planar) return;

        for (auto It = GameViews.CreateIterator(); It; ++It)
            if (!It.Value().Volume.IsValid() || GFrameCounter > It.Value().LastFrame + 120)
            {
                ReleaseViewResources(It.Key(), It.Value());
                It.RemoveCurrent();
            }

        const FViewKey Key = KeyFor(View);
        const FIntPoint Extent = Planar?FIntPoint::DivideAndRoundUp(View.UnscaledViewRect.Size(),2):View.UnscaledViewRect.Size();
        if (Extent.GetMin() <= 0) return;
        FGameView& State = GameViews.FindOrAdd(Key);
        State.World = World;
        if (AYogPlanarFogActor* Previous = Cast<AYogPlanarFogActor>(State.Volume.Get()); Previous && Previous != Planar)
        {
            if (Previous->FogTransport == State.Target.Get()) Previous->SetLatestFogTransport(nullptr);
            if (Previous->FogBrushLayer == State.BrushTarget.Get()) Previous->SetLatestFogBrushLayer(nullptr);
        }
        UMaterialInstanceDynamic* Parent = Planar ? Planar->GetPlanarFinishMID() : Volume->GetFinishMID();
        if (!State.Target.IsValid() || State.Extent != Extent || (Parent&&(!State.Material.IsValid()||State.Source.Get()!=Parent)))
        {
            // Replace rather than resize an in-flight RHI texture. Old UObject
            // resource release is queued after the earlier view's render work.
            auto* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
            Target->ClearColor = FLinearColor(0.f, 0.f, 0.f, 1.f);
            Target->AddressX = TA_Clamp;
            Target->AddressY = TA_Clamp;
            Target->Filter = TF_Bilinear;
            Target->bAutoGenerateMips = false;
            Target->InitCustomFormat(Extent.X, Extent.Y, PF_FloatRGBA, true);
            Target->UpdateResourceImmediate(true);
            // UE's view-state reusable MID rejects a MID -> MID parent chain.
            // Use the generated material parent, then copy the live source below.
            auto* Material = Parent?UMaterialInstanceDynamic::Create(Parent->Parent, GetTransientPackage()):nullptr;
            if(Material)Material->SetFlags(RF_Transient);
            State.Target.Reset(Target);
            State.BrushTarget.Reset();
            State.Material.Reset(Material);
            State.Extent = Extent;
        }
        if (Planar && !State.BrushTarget.IsValid())
        {
            auto* Target = NewObject<UTextureRenderTarget2D>(GetTransientPackage(), NAME_None, RF_Transient);
            Target->ClearColor = FLinearColor::Transparent;
            Target->AddressX = TA_Clamp;
            Target->AddressY = TA_Clamp;
            Target->Filter = TF_Bilinear;
            Target->bAutoGenerateMips = false;
            Target->InitCustomFormat(Extent.X, Extent.Y, PF_FloatRGBA, true);
            Target->UpdateResourceImmediate(true);
            State.BrushTarget.Reset(Target);
        }
        else if (!Planar) State.BrushTarget.Reset();
        State.Volume = Planar?static_cast<AActor*>(Planar):static_cast<AActor*>(Volume);
        State.Source = Parent;
        State.LastFrame = GFrameCounter;
        if(Parent&&State.Material.IsValid())
        {
            State.Material->CopyInterpParameters(Parent);
            State.Material->SetTextureParameterValue(TEXT("AirFogTransmittance"), State.Target.Get());
        }
        FRenderView Snapshot;
        Snapshot.Resource = State.Target->GameThread_GetRenderTargetResource();
        State.BrushPattern.Reset();
        State.Field.Reset();
        if (Planar)
        {
            Planar->SetLatestFogTransport(State.Target.Get());
            Planar->SetLatestFogBrushLayer(State.BrushTarget.Get());
            Snapshot.BrushResource = State.BrushTarget->GameThread_GetRenderTargetResource();
            const UYogFogPlanarSimulationComponent* Simulation = Planar->PlanarSimulation;
            if (Planar->BrushStrength > 0 && Planar->BrushCoverage > 0 && IsValid(Planar->BrushPattern)
                && Simulation && IsValid(Simulation->PlanarField) && Planar->BrushPattern->GetResource())
            {
                State.BrushPattern.Reset(Planar->BrushPattern);
                State.Field.Reset(Simulation->PlanarField);
                Snapshot.BrushReference = Planar->BrushPattern->TextureReference.TextureReferenceRHI;
                Snapshot.FieldResource = Simulation->PlanarField->GameThread_GetRenderTargetResource();
                const FVector Center = Simulation->ActualDomainCenter;
                const FVector2D HalfExtent = Simulation->ActualDomainHalfExtent;
                Snapshot.SimulationDomain = FVector4f(Center.X, Center.Y, FMath::Max(HalfExtent.X, 1.), FMath::Max(HalfExtent.Y, 1.));
                const auto Safe = [](float Value, float Fallback, float Min, float Max)
                { return FMath::Clamp(FMath::IsFinite(Value) ? Value : Fallback, Min, Max); };
                Snapshot.BrushStrength = Safe(Planar->BrushStrength, 0, 0, 1);
                Snapshot.BrushCoverage = Safe(Planar->BrushCoverage, 0, 0, 1);
                Snapshot.BrushContrast = Safe(Planar->BrushContrast, 1, .1f, 4);
                Snapshot.BrushWorldSize = Safe(Planar->BrushWorldSize, 900, 10, 100000);
                Snapshot.FlowDisplacement = Safe(Planar->FlowDisplacement, 0, 0, 100);
                Snapshot.FlowTime = FMath::IsFinite(Simulation->SimulationTime) ? Simulation->SimulationTime : 0;
                Snapshot.BrushSamples = FMath::Clamp(Planar->FogBrushSamples, 4, 24);
            }
        }
        const auto Self = StaticCastSharedRef<FYogAirFogViewExtension>(AsShared());
        ENQUEUE_RENDER_COMMAND(YogAirFogSetViewTarget)([Self, Key, Snapshot](FRHICommandListImmediate&)
        {
            Self->RenderViews.Add(Key, Snapshot);
        });
        // SetupView follows EndFinalPostprocessSettings in both the editor and
        // LocalPlayer. This appends to the same priority-sorted native PP chain;
        // the artist's priority-0 pass still precedes the finish's priority 60.
        // GetReusableMID copies this MID's texture override with its interp data.
        if(Parent&&State.Material.IsValid())State.Material->OverrideBlendableSettings(View, 1.f);
    }

    virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override
    {
        check(IsInRenderingThread());
        if (!InView.Family || !InView.Family->EngineShowFlags.PostProcessing) return;
        const FRenderView* State = RenderViews.Find(KeyFor(InView));
        if (!State || !State->Resource || !InView.bIsViewInfo) return;
        FRHITexture* Texture = State->Resource->GetRenderTargetTexture();
        if (!Texture) return;
        FRDGTextureRef Output = RegisterExternalTexture(GraphBuilder, Texture, TEXT("YogAirFog.Transmittance"));
        FRHITexture* BrushTexture = State->BrushResource ? State->BrushResource->GetRenderTargetTexture() : nullptr;
        FRDGTextureRef BrushOutput = BrushTexture ? RegisterExternalTexture(GraphBuilder, BrushTexture, TEXT("YogAirFog.BrushLayer")) : nullptr;
        const FViewInfo& View = static_cast<const FViewInfo&>(InView);
        FRDGTextureRef Integrated = View.VolumetricFogResources.IntegratedLightScatteringTexture;
        FRDGTextureRef Depth = Inputs.SceneTextures ? Inputs.SceneTextures->GetParameters()->SceneDepthTexture : nullptr;
        if (!Integrated || !Depth || !View.ViewUniformBuffer)
        {
            AddClearRenderTargetPass(GraphBuilder, Output, FLinearColor(0.f, 0.f, 0.f, 1.f));
            if (BrushOutput)
            {
                AddClearRenderTargetPass(GraphBuilder, BrushOutput, FLinearColor::Transparent);
                GraphBuilder.UseExternalAccessMode(BrushOutput, ERHIAccess::SRVGraphics);
            }
            GraphBuilder.UseExternalAccessMode(Output, ERHIAccess::SRVGraphics);
            return;
        }
        static IConsoleVariable* FilterQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.Filtering.Quality"));
        static IConsoleVariable* Jitter = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.UpsampleJitterMultiplier"));
        static IConsoleVariable* GridPixelSize = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VolumetricFog.GridPixelSize"));
        FRHITexture* Pattern = State->BrushReference.IsValid() ? State->BrushReference->GetReferencedTexture() : nullptr;
        FRHITexture* Field = State->FieldResource ? State->FieldResource->GetRenderTargetTexture() : nullptr;
        const bool bMapBrush = BrushOutput && Pattern && Field && State->BrushStrength > 0 && State->BrushCoverage > 0;
        auto* Parameters = GraphBuilder.AllocParameters<FYogAirFogTransmittancePS::FParameters>();
        Parameters->View = View.ViewUniformBuffer;
        Parameters->IntegratedLightScattering = Integrated;
        Parameters->FogSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
        Parameters->SceneDepthTexture = Depth;
        Parameters->DepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
        Parameters->DepthInvExtent = FVector2f(1.f / Depth->Desc.Extent.X, 1.f / Depth->Desc.Extent.Y);
        Parameters->InputRectMin = FVector2f(View.ViewRect.Min);
        Parameters->InputRectSize = FVector2f(View.ViewRect.Size());
        Parameters->FogStartDistance = View.VolumetricFogStartDistance;
        Parameters->UpsampleJitter = (Jitter ? Jitter->GetFloat() : 0.f) * (GridPixelSize ? GridPixelSize->GetInt() : 8);
        Parameters->OnlyRenderedOpaque = View.bFogOnlyOnRenderedOpaque ? 1u : 0u;
        Parameters->FilteringQuality = FilterQuality ? FMath::Clamp(FilterQuality->GetInt(), 0, 2) : 0;
        if (bMapBrush)
        {
            Parameters->BrushPattern = Pattern;
            Parameters->BrushSampler = TStaticSamplerState<SF_Bilinear, AM_Wrap, AM_Wrap, AM_Wrap>::GetRHI();
            Parameters->PlanarField = RegisterExternalTexture(GraphBuilder, Field, TEXT("YogAirFog.BrushFlow"));
            Parameters->FieldSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
            Parameters->UnjitteredClipToTranslatedWorld = FMatrix44f(View.ViewMatrices.ComputeInvProjectionNoAAMatrix() * View.ViewMatrices.GetTranslatedViewMatrix().GetTransposed());
            Parameters->SimulationDomain = State->SimulationDomain;
            Parameters->BrushStrength = State->BrushStrength;
            Parameters->BrushCoverage = State->BrushCoverage;
            Parameters->BrushContrast = State->BrushContrast;
            Parameters->BrushWorldSize = State->BrushWorldSize;
            Parameters->FlowDisplacement = State->FlowDisplacement;
            Parameters->FlowTime = State->FlowTime;
            Parameters->BrushSamples = State->BrushSamples;
            Parameters->RenderTargets[1] = FRenderTargetBinding(BrushOutput, ERenderTargetLoadAction::ENoAction);
        }
        else if (BrushOutput) AddClearRenderTargetPass(GraphBuilder, BrushOutput, FLinearColor::Transparent);
        Parameters->RenderTargets[0] = FRenderTargetBinding(Output, ERenderTargetLoadAction::ENoAction);
        const FScreenPassTextureViewport Viewport(Output);
        FYogAirFogTransmittancePS::FPermutationDomain Permutation;
        Permutation.Set<FYogAirFogTransmittancePS::FBrushMapping>(bMapBrush);
        TShaderMapRef<FYogAirFogTransmittancePS> Shader(View.ShaderMap, Permutation);
        ClearUnusedGraphResources(Shader, Parameters);
        AddDrawScreenPass(GraphBuilder, RDG_EVENT_NAME("YogAirFog.NativeTransmittance"), View, Viewport, Viewport, Shader, Parameters);
        FRDGTextureRef SceneColor = Inputs.SceneTextures->GetParameters()->SceneColorTexture;
        if (bMapBrush && SceneColor && SceneColor->Desc.NumSamples == 1)
        {
            FRDGTextureDesc CopyDesc = SceneColor->Desc;
            CopyDesc.Flags |= TexCreate_ShaderResource;
            FRDGTextureRef SceneCopy = GraphBuilder.CreateTexture(CopyDesc, TEXT("YogAirFog.UnstyledSceneCopy"));
            FRHICopyTextureInfo CopyInfo;
            CopyInfo.SourcePosition = FIntVector(View.ViewRect.Min.X, View.ViewRect.Min.Y, 0);
            CopyInfo.DestPosition = CopyInfo.SourcePosition;
            CopyInfo.Size = FIntVector(View.ViewRect.Width(), View.ViewRect.Height(), 1);
            AddCopyTexturePass(GraphBuilder, SceneColor, SceneCopy, CopyInfo);

            auto* Composite = GraphBuilder.AllocParameters<FYogAirFogBrushCompositePS::FParameters>();
            Composite->View = View.ViewUniformBuffer;
            Composite->FogBrushLayer = BrushOutput;
            Composite->SceneColorCopy = SceneCopy;
            Composite->SceneDepthTexture = Depth;
            Composite->DepthSampler = TStaticSamplerState<SF_Point, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
            Composite->DepthInvExtent = Parameters->DepthInvExtent;
            Composite->SceneColorInvExtent = FVector2f(1.f / SceneColor->Desc.Extent.X, 1.f / SceneColor->Desc.Extent.Y);
            Composite->BrushExtent = FVector2f(BrushOutput->Desc.Extent);
            Composite->InputRectMin = Parameters->InputRectMin;
            Composite->InputRectSize = Parameters->InputRectSize;
            Composite->FogStartDistance = Parameters->FogStartDistance;
            Composite->RenderTargets[0] = FRenderTargetBinding(SceneColor, ERenderTargetLoadAction::ELoad);
            TShaderMapRef<FYogAirFogBrushCompositePS> CompositeShader(View.ShaderMap);
            AddDrawScreenPass(GraphBuilder, RDG_EVENT_NAME("YogAirFog.BrushOnlyComposite"), View,
                FScreenPassTextureViewport(SceneColor, View.ViewRect), FScreenPassTextureViewport(BrushOutput), CompositeShader, Composite);
        }
        // The later material samples this UObject texture through its uniform
        // expressions, outside RDG's explicit bindings. Export its read state
        // now, not merely at graph end, so the write -> sample barrier is real.
        GraphBuilder.UseExternalAccessMode(Output, ERHIAccess::SRVGraphics);
        if (BrushOutput) GraphBuilder.UseExternalAccessMode(BrushOutput, ERHIAccess::SRVGraphics);
        // RDG texture references stay local to this frame. Only the external
        // render target's resource address is retained by RenderViews.
    }

private:
    static bool CanUseWorld(const UWorld* World)
    {
        return World && !World->bIsTearingDown && !World->IsCleanedUp();
    }
    void ReleaseViewResources(const FViewKey& Key, FGameView& State)
    {
        check(IsInGameThread());
        RemoveRenderView(Key);
        // CopyInterpParameters copied world-owned simulation textures. Clear
        // this bridge-owned MID before dropping its strong reference so those
        // textures cannot retain their component/actor/old World through GC.
        if (State.Material.IsValid()) State.Material->ClearParameterValues();
        if (AYogPlanarFogActor* Planar = Cast<AYogPlanarFogActor>(State.Volume.Get()))
        {
            if (Planar->FogTransport == State.Target.Get()) Planar->SetLatestFogTransport(nullptr);
            if (Planar->FogBrushLayer == State.BrushTarget.Get()) Planar->SetLatestFogBrushLayer(nullptr);
        }
    }
    void RemoveRenderView(const FViewKey& Key)
    {
        const auto Self = StaticCastSharedRef<FYogAirFogViewExtension>(AsShared());
        ENQUEUE_RENDER_COMMAND(YogAirFogRemoveViewTarget)([Self, Key](FRHICommandListImmediate&)
        {
            Self->RenderViews.Remove(Key);
        });
    }
    TArray<TWeakObjectPtr<AYogAirFogVolume>> Volumes; // Game thread only.
    TArray<TWeakObjectPtr<AYogPlanarFogActor>> PlanarVolumes;
    TMap<FViewKey, FGameView> GameViews; // Strong UObject owners, game thread only.
    TMap<FViewKey, FRenderView> RenderViews; // Render thread only.
};
TSharedPtr<FYogAirFogViewExtension, ESPMode::ThreadSafe> Extension;
FDelegateHandle WorldCleanupHandle;
}
#endif

namespace YogAirFogRenderBridge
{
void RegisterPlanarVolume(AYogPlanarFogActor* Volume)
{
    check(IsInGameThread());
#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
    if(!IsValid(Volume)||!GEngine||!GEngine->ViewExtensions||IsRunningDedicatedServer()||GUsingNullRHI)return;
    if(!Extension)Extension=FSceneViewExtensions::NewExtension<FYogAirFogViewExtension>();
    Extension->RegisterPlanar(Volume);
#else
    (void)Volume;
#endif
}
void UnregisterPlanarVolume(AYogPlanarFogActor* Volume)
{
    check(IsInGameThread());
#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
    if(Extension)Extension->UnregisterPlanar(Volume);
#else
    (void)Volume;
#endif
}
void Startup()
{
    check(IsInGameThread());
    if (const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("YogVision")))
        if (!AllShaderSourceDirectoryMappings().Contains(TEXT("/Plugin/YogVision")))
            AddShaderSourceDirectoryMapping(TEXT("/Plugin/YogVision"), FPaths::Combine(Plugin->GetBaseDir(), TEXT("Shaders")));
#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
    if (!WorldCleanupHandle.IsValid())
        WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda([](UWorld* World, bool, bool)
        {
            if (Extension) Extension->CleanupWorld(World);
        });
#endif
}
void Shutdown()
{
    check(IsInGameThread());
#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
    FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
    WorldCleanupHandle.Reset();
    if (Extension)
    {
        Extension->Clear();
        Extension.Reset();
    }
#endif
}
void RegisterVolume(AYogAirFogVolume* Volume)
{
    check(IsInGameThread());
#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
    if (!IsValid(Volume) || !GEngine || !GEngine->ViewExtensions || IsRunningDedicatedServer() || GUsingNullRHI) return;
    if (!Extension) Extension = FSceneViewExtensions::NewExtension<FYogAirFogViewExtension>();
    Extension->Register(Volume);
#else
    if (!Volume) return;
    static bool bReportedUnavailable = false;
    if (!bReportedUnavailable)
    {
        bReportedUnavailable = true;
        UE_LOG(LogTemp, Warning, TEXT("YogVision: exact native fog finish bridge is unavailable because this engine omits Renderer/Private/SceneRendering.h. No finish pass was registered; native volume density and simulation remain available."));
    }
#endif
}
void UnregisterVolume(AYogAirFogVolume* Volume)
{
    check(IsInGameThread());
#if YOG_WITH_NATIVE_AIR_FOG_BRIDGE
    if (Extension) Extension->Unregister(Volume);
#else
    (void)Volume;
#endif
}
}
