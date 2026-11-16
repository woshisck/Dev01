#pragma once

#include "CoreMinimal.h"
#include "YogNinjaFogEnvironmentActor.h"
#include "YogNinjaGlobalFogActor.generated.h"

class UPostProcessComponent;
class UMaterialInterface;
class UTextureRenderTarget2D;

/** Unbounded world-space raymarch: one placed settings actor, no proxy mesh or volume bounds. */
UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="YOG 全局体积雾", PrioritizeCategories="全局雾 云团与流动 地面雾体 玩家清雾 灯光清雾 体积受光 描边与笔触"), HideCategories=("绑定", "旧参数", "空间高度"))
class YOGVISIONRUNTIME_API AYogNinjaGlobalFogActor : public AYogNinjaFogEnvironmentActor
{
    GENERATED_BODY()
public:
    AYogNinjaGlobalFogActor();
    virtual void PostRegisterAllComponents() override;
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;
    virtual void BeginDestroy() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& Event) override;
    virtual void PostEditUndo() override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="组件") TObjectPtr<UPostProcessComponent> FogPostProcess;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="组件") TObjectPtr<UPostProcessComponent> FinishPostProcess;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="全局雾", meta=(DisplayName="启用全局雾")) bool bFogEnabled=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="全局雾", meta=(DisplayName="雾密度", ClampMin="0", ClampMax="30")) float Density=5.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="全局雾", meta=(DisplayName="高度衰减尺度", ToolTip="厘米。密度从地面开始连续向上稀薄，每升高此距离，高度密度系数减半；没有固定雾顶。值小则贴地集中，值大则高处保留更多雾。", ClampMin="50", ClampMax="50000", Units="cm")) float FogHeight=450.f;
    // Retain serialized/Blueprint compatibility, but a global exponential
    // atmosphere has no top transition interval. This value no longer clips it.
    UPROPERTY(BlueprintReadWrite, Category="旧参数", meta=(DeprecatedProperty, DeprecationMessage="全局雾改为连续高度衰减，请使用高度衰减尺度；顶部柔化不再参与计算。")) float TopFeather=300.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="全局雾", meta=(DisplayName="最大渲染距离", ToolTip="从每个视图的镜头沿射线计算，不是全局雾的水平盒范围。", ClampMin="100", ClampMax="100000", Units="cm")) float MaxDistance=10000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="全局雾", meta=(DisplayName="镜头起始距离", ClampMin="0", ClampMax="50000", Units="cm")) float StartDistance=0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="全局雾", meta=(DisplayName="镜头前柔化距离", ClampMin="1", ClampMax="10000", Units="cm")) float NearFade=300.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="全局雾", meta=(DisplayName="雾暗部颜色")) FLinearColor Color1=FLinearColor(.025f,.04f,.052f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="全局雾", meta=(DisplayName="雾亮部颜色")) FLinearColor Color2=FLinearColor(.11f,.15f,.17f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="全局雾", meta=(DisplayName="全局优先级", ToolTip="同一世界多个全局雾时，仅启用优先级最高者；相同优先级按Actor路径选择，避免重复叠加。")) int32 GlobalPriority=0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="云团与流动", meta=(DisplayName="云团尺寸", ClampMin="50", ClampMax="20000", Units="cm")) float CloudSize=1000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="云团与流动", meta=(DisplayName="云团厚薄对比", ClampMin="0", ClampMax="1")) float CloudVariation=.78f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="云团与流动", meta=(DisplayName="交界翻卷幅度", ClampMin="0", ClampMax="2")) float FlowStrength=.45f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="云团与流动", meta=(DisplayName="交界翻卷速度", ClampMin="0", ClampMax="2")) float FlowSpeed=.30f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="云团与流动", meta=(DisplayName="交界细节", ClampMin="0", ClampMax="1")) float BoundaryDetail=.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="云团与流动", meta=(DisplayName="播放密度贴图动画")) bool bAnimateInput=true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积受光", meta=(DisplayName="主光颜色")) FLinearColor VolumeLightColor=FLinearColor(.65f,.85f,.95f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积受光", meta=(DisplayName="主光强度", ClampMin="0", ClampMax="20")) float VolumeLightIntensity=1.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积受光", meta=(DisplayName="主光方向", ToolTip="世界空间朝向光源的方向；未绑定位置Actor时使用。")) FVector VolumeLightDirection=FVector(.25,-.35,.90);
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, AdvancedDisplay, Category="体积受光", meta=(DisplayName="主光位置Actor（可选）", ToolTip="有绑定时按其世界位置计算点光照；为空时使用主光方向。灯光退雾仍自动读取周围有效局部灯。")) TObjectPtr<AActor> LightingActor;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="体积受光", meta=(DisplayName="点主光衰减半径", ClampMin="1", ClampMax="100000", Units="cm")) float LightRadius=5500.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积受光", meta=(DisplayName="自阴影强度", ClampMin="0", ClampMax="10")) float ShadowStrength=1.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描边与笔触", meta=(DisplayName="启用场景描边与外围笔触", ToolTip="挂接已有雾后Finish材质。场景已有同一后处理时可关闭此项，避免重复。")) bool bEnableFinish=true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描边与笔触", meta=(DisplayName="雾内笔触强度", ClampMin="0", ClampMax="2")) float FogBrushStrength=.3f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描边与笔触", meta=(DisplayName="结构描边强度", ClampMin="0", ClampMax="2")) float OutlineStrength=.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描边与笔触", meta=(DisplayName="外围笔触强度", ClampMin="0", ClampMax="2")) float OuterBrushStrength=.75f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="描边与笔触", meta=(DisplayName="外围笔触疏密", ClampMin="0", ClampMax="1")) float OuterBrushDensity=.56f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="预览", meta=(DisplayName="雾通道预览", ToolTip="0 成图；1 雾遮罩；2 雾流向；3 雾受光。通道预览时暂停本Actor的场景描边合成。", ClampMin="0", ClampMax="3")) int32 PreviewMode=0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="渲染质量", meta=(DisplayName="视线步数", ClampMin="8", ClampMax="128")) int32 ViewSteps=32;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="渲染质量", meta=(DisplayName="自阴影步数", ClampMin="1", ClampMax="20")) int32 ShadowSamples=6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="渲染质量", meta=(DisplayName="交互缓存半径", ToolTip="仅控制距离场交互缓存的覆盖与精度，缓存外仍有全局雾和玩家/灯光解析退雾。", ClampMin="1000", ClampMax="30000", Units="cm")) float InteractionRadius=6000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="渲染质量", meta=(DisplayName="交互缓存移动网格", ClampMin="100", ClampMax="4000", Units="cm")) float CacheSnapSize=1000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="渲染质量", meta=(DisplayName="密度输入分辨率", ClampMin="64", ClampMax="1024")) int32 InputResolution=256;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="素材", meta=(DisplayName="全局雾材质")) TSoftObjectPtr<UMaterialInterface> GlobalFogMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="素材", meta=(DisplayName="Ninja密度与速度输入材质")) TSoftObjectPtr<UMaterialInterface> InputMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="素材", meta=(DisplayName="场景描边笔触材质")) TSoftObjectPtr<UMaterialInterface> FinishMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="素材", meta=(DisplayName="输入世界锚点", ToolTip="固定世界坐标，镜头移动不带动贴图滑动。")) FVector InputOrigin=FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category="素材", meta=(DisplayName="输入世界平铺尺寸", ClampMin="50", ClampMax="50000", Units="cm")) float InputWorldSize=4375.f;

    UPROPERTY(VisibleInstanceOnly, Transient, DuplicateTransient, BlueprintReadOnly, Category="状态") TObjectPtr<UMaterialInstanceDynamic> DynamicMaterialInstance;
    UPROPERTY(VisibleInstanceOnly, Transient, DuplicateTransient, BlueprintReadOnly, Category="状态") TObjectPtr<UTextureRenderTarget2D> DensityInput;
    UPROPERTY(VisibleInstanceOnly, Transient, BlueprintReadOnly, Category="状态", meta=(DisplayName="全局雾状态")) FString GlobalStatus;

    UFUNCTION(BlueprintCallable, CallInEditor, Category="全局雾", meta=(DisplayName="刷新全局雾", ToolTip="重新加载已选择的材质并更新全局参数；无需设置容器范围。")) void RefreshGlobalFog();

protected:
    virtual UMaterialInstanceDynamic* ResolveOutputMaterial() const override;
    virtual bool ResolveMaskDomain(UMaterialInstanceDynamic* Material,
        const FVector& ObserverPosition, FVector& Minimum, FVector& Size) const override;

private:
    bool CanUpdate() const;
    bool IsSelectedGlobalFog() const;
    void RegisterGlobalFog();
    void UnregisterGlobalFog();
    void UpdateGlobalFog(float DeltaSeconds, bool bForce);
    void ClearOwnedRendering();
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> InputMID;
    UPROPERTY(Transient, DuplicateTransient) TObjectPtr<UMaterialInstanceDynamic> FinishMID;
    bool bTriedResources=false;
    bool bHasInputFrame=false;
    float InputElapsed=0.f;
};
