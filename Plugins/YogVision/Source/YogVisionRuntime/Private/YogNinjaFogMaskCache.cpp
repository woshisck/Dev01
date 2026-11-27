#include "YogNinjaFogMaskCache.h"

#include "DataDrivenShaderPlatformInfo.h"
#include "EngineModule.h"
#include "Engine/Engine.h"
#include "GlobalDistanceFieldParameters.h"
#include "GlobalShader.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RendererInterface.h"
#include "RenderingThread.h"
#include "SceneInterface.h"
#include "SceneView.h"
#include "SceneViewExtension.h"
#include "ShaderParameterStruct.h"
#include "TextureResource.h"
#include "RHIStaticStates.h"

namespace YogNinjaFogMaskCache
{
namespace
{
class FEnvironmentCacheCS : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FEnvironmentCacheCS);
    SHADER_USE_PARAMETER_STRUCT(FEnvironmentCacheCS,FGlobalShader);
    class FPlayerWake : SHADER_PERMUTATION_BOOL("YOG_PLAYER_WAKE");
    using FPermutationDomain = TShaderPermutationDomain<FPlayerWake>;
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters,)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D, WakeDensity)
        SHADER_PARAMETER_SAMPLER(SamplerState, WakeSampler)
        SHADER_PARAMETER(FVector3f, WakeMin)
        SHADER_PARAMETER(FVector3f, WakeInvSize)
        SHADER_PARAMETER(float, WakeStrength)
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters,View)
        SHADER_PARAMETER(FVector3f,DomainMin)
        SHADER_PARAMETER(FVector3f,DomainSize)
        SHADER_PARAMETER(FVector3f,TranslatedMin)
        SHADER_PARAMETER(uint32,Resolution)
        SHADER_PARAMETER(uint32,SliceCount)
        SHADER_PARAMETER(uint32,ColumnCount)
        SHADER_PARAMETER(uint32,GDFReady)
        SHADER_PARAMETER(float,GroundHeight)
        SHADER_PARAMETER(float,BodyHeight)
        SHADER_PARAMETER(float,GroundAmount)
        SHADER_PARAMETER(float,UpperAmount)
        SHADER_PARAMETER(float,SurfaceFade)
        SHADER_PARAMETER(float,SurfaceBand)
        SHADER_PARAMETER(float,ContactBoost)
        SHADER_PARAMETER(uint32,HeightEnabled)
        SHADER_PARAMETER(float,NearHeight)
        SHADER_PARAMETER(float,FarHeight)
        SHADER_PARAMETER(float,WallHeight)
        SHADER_PARAMETER(float,HeightFeather)
        SHADER_PARAMETER(float,WallRange)
        SHADER_PARAMETER(float,PlayerStrength)
        SHADER_PARAMETER(float,PlayerFeather)
        SHADER_PARAMETER(float,PlayerHeight)
        SHADER_PARAMETER(float,LightStrength)
        SHADER_PARAMETER(float,LightFeather)
        SHADER_PARAMETER(FVector4f,Observer)
        SHADER_PARAMETER_ARRAY(FVector4f,Lights,[4])
        SHADER_PARAMETER_ARRAY(FVector4f,Directions,[4])
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>,OutputMask)
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float4>,OutputVolumeMask)
    END_SHADER_PARAMETER_STRUCT()

    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {
        return IsFeatureLevelSupported(Parameters.Platform,ERHIFeatureLevel::SM5);
    }
};
IMPLEMENT_GLOBAL_SHADER(FEnvironmentCacheCS,"/Plugin/YogVision/Private/YogNinjaFogMaskCache.usf","MainCS",SF_Compute);

struct FEntry
{
    FInput Input;
    TSharedPtr<FState,ESPMode::ThreadSafe> State;
    uint64 LastRevision=MAX_uint64;
};

class FMaskCacheViewExtension : public FSceneViewExtensionBase
{
public:
    explicit FMaskCacheViewExtension(const FAutoRegister& AutoRegister)
        : FSceneViewExtensionBase(AutoRegister) {}

    // Only touched on the render thread, including Submit/Remove commands.
    TMap<uint64,FEntry> Entries;

    virtual void SetupViewFamily(FSceneViewFamily&) override {}
    virtual void SetupView(FSceneViewFamily&,FSceneView&) override {}
    virtual void BeginRenderViewFamily(FSceneViewFamily&) override {}

