#pragma once
#include "CoreMinimal.h"
#include "YogNativeAtlasFogActor.h"
#include "Engine/Scene.h"
#include "YogPlanarFogActor.generated.h"
class UYogFogPlanarSimulationComponent;
class UTexture2D;
class UTextureRenderTarget2D;
class APostProcessVolume;
struct FPlanarFogArtistOverride
{
    TWeakObjectPtr<APostProcessVolume> Volume;
    int32 Index=INDEX_NONE;
    FWeightedBlendable Original;
    TWeakObjectPtr<UMaterialInstanceDynamic> Applied;
};

/** World XY transport reconstructed into a genuinely spatial native volumetric medium. */
UCLASS(BlueprintType, Blueprintable, HideCategories=("图集动画","体积形态","配色","预览"), meta=(DisplayName="YOG 画意环境雾", PrioritizeCategories="雾团形态 笔触 配色与照明 视野与预览 二维流动 实际来源"))
class YOGVISIONRUNTIME_API AYogPlanarFogActor : public AYogNativeAtlasFogActor
{
    GENERATED_BODY()
public:
    AYogPlanarFogActor();
    virtual void OnConstruction(const FTransform&) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type) override;
    virtual void Destroyed() override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent&) override;
    // Editor module owns save/PIE delegates; no UnrealEd dependency belongs in Runtime.
    void SetEditorLegacyFogOverrideSuspended(bool bSuspend);
