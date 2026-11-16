#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogNativeAtlasFogActor.generated.h"

class UExponentialHeightFogComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;

/** Animated density is injected by a volume material; UE's native froxel fog evaluates lighting. */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="YOG 原生体积雾 / 流体图集", PrioritizeCategories="体积形态 图集动画 配色 预览 实际来源"))
class YOGVISIONRUNTIME_API AYogNativeAtlasFogActor : public AActor
{
    GENERATED_BODY()

public:
    AYogNativeAtlasFogActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="体积形态") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="体积形态") TObjectPtr<UStaticMeshComponent> VolumeMesh;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="体积形态") TObjectPtr<UExponentialHeightFogComponent> NativeFog;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="启用原生体积雾")) bool bFogEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="体积域材质", ToolTip="必须是Volume材质；密度写入原生体积网格，场景灯光由UE处理。")) TObjectPtr<UMaterialInterface> VolumeMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="雾体半尺寸（厘米）", ClampMin="1")) FVector BoundsExtent = FVector(2200.f, 2400.f, 1400.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="消光密度（每厘米）", ClampMin="0", ClampMax="0.1")) float Density = .0012f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="世界形态频率（每厘米）", ClampMin="0.00001", ClampMax="0.1")) float ShapeScale = .0006f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="密度细节强度", ClampMin="0", ClampMax="2")) float DetailStrength = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="散射反照率")) FLinearColor FogAlbedo = FLinearColor::White;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="原生雾可视距离（厘米）", ClampMin="100", ClampMax="50000")) float ViewDistance = 6000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积形态", meta=(DisplayName="原生雾散射方向性", ClampMin="-0.9", ClampMax="0.9")) float ScatteringDistribution = .2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="密度图集", ToolTip="独立验证工程内的密度flipbook图集；留空使用体积材质默认图集。")) TObjectPtr<UTexture2D> DensityAtlas;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="速度图集", ToolTip="与密度图集配套的速度图集，用于帧间流动插值。留空使用材质默认图集。")) TObjectPtr<UTexture2D> VelocityAtlas;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="图集播放帧率", ClampMin="0", ClampMax="60")) float AtlasFPS = 4.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="速度扰动强度", ClampMin="0", ClampMax="1")) float FlowStrength = .05f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="播放雾体动画", ToolTip="关闭后冻结当前帧；恢复后继续，不补算暂停期间时间。")) bool bAnimateFog = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="编辑器播放", ToolTip="编辑器视口需开启实时更新；游戏运行独立于此开关。")) bool bAnimateInEditor = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="图集动画", meta=(DisplayName="动画速度倍率", ClampMin="0", ClampMax="20")) float AnimationSpeed = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="读取原美术MI配色")) bool bUseArtistPalette = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="原美术配色材质", ToolTip="只读Fog Color 01、Fog Color 02、lerp_fog，不修改源资产。参数变更后可点刷新。")) TObjectPtr<UMaterialInterface> PaletteMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="手动近处色")) FLinearColor NearColour = FLinearColor(.112495f, .678705f, .744792f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="手动远处色")) FLinearColor FarColour = FLinearColor(.1106f, .1505f, .35f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="近远过渡距离（厘米）", ClampMin="1", ClampMax="50000")) float PaletteDistance = 3000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="检测场景方向光", ToolTip="每0.5秒检查一次。有真实方向光时关闭人工自发光塑形，原生雾自行受光；不创建或修改场景灯。")) bool bUseSceneDirectionalLight = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="无方向光时的塑形方向")) FVector FallbackLightDirection = FVector(.6f, -.6f, .6f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="配色", meta=(DisplayName="无方向光时的塑形强度", ToolTip="仅为材质的微弱自发光塑形。实际灯光照明由原生体积雾处理。", ClampMin="0", ClampMax="1")) float FallbackLightStrength = .02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="启用观察者清雾")) bool bEnableVision = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="清雾半径（厘米）", ClampMin="0", ClampMax="20000")) float ClearRadius = 500.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="清雾羽化（厘米）", ClampMin="1", ClampMax="20000")) float ClearFeather = 650.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="近处残留密度", ClampMin="0", ClampMax="1")) float ClearResidual = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="编辑器使用固定清雾中心")) bool bUseExplicitPreviewFocus = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="固定清雾中心", EditCondition="bUseExplicitPreviewFocus")) FVector PreviewFocusPosition = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="镜头中心投射高度（厘米）")) float PreviewFocusHeight = 150.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="镜头中心投射距离上限（厘米）", ClampMin="1", ClampMax="50000")) float PreviewFocusMaxDistance = 7000.f;

    UFUNCTION(BlueprintCallable, Category="YOG|NativeAtlasFog") void SetVolumeMaterial(UMaterialInterface* Material);
    UFUNCTION(BlueprintPure, Category="YOG|NativeAtlasFog") UMaterialInstanceDynamic* GetFogMID() const { return FogMID; }
    UFUNCTION(BlueprintPure, Category="YOG|NativeAtlasFog") bool IsFogMaterialBound() const;
    UFUNCTION(BlueprintCallable, Category="YOG|NativeAtlasFog") void SetFogEnabled(bool bEnabled);
    UFUNCTION(BlueprintCallable, Category="YOG|NativeAtlasFog") void SetObserverActor(AActor* Actor);
    UFUNCTION(BlueprintCallable, CallInEditor, Category="预览", meta=(DisplayName="刷新体积材质参数")) void RefreshMaterialParameters();
    UFUNCTION(BlueprintCallable, Category="YOG|NativeAtlasFog") void SetFogTimeForCapture(float Seconds);
    UFUNCTION(BlueprintCallable, Category="YOG|NativeAtlasFog") void ClearFogTimeOverride();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="图集动画", meta=(DisplayName="重置雾体动画")) void ResetFogAnimation();
    UFUNCTION(BlueprintPure, Category="YOG|NativeAtlasFog") FString GetSourceStatus() const;

    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="雾体播放时间（秒）")) float FogTimeSeconds = 0.f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="动画状态")) FString AnimationStatus;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="实际材质绑定")) FString MaterialBindingStatus;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="静态参数上传次数")) int32 StaticParameterUploadCount = 0;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源", meta=(DisplayName="实际观察者位置")) FLinearColor EffectiveObserverPosition = FLinearColor::Transparent;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FString ObserverSource;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FString DirectionalSource;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FString PaletteSource;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FLinearColor EffectiveNearColour;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") FLinearColor EffectiveFarColour;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="实际来源") float EffectivePaletteMix = .07f;

private:
    bool EnsureMaterial(bool bForceNew = false);
    void ApplyNativeFogSettings();
    void PushStaticParameters();
    void PushDynamicParameters();
    void ResolvePalette(bool bForce);
    void ResolveDirectional();
    void ResolveObserver();
    void UpdateAnimation(float DeltaSeconds);
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> FogMID;
    UPROPERTY(Transient) TWeakObjectPtr<AActor> ObserverActor;
    UPROPERTY(Transient) TWeakObjectPtr<UMaterialInterface> CachedPaletteMaterial;
    bool bTriedDefaultMaterial = false;
    bool bTriedDefaultTextures = false;
    bool bTriedDefaultPalette = false;
    bool bPaletteCached = false;
    bool bSceneDirectionalFound = false;
    bool bStaticParametersDirty = true;
    FVector CachedBoundsExtent = FVector::ZeroVector;
    FTransform CachedActorTransform;
    float DirectionalCheckAccumulator = 0.f;
    FVector EffectiveFallbackDirection = FVector(.6f, -.6f, .6f);
    double AccumulatedFogSeconds = 0.;
    bool bHasTimeOverride = false;
    float OverrideFogSeconds = 0.f;
};