    virtual void PostRenderBasePassDeferred_RenderThread(FRDGBuilder& GraphBuilder,
        FSceneView& InView,const FRenderTargetBindingSlots&,
        TRDGUniformBufferRef<FSceneTextureUniformParameters>) override
    {
        check(IsInRenderingThread());
        if (!InView.Family || !InView.Family->Scene || !InView.ViewUniformBuffer
            || InView.bIsSceneCapture || InView.bIsReflectionCapture
            || InView.GetFeatureLevel()<ERHIFeatureLevel::SM5) return;

        // The real scene renderer updates GDF before base pass. Canvas/DrawRT
        // temporary views do not provide this scene field and are not used.
        const FGlobalDistanceFieldParameterData* GDF=InView.bIsViewInfo
            ? GetRendererModule().GetGlobalDistanceFieldParameterData(InView) : nullptr;
        const bool bValidGDF=GDF && GDF->NumGlobalSDFClipmaps>0;

        for (auto& Pair:Entries)
        {
            FEntry& Entry=Pair.Value;
            const FInput& Input=Entry.Input;
            if (!Entry.State.IsValid() || Input.Scene!=InView.Family->Scene
                || !Input.Target.IsValid() || !Input.VolumeTarget.IsValid()
                || Entry.LastRevision==Input.Revision) continue;
            const uint32 Resolution=FMath::Clamp(Input.Resolution,16u,512u);
            const uint32 Slices=FMath::Clamp(Input.Slices,2u,64u);
            const uint32 Columns=FMath::Clamp(Input.Columns,1u,16u);
            const FIntPoint Extent(Resolution*Columns,Resolution*FMath::DivideAndRoundUp(Slices,Columns));
            if (Input.Target->GetDesc().Extent!=Extent
                || !EnumHasAnyFlags(Input.Target->GetDesc().Flags,ETextureCreateFlags::UAV)
                || !Input.VolumeTarget->GetDesc().IsTexture3D()
                || Input.VolumeTarget->GetSizeXYZ()!=FIntVector(Resolution,Resolution,Slices)
                || !EnumHasAnyFlags(Input.VolumeTarget->GetDesc().Flags,ETextureCreateFlags::UAV)) continue;

            FRDGTextureRef Output=RegisterExternalTexture(GraphBuilder,Input.Target.GetReference(),TEXT("YogNinjaFog.EnvironmentMask"));
            FRDGTextureRef VolumeOutput=RegisterExternalTexture(GraphBuilder,Input.VolumeTarget.GetReference(),TEXT("YogNinjaFog.EnvironmentMaskVolume"));
            auto* Parameters=GraphBuilder.AllocParameters<FEnvironmentCacheCS::FParameters>();
            Parameters->View=InView.ViewUniformBuffer;
            Parameters->DomainMin=Input.Min;
            Parameters->DomainSize=Input.Size;
            // Add in CPU double precision before demotion. Each shader voxel
            // then adds only a small domain offset to translated-world origin.
            Parameters->TranslatedMin=FVector3f(FVector(Input.Min)+InView.ViewMatrices.GetPreViewTranslation());
            Parameters->Resolution=Resolution;
            Parameters->SliceCount=Slices;
            Parameters->ColumnCount=Columns;
            Parameters->GDFReady=bValidGDF?1u:0u;
            Parameters->GroundHeight=Input.GroundHeight;
            Parameters->BodyHeight=Input.BodyHeight;
            Parameters->GroundAmount=Input.GroundAmount;
            Parameters->UpperAmount=Input.UpperAmount;
            Parameters->SurfaceFade=Input.SurfaceFade;
            Parameters->SurfaceBand=Input.SurfaceBand;
            Parameters->ContactBoost=Input.ContactBoost;
            Parameters->HeightEnabled=Input.HeightEnabled;
            Parameters->NearHeight=Input.NearHeight;
            Parameters->FarHeight=Input.FarHeight;
            Parameters->WallHeight=Input.WallHeight;
            Parameters->HeightFeather=Input.HeightFeather;
            Parameters->WallRange=Input.WallRange;
            Parameters->PlayerStrength=Input.PlayerStrength;
            Parameters->PlayerFeather=Input.PlayerFeather;
            Parameters->PlayerHeight=Input.PlayerHeight;
            Parameters->LightStrength=Input.LightStrength;
            Parameters->LightFeather=Input.LightFeather;
            Parameters->Observer=Input.Observer;
            for (int32 Index=0;Index<4;++Index)
            {
                Parameters->Lights[Index]=Input.Lights[Index];
                Parameters->Directions[Index]=Input.Directions[Index];
            }
            Parameters->OutputMask=GraphBuilder.CreateUAV(Output);
            Parameters->OutputVolumeMask=GraphBuilder.CreateUAV(VolumeOutput);
            const bool bUseWake = Input.WakeStrength > 0.f && Input.WakeDensity.IsValid();
            FEnvironmentCacheCS::FPermutationDomain Permutation;
            Permutation.Set<FEnvironmentCacheCS::FPlayerWake>(bUseWake);
            if (bUseWake)
            {
                Parameters->WakeDensity = RegisterExternalTexture(GraphBuilder, Input.WakeDensity.GetReference(), TEXT("YogNinjaFog.WakeDensity"));
                Parameters->WakeSampler = TStaticSamplerState<SF_Bilinear, AM_Clamp, AM_Clamp, AM_Clamp>::GetRHI();
                Parameters->WakeMin = Input.WakeMin;
                Parameters->WakeInvSize = Input.WakeInvSize;
                Parameters->WakeStrength = Input.WakeStrength;
            }
            TShaderMapRef<FEnvironmentCacheCS> Shader(GetGlobalShaderMap(InView.GetFeatureLevel()), Permutation);
            FComputeShaderUtils::AddPass(GraphBuilder,RDG_EVENT_NAME("YogNinjaFog.CacheEnvironment %ux%ux%u",Resolution,Resolution,Slices),
                Shader,Parameters,FComputeShaderUtils::GetGroupCount(Extent,FIntPoint(8,8)));
            // The later translucent material reads the external RHI texture,
            // outside RDG's explicit SRV declarations. Publish its transition.
            GraphBuilder.UseExternalAccessMode(Output,ERHIAccess::SRVMask);
            GraphBuilder.UseExternalAccessMode(VolumeOutput,ERHIAccess::SRVMask);
            Entry.LastRevision=Input.Revision;
            Entry.State->ValidGDF.Set(bValidGDF?1:0);
            Entry.State->CompletedRevision.Set(static_cast<int32>(Input.Revision));
            Entry.State->Executed.Increment();
        }
    }
};

