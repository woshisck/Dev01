#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogNinjaFogEnvironmentActor.generated.h"

class APlayerController;
class ULocalLightComponent;
class UTextureRenderTarget2D;
class UTextureRenderTargetVolume;
namespace YogNinjaFogMaskCache { struct FState; }
class UMaterialInstanceDynamic;
class USceneComponent;

/** Supplies world-space interaction parameters to an explicitly bound Ninja MID. */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="Ninja 雾体环境交互", PrioritizeCategories="绑定 空间高度 地面雾体 玩家清雾 灯光清雾"))
class YOGVISIONRUNTIME_API AYogNinjaFogEnvironmentActor : public AActor
{
    GENERATED_BODY()

public:
    AYogNinjaFogEnvironmentActor();
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;
    virtual void BeginDestroy() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="绑定")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="绑定", meta=(DisplayName="绑定 Ninja 雾体", ToolTip="选择当前场景中带 YogNinjaSmokeExact 标签的原版 VolumeSmoke 容器。每个雾体只绑定一个环境控制器；仅写它的动态材质，不修改原始蓝图或材质资产。"))
    TObjectPtr<AActor> FogCarrier;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="绑定", meta=(DisplayName="启用环境交互", ToolTip="关闭后把绑定动态材质的 SpatialEnvironmentEnabled 归零，保留原版 Ninja 灯光和动画。"))
    bool bEnvironmentEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="地面世界高度", ToolTip="世界 Z，单位厘米。编辑器清雾中心使用视口中心射线与此水平面的交点；不会逐帧自动改变地面高度。", ClampMin="-1000000", ClampMax="1000000"))
    float ManualGroundHeight = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="贴地雾体高度", ToolTip="厘米。控制近地实心雾的高度范围。", ClampMin="1", ClampMax="10000"))
    float GroundBodyHeight = 180.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="贴地雾量", ToolTip="近地雾密度乘数。", ClampMin="0", ClampMax="20"))
    float GroundAmount = 2.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="上层雾量", ToolTip="较高区域的雾量乘数。", ClampMin="0", ClampMax="20"))
    float UpperAmount = .55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="表面退散宽度", ToolTip="厘米。交给材质的表面接触退散距离。", ClampMin="1", ClampMax="5000"))
    float SurfaceFade = 40.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="接触影响范围", ToolTip="厘米。交给材质的环境表面影响范围。", ClampMin="1", ClampMax="10000"))
    float SurfaceBand = 250.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="地面雾体", meta=(DisplayName="接触雾量增强", ToolTip="环境表面附近的雾量增强系数。", ClampMin="0", ClampMax="10"))
    float ContactBoost = .35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间高度", meta=(DisplayName="启用空间雾高", ToolTip="仅 Hybrid 交互缓存使用。一个雾体内，玩家和局部灯附近保持低雾，远处升高，靠墙进一步增高；不改变容器的位置或缩放。"))
    bool bSpatialHeightEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间高度", meta=(DisplayName="近处雾高", ToolTip="厘米，相对地面。玩家或有效局部灯中心附近留下的低雾高度；清雾仍保留底雾。", EditCondition="bSpatialHeightEnabled", ClampMin="1", ClampMax="10000"))
    float NearFogHeight = 35.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间高度", meta=(DisplayName="远处雾高", ToolTip="厘米，相对地面。远离玩家和灯光后逐渐达到此高度；不低于近处雾高。", EditCondition="bSpatialHeightEnabled", ClampMin="1", ClampMax="10000"))
    float FarFogHeight = 350.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间高度", meta=(DisplayName="靠墙雾高", ToolTip="厘米，相对地面。距离场探测到墙体时的雾高；不低于远处雾高，玩家和灯光附近仍保持低雾。", EditCondition="bSpatialHeightEnabled", ClampMin="1", ClampMax="10000"))
    float WallFogHeight = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空间高度", meta=(DisplayName="顶部退散宽度", ToolTip="厘米。雾顶上下的宽缓过渡范围；近处自动收窄，避免低雾变成高层薄纱。", EditCondition="bSpatialHeightEnabled", ClampMin="1", ClampMax="5000"))
    float HeightFeather = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="空间高度", meta=(DisplayName="靠墙影响范围", ToolTip="厘米。只在低频缓存中探测墙体，探测高度高于地面以排除平地；不增加视线或阴影 raymarch 的距离场查询。", EditCondition="bSpatialHeightEnabled", ClampMin="1", ClampMax="1000"))
    float WallRange = 150.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家清雾", meta=(DisplayName="清雾半径", ToolTip="厘米。编辑状态以视口中心地面点为中心；运行时跟随第一个本地玩家。", ClampMin="0", ClampMax="50000"))
    float PlayerRadius = 450.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家清雾", meta=(DisplayName="清雾强度", ToolTip="0 至 1。雾体材质中的玩家稀薄权重，不授予 gameplay 视野。", ClampMin="0", ClampMax="1"))
    float PlayerStrength = .85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家清雾", meta=(DisplayName="清雾羽化", ToolTip="厘米。玩家清雾区域边缘的过渡宽度。", ClampMin="1", ClampMax="10000"))
    float PlayerFeather = 400.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="玩家清雾", meta=(DisplayName="清雾高度", ToolTip="厘米。交给材质的玩家清雾垂直范围。", ClampMin="1", ClampMax="10000"))
    float PlayerHeight = 350.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灯光清雾", meta=(DisplayName="灯光清雾强度", ToolTip="最多四个有效局部灯参与；范围取实际衰减半径的 45%。只影响雾密度，不修改灯光或 gameplay 视野。", ClampMin="0", ClampMax="1"))
    float LightStrength = .8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灯光清雾", meta=(DisplayName="灯光清雾半径上限", ToolTip="厘米。清雾半径取真实灯光衰减半径的 45%，再限制到此上限；避免大范围补光清薄整个场景。最小 1 厘米，关闭请将灯光清雾强度设为 0。", ClampMin="1", ClampMax="50000"))
    float LightRadiusLimit = 800.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灯光清雾", meta=(DisplayName="灯光清雾羽化", ToolTip="厘米。灯光清雾范围边缘的过渡宽度。", ClampMin="1", ClampMax="10000"))
    float LightFeather = 450.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="缓存质量", meta=(DisplayName="使用低分辨率交互缓存", ToolTip="Hybrid 材质把地面、距离场和清雾系数缓存到分层贴图，避免逐个视线与阴影样点查询距离场。"))
    bool bUseCachedEnvironment = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="缓存质量", meta=(DisplayName="遮罩平面分辨率", ClampMin="32", ClampMax="256"))
    int32 MaskResolution = 128;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="缓存质量", meta=(DisplayName="遮罩高度层数", ToolTip="高度采样向地面集中；影响环境遮罩精度，不是雾的raymarch步数。", ClampMin="4", ClampMax="32"))
    int32 MaskHeightSlices = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="缓存质量", meta=(DisplayName="交互缓存更新频率", ToolTip="每秒更新次数；角色和灯光清雾最多延迟一个更新周期。", ClampMin="1", ClampMax="30"))
    float MaskUpdatesPerSecond = 10.f;

    UPROPERTY(VisibleInstanceOnly, Transient, DuplicateTransient, Category="状态", meta=(DisplayName="环境遮罩缓存"))
    TObjectPtr<UTextureRenderTarget2D> EnvironmentMask;

    // Keep the existing atlas available to debug tools and the reference
    // material. Both resources receive the same coefficients in one dispatch.
    UPROPERTY(VisibleInstanceOnly, Transient, DuplicateTransient, Category="状态", meta=(DisplayName="三维环境遮罩缓存", ToolTip="与二维诊断图集使用相同系数、分辨率和更新频率；体积材质以一次三维线性采样读取。"))
    TObjectPtr<UTextureRenderTargetVolume> EnvironmentMaskVolume;

    UPROPERTY(VisibleInstanceOnly, Transient, Category="状态", meta=(DisplayName="缓存计算次数"))
    int32 MaskBakeCount = 0;

    UPROPERTY(VisibleInstanceOnly, Transient, Category="状态", meta=(DisplayName="当前观察点"))
    FString ObserverStatus;

    UPROPERTY(VisibleInstanceOnly, Transient, Category="状态", meta=(DisplayName="有效清雾灯数"))
    int32 ActiveLightCount = 0;

    UPROPERTY(VisibleInstanceOnly, Transient, Category="状态", meta=(DisplayName="绑定状态"))
    FString BindingStatus;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="地面雾体", meta=(DisplayName="向下拾取地面高度", ToolTip="从当前观察点上方 500 厘米向下，对 WorldStatic 做复杂射线查询。成功后写入地面世界高度；不会持续自动采样。"))
    void SampleGroundHeight();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="绑定", meta=(DisplayName="刷新环境参数", ToolTip="立即更新绑定动态材质，并重新选择距离观察点最近的四个有效局部灯。"))
    void RefreshEnvironment();

    UFUNCTION(BlueprintCallable, CallInEditor, Category="绑定", meta=(DisplayName="开启当前视口实时预览"))
    void EnableEditorPreview();

