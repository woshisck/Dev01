#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YogFogSimulationComponent.h"
#include "YogFogPlanarSimulationComponent.generated.h"

class UTexture2D;
class UTextureRenderTarget2D;
struct FYogFogPlanarRenderState;

/** World-XY simulation only. A separate volume material reconstructs its height and lighting. */
UCLASS(ClassGroup=(Rendering), BlueprintType, meta=(BlueprintSpawnableComponent, DisplayName="YOG 世界平面雾场"))
class YOGVISIONRUNTIME_API UYogFogPlanarSimulationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYogFogPlanarSimulationComponent();
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void BeginDestroy() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="平面模拟", meta=(DisplayName="启用模拟")) bool bSimulationEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="平面模拟", meta=(DisplayName="编辑器实时模拟")) bool bSimulateInEditor = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="平面模拟", meta=(DisplayName="网格分辨率", ClampMin="16", ClampMax="1024")) FIntPoint GridResolution = FIntPoint(256, 256);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="艺术密度", meta=(DisplayName="密度纹理（线性R通道）", ToolTip="在固定世界XY坐标中平铺；白色浓，黑色淡。属于YOG画意环境雾Actor时，此项由Actor的雾团形态→美术密度纹理管理并置灰。独立组件更换纹理会重置场。")) TObjectPtr<UTexture2D> DensityTexture;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="艺术密度", meta=(DisplayName="纹理平铺尺寸（厘米）", ToolTip="属于YOG画意环境雾Actor时，由Actor的雾团世界平铺尺寸管理。独立组件可在此设置。", ClampMin="1")) FVector2D DensityTileSize = FVector2D(1800., 1800.);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="艺术密度", meta=(DisplayName="纹理UV偏移")) FVector2D ShapeOffsetUV = FVector2D::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="艺术密度", meta=(DisplayName="基础密度倍率", ClampMin="0", ClampMax="3")) float SeedDensityScale = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="艺术密度", meta=(DisplayName="基础密度偏移", ClampMin="-1", ClampMax="1")) float SeedDensityBias = .05f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="世界XY风速（厘米/秒）")) FVector2D WindVelocity = FVector2D(3., -1.);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="低频旋流强度", ClampMin="0", ClampMax="100")) float CurlStrength = 15.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="密度补充速度（每秒）", ClampMin="0", ClampMax="1")) float DensityReplenishment = .04f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="速度阻尼（每秒）", ClampMin="0", ClampMax="5")) float VelocityDamping = .3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互", meta=(DisplayName="交互高度带半高（厘米）", ToolTip="只接收和以模拟中心Z为中心的高度带相交的胶囊。二维场不表达不同高度独立的尾迹。", ClampMin="1")) float HeightBandHalfHeight = 600.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互", meta=(DisplayName="尾迹衰减时间（秒）", ClampMin="0.1", ClampMax="30")) float WakeDecaySeconds = 3.f;

    /** Stable texture identity. RG=world XY cm/s; B=final normalized density; A=signed wake. */
    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出", meta=(DisplayName="平面密度速度RT（RG速度/B密度/A扰动）")) TObjectPtr<UTextureRenderTarget2D> PlanarField;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") float SimulationTime = 0.f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") int32 SubmittedStepCount = 0;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") int32 SubmittedBodyCount = 0;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FVector ActualDomainCenter = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FVector2D ActualDomainHalfExtent = FVector2D(2200., 2400.);
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FVector2D CellSize = FVector2D::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FIntPoint ActualGridResolution = FIntPoint::ZeroValue;

    UFUNCTION(BlueprintCallable, Category="YOG|PlanarFog") void SetDomain(FVector Center, FVector2D HalfExtent);
    UFUNCTION(BlueprintCallable, Category="YOG|PlanarFog") void SetBodies(const TArray<FYogFogSimBody>& Bodies);
    UFUNCTION(BlueprintCallable, CallInEditor, Category="YOG|PlanarFog", meta=(DisplayName="重置世界平面雾场")) void ResetSimulation();
    UFUNCTION(BlueprintPure, Category="YOG|PlanarFog") FString GetSimulationStatus() const;

private:
    bool EnsureResources();
    void ReleaseRenderState();
    void QueueSimulation(int32 Steps, bool bReset);
    bool HasSeedChanged() const;
    UPROPERTY(VisibleAnywhere, Category="模拟域") FVector DomainCenter = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, Category="模拟域") FVector2D DomainHalfExtent = FVector2D(2200., 2400.);
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UTextureRenderTarget2D> ScratchField;
    UPROPERTY(Transient) TArray<FYogFogSimBody> InteractionBodies;
    UPROPERTY(Transient) TWeakObjectPtr<UTexture2D> CachedDensityTexture;
    TSharedPtr<FYogFogPlanarRenderState, ESPMode::ThreadSafe> RenderState;
    FIntPoint AllocatedResolution = FIntPoint::ZeroValue;
    FVector2D CachedTileSize = FVector2D::ZeroVector;
    FVector2D CachedOffsetUV = FVector2D::ZeroVector;
    const void* CachedArtReference = nullptr;
    float CachedSeedScale = 0.f, CachedSeedBias = 0.f;
    float TimeAccumulator = 0.f;
    bool bResetPending = true;
    bool bWasSimulationActive = false;
};