TSharedPtr<FMaskCacheViewExtension,ESPMode::ThreadSafe> Extension;
}

void Startup()
{
    check(IsInGameThread());
    // The module loads at PostConfigInit for global shader registration. GEngine
    // and its view-extension registry do not exist yet; Submit retries lazily.
    if (!IsRunningCommandlet() && GEngine && GEngine->ViewExtensions && !Extension.IsValid())
        Extension=FSceneViewExtensions::NewExtension<FMaskCacheViewExtension>();
}

void Shutdown()
{
    check(IsInGameThread());
    if (!Extension.IsValid()) return;
    const auto Held=Extension;
    ENQUEUE_RENDER_COMMAND(YogNinjaFogMaskClear)([Held](FRHICommandListImmediate&)
    {
        Held->Entries.Reset();
    });
    FlushRenderingCommands();
    Extension.Reset();
}

void Submit(uint64 Key,const FInput& Input,TSharedRef<FState,ESPMode::ThreadSafe> State,
    FTextureRenderTargetResource* WakeResource)
{
    check(IsInGameThread());
    Startup();
    if (!Extension.IsValid() || !Input.Scene || !Input.Target.IsValid() || !Input.VolumeTarget.IsValid()) return;
    const auto Held=Extension;
    ENQUEUE_RENDER_COMMAND(YogNinjaFogMaskSubmit)([Held,Key,Input,State,WakeResource](FRHICommandListImmediate&)
    {
        FEntry& Entry=Held->Entries.FindOrAdd(Key);
        if (Entry.Input.Target!=Input.Target || Entry.Input.VolumeTarget!=Input.VolumeTarget || Entry.State!=State)
            Entry.LastRevision=MAX_uint64;
        Entry.Input=Input;
        // Resolve after volume initialization, before any later GT release command.
        if (WakeResource) Entry.Input.WakeDensity = WakeResource->GetRenderTargetTexture();
        Entry.State=State;
    });
}

void Remove(uint64 Key)
{
    check(IsInGameThread());
    if (!Extension.IsValid()) return;
    const auto Held=Extension;
    ENQUEUE_RENDER_COMMAND(YogNinjaFogMaskRemove)([Held,Key](FRHICommandListImmediate&)
    {
        Held->Entries.Remove(Key);
    });
}
}
