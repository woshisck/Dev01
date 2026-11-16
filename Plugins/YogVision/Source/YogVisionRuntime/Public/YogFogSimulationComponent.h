#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "YogFogSimulationComponent.generated.h"

class UTextureRenderTargetVolume;
struct FYogFogSimulationRenderState;

UENUM(BlueprintType)
enum class EYogFogSolverMode : uint8
{
    Advection UMETA(DisplayName="三维平流与交互"),
    ProjectedNS UMETA(DisplayName="三维平流与压力投影")
};

USTRUCT(BlueprintType)
struct YOGVISIONRUNTIME_API FYogFogSimBody
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互体") FVector Center = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互体") FVector Velocity = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互体", meta=(ClampMin="1")) float Radius = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互体", meta=(ClampMin="1")) float HalfHeight = 90.f;
};

/** World-aligned 3D density/velocity on the GPU. Textures are sampled directly by volume materials. */
UCLASS(ClassGroup=(Rendering), BlueprintType, meta=(BlueprintSpawnableComponent, DisplayName="YOG 三维雾场模拟"))
class YOGVISIONRUNTIME_API UYogFogSimulationComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UYogFogSimulationComponent();
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
    virtual void BeginDestroy() override;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维场", meta=(DisplayName="启用模拟")) bool bSimulationEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维场", meta=(DisplayName="编辑器实时模拟")) bool bSimulateInEditor = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维场", meta=(DisplayName="网格分辨率", ClampMin="8", ClampMax="128")) FIntVector GridResolution = FIntVector(64, 64, 48);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维场", meta=(DisplayName="三轴形态拉伸", ToolTip="X/Y/Z分别拉伸真实三维密度形态，改变模拟域大小也会拉伸同一初始形态；不是二维投射。", ClampMin="0.05", ClampMax="20")) FVector ShapeScaleXYZ = FVector::OneVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维场", meta=(DisplayName="求解方案")) EYogFogSolverMode SolverMode = EYogFogSolverMode::Advection;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维场", meta=(DisplayName="压力迭代次数", ClampMin="1", ClampMax="32", EditCondition="SolverMode==EYogFogSolverMode::ProjectedNS")) int32 PressureIterations = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="基础风速（厘米/秒）")) FVector WindVelocity = FVector(3.f, -1.f, .5f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="三维旋流强度", ClampMin="0", ClampMax="100")) float CurlStrength = 15.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="密度补充速度", ToolTip="缓慢补回世界空间的基础密度，避免长时间模拟后雾消失。", ClampMin="0", ClampMax="1")) float DensityReplenishment = .04f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="运动", meta=(DisplayName="速度阻尼（每秒）", ClampMin="0", ClampMax="5")) float VelocityDamping = .3f;

    /** Stable output: R is normalized density. No CPU readback or per-frame material texture swaps. */
    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出", meta=(DisplayName="三维密度场（R）")) TObjectPtr<UTextureRenderTargetVolume> DensityField;
    /** RGB is world velocity in centimetres/second. */
    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出", meta=(DisplayName="三维速度场（RGB厘米/秒）")) TObjectPtr<UTextureRenderTargetVolume> VelocityField;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出", meta=(DisplayName="已排队模拟时间（秒）")) float SimulationTime = 0.f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出", meta=(DisplayName="已排队固定步数")) int32 SubmittedStepCount = 0;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FVector ActualDomainCenter = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FVector ActualDomainExtent = FVector(2200.f, 2400.f, 1400.f);
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出", meta=(DisplayName="网格单元尺寸（厘米）")) FVector CellSize = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU输出") FIntVector ActualGridResolution = FIntVector::ZeroValue;

    UFUNCTION(BlueprintCallable, Category="YOG|FogSimulation") void SetDomain(FVector Center, FVector Extent);
    UFUNCTION(BlueprintCallable, Category="YOG|FogSimulation") void SetBodies(const TArray<FYogFogSimBody>& Bodies);
    UFUNCTION(BlueprintCallable, CallInEditor, Category="YOG|FogSimulation", meta=(DisplayName="重置三维雾场")) void ResetSimulation();
    UFUNCTION(BlueprintPure, Category="YOG|FogSimulation") FString GetSimulationStatus() const;

private:
    bool EnsureResources();
    void QueueSimulation(int32 StepCount, bool bReset);
    void ReleaseRenderState();
    UPROPERTY(VisibleAnywhere, Category="模拟域") FVector DomainCenter = FVector::ZeroVector;
    UPROPERTY(VisibleAnywhere, Category="模拟域") FVector DomainExtent = FVector(2200.f, 2400.f, 1400.f);
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UTextureRenderTargetVolume> DensityScratch;
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UTextureRenderTargetVolume> VelocityScratch;
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UTextureRenderTargetVolume> PressureA;
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UTextureRenderTargetVolume> PressureB;
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UTextureRenderTargetVolume> Divergence;
    UPROPERTY(Transient) TArray<FYogFogSimBody> InteractionBodies;
    TSharedPtr<FYogFogSimulationRenderState, ESPMode::ThreadSafe> RenderState;
    FIntVector AllocatedResolution = FIntVector::ZeroValue;
    FVector CachedShapeScale = FVector::ZeroVector;
    float TimeAccumulator = 0.f;
    bool bResetPending = true;
    bool bWasSimulationActive = false;
};
