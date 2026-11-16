#pragma once

#include "CoreMinimal.h"
#include "YogFogSimulationComponent.h"
#include "YogNativeAtlasFogActor.h"
#include "YogComputeFogActor.generated.h"

class UMaterialInstanceDynamic;
class UTextureRenderTargetVolume;

/** Local GPU density/velocity simulation injected into UE's native volumetric fog. */
UCLASS(BlueprintType, Blueprintable, HideCategories=("图集动画", "实际来源"),
    HideFunctions=(SetFogTimeForCapture, ClearFogTimeOverride, ResetFogAnimation, GetSourceStatus),
    meta=(DisplayName="YOG 三维计算雾 / 原生体积受光", PrioritizeCategories="局部模拟域 交互来源 演示胶囊 GPU场预览 体积形态 配色 预览"))
class YOGVISIONRUNTIME_API AYogComputeFogActor : public AYogNativeAtlasFogActor
{
    GENERATED_BODY()

public:
    AYogComputeFogActor();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
#if WITH_EDITOR
    virtual bool CanEditChange(const FProperty* InProperty) const override;
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="局部模拟域", meta=(DisplayName="三维场模拟组件")) TObjectPtr<UYogFogSimulationComponent> Simulation;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="局部模拟域", meta=(DisplayName="局部模拟半尺寸（厘米）", ToolTip="局部交互网格的世界轴半尺寸，和父类的整体雾容器半尺寸独立。Actor缩放会改变模拟半尺寸，Actor旋转不旋转模拟网格。", ClampMin="1")) FVector SimulationHalfExtent = FVector(900.f, 900.f, 600.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="局部模拟域", meta=(DisplayName="模拟中心世界偏移（厘米）", ToolTip="相对Actor位置的世界轴偏移；不随Actor旋转或缩放。域外由体积材质的真实三维背景密度填充。")) FVector SimulationCenterOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="局部模拟域", meta=(DisplayName="空材质时加载默认计算雾", ToolTip="读取 /YogVision/Stage12/Compute/M_ComputeVolumeFog。默认材质尚未准备好时重试；父类自动回退的图集材质不会用于计算雾预览。")) bool bLoadDefaultComputeMaterial = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="优先使用本地玩家胶囊", ToolTip="游戏中只查询本世界的本地玩家控制器，不扫描全场Actor。尺度验证可关闭此项、清空显式列表并关闭演示体，以得到无交互源的同种子密度场。")) bool bUsePlayerBody = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="显式交互Actor", ToolTip="优先级为本地玩家、显式列表、演示胶囊。最多向模拟提交8个世界Z向胶囊；骨骼和复杂碰撞体后续接入。")) TArray<TObjectPtr<AActor>> Bodies;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="读取来源胶囊尺寸")) bool bUseSourceCapsuleSize = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="默认交互半径（厘米）", ClampMin="1", ClampMax="10000")) float DefaultBodyRadius = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="默认交互半高（厘米）", ClampMin="1", ClampMax="10000")) float DefaultBodyHalfHeight = 90.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="来源速度推力倍率", ClampMin="0", ClampMax="20")) float SourceVelocityStrength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="交互来源", meta=(DisplayName="最小交互核半径（格）", ToolTip="按模拟网格最大单元边长扩展过小胶囊，避免角色比一个网格单元还小而没有可见扰动。", ClampMin="0.5", ClampMax="5")) float MinimumKernelCells = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="启用演示移动胶囊", ToolTip="只向三维场提交胶囊数据，不生成Actor、网格或灯光。")) bool bEnableDemoBody = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="播放演示运动", EditCondition="bEnableDemoBody")) bool bAnimateDemo = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="演示随模拟暂停", ToolTip="开启时，关闭模拟或编辑器实时模拟也会冻结演示时钟；恢复后不补算暂停时间。", EditCondition="bEnableDemoBody")) bool bPauseDemoWithSimulation = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="演示中心偏移（厘米）", ToolTip="相对局部模拟域中心的世界轴偏移。", EditCondition="bEnableDemoBody")) FVector DemoCenterOffset = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="轨道XY幅度（厘米）", EditCondition="bEnableDemoBody")) FVector2D DemoOrbitAmplitude = FVector2D(350.f, 220.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="轨道周期（秒）", ClampMin="0.1", ClampMax="600", EditCondition="bEnableDemoBody")) float DemoPeriod = 8.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="起始相位（圈）", ClampMin="0", ClampMax="1", EditCondition="bEnableDemoBody")) float DemoPhaseOffset = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="演示半径（厘米）", ClampMin="1", ClampMax="10000", EditCondition="bEnableDemoBody")) float DemoRadius = 65.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="演示半高（厘米）", ClampMin="1", ClampMax="10000", EditCondition="bEnableDemoBody")) float DemoHalfHeight = 140.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="演示胶囊", meta=(DisplayName="演示速度推力倍率", ClampMin="0", ClampMax="20", EditCondition="bEnableDemoBody")) float DemoVelocityStrength = 1.f;

    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="GPU场预览", meta=(DisplayName="实际三维密度RT（R）")) TObjectPtr<UTextureRenderTargetVolume> DensityFieldPreview;
    UPROPERTY(Transient, DuplicateTransient, VisibleAnywhere, BlueprintReadOnly, Category="GPU场预览", meta=(DisplayName="实际三维速度RT（RGB厘米/秒）")) TObjectPtr<UTextureRenderTargetVolume> VelocityFieldPreview;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU场预览", meta=(DisplayName="密度RT实际绑定到材质")) bool bDensityFieldBound = false;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU场预览", meta=(DisplayName="体积材质声明DensityField参数")) bool bMaterialAcceptsDensityField = false;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="GPU场预览", meta=(DisplayName="GPU场绑定状态")) FString FieldBindingStatus;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="局部模拟域", meta=(DisplayName="实际模拟中心（世界）")) FVector EffectiveSimulationCenter = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="局部模拟域", meta=(DisplayName="实际模拟半尺寸（世界）")) FVector EffectiveSimulationExtent = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="局部模拟域", meta=(DisplayName="实际XYZ形态倍率", ToolTip="由模拟组件ShapeScaleXYZ控制；整体雾容器、局部网格范围、三轴形态比例是三个独立设置。")) FVector EffectiveShapeScaleXYZ = FVector::OneVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="交互来源", meta=(DisplayName="本帧提交胶囊数")) int32 ActiveBodyCount = 0;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="演示胶囊", meta=(DisplayName="演示独立时间（秒）")) float DemoTimeSeconds = 0.f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="演示胶囊", meta=(DisplayName="演示当前相位（圈）")) float DemoNormalizedPhase = 0.f;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="演示胶囊", meta=(DisplayName="演示世界位置")) FVector DemoBodyWorldPosition = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="演示胶囊", meta=(DisplayName="演示实际注入速度")) FVector DemoBodyWorldVelocity = FVector::ZeroVector;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="演示胶囊", meta=(DisplayName="演示胶囊已提交")) bool bDemoBodySubmitted = false;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="局部模拟域", meta=(DisplayName="刷新计算雾与材质绑定")) void RefreshComputeFog();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="局部模拟域", meta=(DisplayName="重置三维模拟场")) void ResetComputeFogSimulation();
    UFUNCTION(BlueprintCallable, CallInEditor, Category="演示胶囊", meta=(DisplayName="重置演示独立时钟")) void ResetDemoMotion();
    UFUNCTION(BlueprintPure, Category="YOG|ComputeFog", meta=(DisplayName="获取演示胶囊位置")) FVector GetDemoBodyWorldPosition() const { return DemoBodyWorldPosition; }
    UFUNCTION(BlueprintPure, Category="YOG|ComputeFog", meta=(DisplayName="获取演示胶囊注入速度")) FVector GetDemoBodyWorldVelocity() const { return DemoBodyWorldVelocity; }
    UFUNCTION(BlueprintPure, Category="YOG|ComputeFog", meta=(DisplayName="获取计算雾实际状态")) FString GetComputeFogStatus() const;

private:
    void TryResolveDefaultMaterial(float DeltaSeconds, bool bForceAttempt);
    void UpdateSimulationInputs(float DeltaSeconds);
    void BindSimulationFields();

    TArray<FYogFogSimBody> FrameBodies;
    TMap<TWeakObjectPtr<AActor>, FVector> PreviousBodyPositions;
    TWeakObjectPtr<UMaterialInstanceDynamic> CachedFieldMID;
    TWeakObjectPtr<UTextureRenderTargetVolume> CachedDensityField;
    FVector CachedSimulationCenter = FVector::ZeroVector;
    FVector CachedSimulationExtent = FVector::ZeroVector;
    FVector CachedShapeScaleXYZ = FVector::ZeroVector;
    FIntVector CachedGridResolution = FIntVector::ZeroValue;
    double DemoClockSeconds = 0.;
    float DefaultMaterialRetrySeconds = 0.f;
    bool bCachedSimulationReady = false;
};