protected:
    // A global renderer supplies its own MID and cache footprint; legacy
    // placed Ninja containers retain the original resolution path unchanged.
    virtual UMaterialInstanceDynamic* ResolveOutputMaterial() const;
    virtual bool ResolveMaskDomain(UMaterialInstanceDynamic* Material,
        const FVector& ObserverPosition, FVector& Minimum, FVector& Size) const;

private:
    bool ResolveObserverPosition(FVector& Position, FString& Source);
    bool IsUsableLight(const ULocalLightComponent* Light) const;
    void RefreshLightCache(const FVector& ObserverPosition);
    void PushEnvironment(bool bForceLightRefresh);
    void DisableEnvironmentOverride();
    void UpdateMaskCache(UMaterialInstanceDynamic* Material, const FVector& ObserverPosition, bool bHaveObserver, bool bForce);
    void ReleaseMaskCache();
    float GroundHeight() const;

    TWeakObjectPtr<UMaterialInstanceDynamic> LastMaterial;
    TWeakObjectPtr<APlayerController> CachedLocalPlayer;
    TArray<TWeakObjectPtr<ULocalLightComponent>> CachedLights;
    double NextLightScanTime = 0.0;
    double NextMaskUpdateTime = 0.0;
    uint32 MaskRevision = 0;
    uint32 MaskDomainRevision = 0;
    FVector CachedMaskMin = FVector::ZeroVector;
    FVector CachedMaskSize = FVector::ZeroVector;
    TSharedPtr<YogNinjaFogMaskCache::FState, ESPMode::ThreadSafe> MaskRenderState;
};