#endif
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="二维流动") TObjectPtr<UYogFogPlanarSimulationComponent> PlanarSimulation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="启用环境雾")) bool bMistEnabled=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="雾体半尺寸（厘米）")) FVector MistExtent=FVector(2600,2800,1500);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="消光密度（每厘米）",ClampMin="0",ClampMax=".03")) float MistDensity=.0016f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="美术密度纹理",ToolTip="这是本Actor和二维模拟共同使用的唯一密度纹理入口。PlanarSimulation中的对应项由此自动绑定，不需要重复设置。")) TObjectPtr<UTexture2D> DensityPattern;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="雾团世界平铺尺寸（厘米）",ClampMin="100",ToolTip="同时控制世界XY模拟的密度纹理平铺和体积材质的域外基础雾。改变后二维场会重新初始化。")) float PatternWorldSize=1800;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="底雾保留",ClampMin="0",ClampMax="1")) float DensityFloor=.20f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="团块密度曲线",ClampMin=".1",ClampMax="4")) float DensityPower=1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="雾层中心相对高度（厘米）")) float HeightCenter=0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="雾层半高（厘米）",ClampMin="1")) float HeightHalfSize=1300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="高度退散宽度（厘米）",ClampMin="1")) float HeightSoftness=450;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="三维细节尺寸（厘米）",ClampMin="10")) float NoiseWorldSize=380;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团形态", meta=(DisplayName="三维细节侵蚀",ClampMin="0",ClampMax="1")) float NoiseStrength=.32f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="粗笔刷纹理")) TObjectPtr<UTexture2D> BrushPattern;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="笔刷世界平铺尺寸（厘米）",ClampMin="10")) float BrushWorldSize=900;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="雾层笔触强度",ClampMin="0",ClampMax="1",ToolTip="先渲染原生体积雾，再对雾散射光映射笔触；0只显示基础雾，不改变消光密度和遮挡。")) float BrushStrength=0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="笔触对比",ClampMin=".1",ClampMax="4")) float BrushContrast=1.3f;
    UPROPERTY(EditAnywhere, Category="笔触", meta=(DisplayName="雾层笔触分布",ClampMin="0",ClampMax="1",ToolTip="从雾中采样位置筛选笔触，不是屏幕像素覆盖百分比。")) float BrushCoverage=.22f;
    UPROPERTY(EditAnywhere, Category="笔触", meta=(DisplayName="雾层映射采样数",ClampMin="4",ClampMax="24",ToolTip="沿原生雾累计散射分段映射，更多采样减少多层雾混合误差，同时增加开销。")) int32 FogBrushSamples=8;
    // Old drafts may deserialize this value; brush mapping never erodes density.
    UPROPERTY() float BrushRelief=.60f;
    // Retained only to deserialize old instances; surface brush rendering is no longer used.
    UPROPERTY() float SurfaceStrokeStrength=0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="建筑块面塑形",ClampMin="0",ClampMax="2")) float FormStrength=.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="预览时停用旧MI雾，保留描边",ToolTip="只对当前世界PencilPostOutline创建临时MID并延后其雾距离，不修改源MI或关卡；关闭或销毁时恢复。")) bool bSuppressLegacyFog=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="二维流动", meta=(DisplayName="共用流速扰动位移（厘米）",ClampMin="0",ClampMax="100",ToolTip="雾细节和雾层笔触共用世界速度场。此项调整共用流动，不是笔触密度。")) float FlowDisplacement=12;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="读取原美术MI配色")) bool bReadArtistPalette=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="原美术配色材质")) TObjectPtr<UMaterialInterface> ArtistPalette;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="手动近处雾色",EditCondition="!bReadArtistPalette")) FLinearColor MistNearColour=FLinearColor(.10f,.22f,.27f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="手动远处雾色",EditCondition="!bReadArtistPalette")) FLinearColor MistFarColour=FLinearColor(.13f,.20f,.27f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="镜头配色过渡距离（厘米）",ClampMin="1")) float ColourDistance=4000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="散射反照率")) FLinearColor MistAlbedo=FLinearColor(.62f,.68f,.75f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="基础空气受光强度",ClampMin="0",ClampMax="2")) float AmbientStrength=.24f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="读取场景方向光")) bool bReadSceneDirectional=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色与照明", meta=(DisplayName="无方向光时的塑形方向")) FVector MistLightDirection=FVector(.6,-.6,.6);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="启用近处薄雾区")) bool bClearNearObserver=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="镜头前雾剔除距离（厘米）",ClampMin="0",ToolTip="使用原生体积雾的近距离剔除，避免相机前的雾把整张画面冲平；不移动世界密度场。")) float CameraFogStartDistance=1200.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="镜头前雾渐入宽度（厘米）",ClampMin="0")) float CameraFogFadeDistance=700.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="近处薄雾区半径（厘米）",ClampMin="0")) float VisionRadius=420;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="薄雾退散宽度（厘米）",ClampMin="1")) float VisionFeather=700;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="近处残留雾比例",ClampMin="0",ClampMax="1")) float VisionResidual=.22f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="固定编辑器观察中心")) bool bPinEditorFocus=false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="编辑器观察中心",EditCondition="bPinEditorFocus")) FVector EditorFocus=FVector(1800,-600,100);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野与预览", meta=(DisplayName="激活原生雾渲染开关",ToolTip="仅首次激活请求r.VolumetricFog=1；外部随后关闭时不抢写，点刷新可重新激活。")) bool bRequestNativeFog=true;
    UFUNCTION(BlueprintCallable, CallInEditor, Category="视野与预览") void RefreshPlanarFog();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="视野与预览", meta=(DisplayName="暂停雾体流动")) void PausePlanarFog();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="视野与预览", meta=(DisplayName="继续雾体流动")) void ResumePlanarFog();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="二维流动", meta=(DisplayName="控制入口说明")) FString SimulationControlSource=TEXT("播放和编辑器播放：选择PlanarSimulation组件。密度纹理和平铺尺寸：在Actor的雾团形态组设置。");
    UFUNCTION(BlueprintPure, Category="YOG|PlanarFog") FString GetPlanarFogStatus() const;
    /** RGB is pre-exposed native fog light; A is transmittance to the visible scene depth of the last persistent viewport. */
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="雾光照RGB/透射率A（最近视口）")) TObjectPtr<UTextureRenderTarget2D> FogTransport;
    void SetLatestFogTransport(UTextureRenderTarget2D* Target){FogTransport=Target;}
    /** Signed pre-exposed scattering delta; A stores scene depth in metres for edge-aware composition. */
    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="雾笔触散射差值RGB/裁剪深度A（最近视口）")) TObjectPtr<UTextureRenderTarget2D> FogBrushLayer;
    void SetLatestFogBrushLayer(UTextureRenderTarget2D* Target){FogBrushLayer=Target;}
    UMaterialInstanceDynamic* GetPlanarFinishMID() const{return SurfaceFinishMID;}
private:
    void ConfigureParent();
    void BindField();
    void UpdateSurfaceFinish();
    void UpdateLegacyFogOverride();
    void RestoreLegacyFog();
    void ReleaseFogLease();
    void SynchronizeClock();
    bool bOwnFogLease=false;
    bool bTransportRegistered=false;
    bool bLegacyChecked=false;
#if WITH_EDITOR
    // Deliberately not reflected: PIE must start with its own unsuspended restore records.
    bool bEditorLegacyFogOverrideSuspended=false;
#endif
    TArray<TWeakObjectPtr<UExponentialHeightFogComponent>> SuppressedHeightFog;
    bool bTriedSurfaceFinish=false;
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> SurfaceFinishMID;
    UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> ArtistOriginalReferences;
    TArray<FPlanarFogArtistOverride> ArtistOverrides;
    int32 PreviousFogValue=0;
    uint32 PreviousFogPriority=0;
    float MaterialRetry=0;
    bool bTriedPlanarMaterial=false;
    bool bTriedDensityPattern=false;
    bool bTriedBrushPattern=false;
    bool bTriedArtistPalette=false;
};
