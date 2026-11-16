#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogStage12FogActor.generated.h"

class UBoxComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPostProcessComponent;
class USceneComponent;
class UTexture;
class UVolumeTexture;

/** Phase 1/2 candidate: a world-space density field integrated along the view ray. */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="YOG 阶段1/2 空间雾", PrioritizeCategories="空间雾 雾体形态 雾体动画 密度 受光 实际来源"))
class YOGVISIONRUNTIME_API AYogStage12FogActor : public AActor
{
    GENERATED_BODY()

public:
    AYogStage12FogActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="空间雾") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="空间雾") TObjectPtr<UBoxComponent> VolumeBounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="空间雾") TObjectPtr<UPostProcessComponent> PostProcess;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间雾", meta=(DisplayName="启用空间雾")) bool bFogEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间雾", meta=(DisplayName="空间积分材质")) TObjectPtr<UMaterialInterface> FogMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间雾", meta=(DisplayName="雾体半尺寸（厘米）", ClampMin="1")) FVector BoundsExtent = FVector(1700.f, 2000.f, 1200.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="密度", meta=(DisplayName="消光密度（每米）", ClampMin="0", ClampMax="5")) float Density = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="密度", meta=(DisplayName="静态密度起伏", ClampMin="0", ClampMax="1")) float DensityVariation = .3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="密度", meta=(DisplayName="世界噪声频率（每厘米）", ClampMin="0.00001", ClampMax="0.1")) float NoiseScale = .002f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="密度", meta=(DisplayName="边缘退散宽度（厘米）", ClampMin="1", ClampMax="10000")) float EdgeWidth = 350.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="启用三维密度形态", ToolTip="在世界空间的雾体内部采样三维纹理，不随镜头移动。关闭后保留基础密度起伏。")) bool bUseDensityShape = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="三维形态纹理（可选）", ToolTip="仅接受Volume Texture。留空使用空间积分材质自带的三维纹理。", EditCondition="bUseDensityShape")) TObjectPtr<UVolumeTexture> DensityShapeTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="内部形态强度", ClampMin="0", ClampMax="1", EditCondition="bUseDensityShape")) float ShapeStrength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="大形态频率（每厘米）", ClampMin="0.00001", ClampMax="0.1", EditCondition="bUseDensityShape")) float ShapeScale = .0005f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="细节强度", ClampMin="0", ClampMax="1", EditCondition="bUseDensityShape")) float DetailStrength = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="细节频率（每厘米）", ClampMin="0.00001", ClampMax="0.1", EditCondition="bUseDensityShape")) float DetailScale = .0025f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="内部形态边缘柔度", ClampMin="0.01", ClampMax="1", EditCondition="bUseDensityShape")) float ShapeEdgeSoftness = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="内部残留密度", ToolTip="在有机雾体内部保留的基础密度，避免形态纹理挖出全透明孔洞。", ClampMin="0", ClampMax="1")) float BodyCoreDensity = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="雾体轴向拉伸", ClampMin="0.05", ClampMax="10")) FVector ShapeStretch = FVector(1.f, 1.f, .7f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体形态", meta=(DisplayName="有机雾体轮廓", ToolTip="0保留原方形容器轮廓，1使用完整的有机雾体轮廓；容器仍限制最大范围。", ClampMin="0", ClampMax="1")) float BodyRoundness = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体动画", meta=(DisplayName="播放雾体动画", ToolTip="关闭后冻结在当前雾形，重新启用时从冻结时间继续。")) bool bAnimateFog = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体动画", meta=(DisplayName="编辑器预览动画", ToolTip="编辑器视口需要开启实时更新（Ctrl+R）。游戏运行中的动画独立于此开关。", EditCondition="bAnimateFog")) bool bAnimateInEditor = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体动画", meta=(DisplayName="世界流速（厘米/秒）", EditCondition="bAnimateFog")) FVector FlowVelocity = FVector(5.f, -2.f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体动画", meta=(DisplayName="流动弯曲幅度（厘米）", ClampMin="0", ClampMax="1000")) float FlowWarpStrength = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体动画", meta=(DisplayName="弯曲周期（秒）", ClampMin="0.1", ClampMax="600")) float FlowPeriod = 24.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾体动画", meta=(DisplayName="动画速度倍率", ToolTip="倍率作用于累计动画时间。0冻结；调整倍率不会跳到另一时刻。", ClampMin="0", ClampMax="20")) float AnimationSpeed = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="散射反照率")) FLinearColor FogAlbedo = FLinearColor(.85f, .9f, .95f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="环境底光")) FLinearColor AmbientRadiance = FLinearColor(.035f, .045f, .06f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="环境底光强度", ClampMin="0", ClampMax="10")) float AmbientStrength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="读取场景方向光")) bool bUseSceneDirectionalLight = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="手动方向水平角", ClampMin="-180", ClampMax="180")) float ManualDirectionalYaw = -45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="手动方向仰角", ClampMin="-89", ClampMax="89")) float ManualDirectionalPitch = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="手动方向光色")) FLinearColor DirectionalColour = FLinearColor(.08f, .1f, .14f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="方向光强度", ClampMin="0", ClampMax="10")) float DirectionalStrength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="场景方向光亮度换算", ToolTip="将场景方向光的照度换算为候选雾积分的入射辐亮度；独立美术倍率。", ClampMin="0", ClampMax="10")) float SceneDirectionalLightScale = .01f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="局部灯亮度换算", ToolTip="读取真实灯亮度、颜色、色温、体积散射倍率后乘此值。此候选实现的美术换算，不等同原生灯光测光单位。", ClampMin="0", ClampMax="10")) float LocalLightScale = .015f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="受光", meta=(DisplayName="灯光清雾强度", ToolTip="受光与清雾独立。受光验收时设0，灯只改变散射。", ClampMin="0", ClampMax="1")) float LightClearStrength = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="遮挡", meta=(DisplayName="几何距离场阴影")) bool bUseGeometryShadows = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="遮挡", meta=(DisplayName="阴影起点偏移（厘米）", ClampMin="0", ClampMax="100")) float GeometryShadowBias = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="读取原MI配色")) bool bUseArtistPalette = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="原美术配色材质", ToolTip="只读取Fog Color 01、Fog Color 02、lerp_fog，不修改原材质。")) TObjectPtr<UMaterialInterface> PaletteMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="原MI配色权重", ClampMin="0", ClampMax="1")) float PaletteStrength = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="近远配色过渡距离（厘米）", ClampMin="1", ClampMax="50000")) float PaletteDistance = 3000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="启用观察者清雾")) bool bEnableVision = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="清雾半径（厘米）", ClampMin="0", ClampMax="20000")) float ObserverClearRadius = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="清雾羽化（厘米）", ClampMin="1", ClampMax="20000")) float ObserverFeather = 650.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="清雾区残留密度比例", ClampMin="0", ClampMax="1")) float ClearDensityFraction = .1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="编辑器视野", meta=(DisplayName="镜头中心投射高度（厘米）")) float PreviewFocusHeight = 150.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="编辑器视野", meta=(DisplayName="镜头中心最大投射距离（厘米）", ClampMin="1", ClampMax="50000")) float PreviewFocusMaxDistance = 7000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="编辑器视野", meta=(DisplayName="使用固定预览中心")) bool bUseExplicitPreviewFocus = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="编辑器视野", meta=(DisplayName="固定预览中心", EditCondition="bUseExplicitPreviewFocus")) FVector PreviewFocusPosition = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="质量", meta=(DisplayName="视线积分步数", ClampMin="8", ClampMax="128")) int32 StepCount = 64;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="质量", meta=(DisplayName="光线阴影步数", ClampMin="1", ClampMax="32")) int32 ShadowSteps = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="质量", meta=(DisplayName="最大积分距离（厘米）", ClampMin="100", ClampMax="100000")) float MaxDistance = 15000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="调试", meta=(DisplayName="调试模式", ToolTip="0正常；其他取值由阶段1/2积分材质定义。", ClampMin="0", ClampMax="8")) int32 DebugMode = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共享场验证", meta=(DisplayName="三维场纹理（可选）", ToolTip="Texture3D接口：RGB为世界速度（厘米/秒），A为有符号密度增量（每米）。本阶段仅验证静态数据连通性，不运行流体模拟。留空使用积分材质默认纹理。")) TObjectPtr<UTexture> ExternalFieldTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="共享场验证", meta=(DisplayName="外部密度场强度", ToolTip="0关闭外部场对基础密度的影响。下一阶段模拟和笔触可复用此世界空间接口。", ClampMin="0", ClampMax="10")) float ExternalFieldStrength = 0.f;

    UFUNCTION(BlueprintCallable, Category="YOG|Stage12Fog") void SetFogMaterial(UMaterialInterface* Material);
    UFUNCTION(BlueprintPure, Category="YOG|Stage12Fog") UMaterialInstanceDynamic* GetFogMID() const { return FogMID; }
    UFUNCTION(BlueprintCallable, Category="YOG|Stage12Fog") void SetFogEnabled(bool bEnabled);
    UFUNCTION(BlueprintCallable, Category="YOG|Stage12Fog") void SetObserverActor(AActor* Actor);
    UFUNCTION(BlueprintCallable, CallInEditor, Category="YOG|Stage12Fog") void RefreshMaterialParameters();
    UFUNCTION(BlueprintPure, Category="YOG|Stage12Fog") FString GetSourceStatus() const;
    UFUNCTION(BlueprintCallable, Category="YOG|Stage12Fog", meta=(ToolTip="暂时固定雾体时间以便对比截图；不会改动已累计的播放时间。")) void SetFogTimeForCapture(float Seconds);
    UFUNCTION(BlueprintCallable, Category="YOG|Stage12Fog") void ClearFogTimeOverride();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="雾体动画", meta=(DisplayName="重置雾体动画")) void ResetFogAnimation();

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FString DirectionalSource;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FString ObserverSource;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FString PaletteSource;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") int32 LocalLightCount = 0;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FLinearColor EffectiveNearColour;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FLinearColor EffectiveFarColour;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") float EffectivePaletteMix = .07f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FLinearColor EffectiveGameplayObserver = FLinearColor::Transparent;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="雾体动画时间（秒）", ToolTip="实际传给雾积分的时间；已包含速度倍率。实时预览时应持续增加。")) float FogTimeSeconds = 0.f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="动画状态", ToolTip="编辑器运行状态表示允许累计时间；视口仍需要实时更新才能驱动Tick。")) FString AnimationStatus;

private:
    void EnsureMaterial();
    void ResolvePalette();
    void ResolveDirectional(FVector& TowardLight, FLinearColor& Radiance);
    void ResolveObserver();
    void UpdateLocalLights();
    void UpdateFogAnimation(float DeltaSeconds);
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FogMID;
    UPROPERTY(Transient) TWeakObjectPtr<AActor> ObserverActor;
    bool bTriedDefaultPalette = false;
    bool bTriedDefaultFog = false;
    bool bHadExternalFieldOverride = false;
    bool bHadDensityShapeOverride = false;
    double AccumulatedFogTimeSeconds = 0.;
    bool bHasFogTimeOverride = false;
    float FogTimeOverrideSeconds = 0.f;
};
