#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "YogAirFogVolume.generated.h"

class UExponentialHeightFogComponent;
class ULocalLightComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class USceneComponent;
class UStaticMeshComponent;
class UTexture2D;

USTRUCT(BlueprintType)
struct YOGVISIONRUNTIME_API FYogAirFogParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维雾体", meta=(DisplayName="浓雾体中心", ToolTip="固定世界坐标，厘米。浓雾是空间中的体积区，不跟随镜头；外部仍有薄雾。")) FVector FogBankCenter = FVector(2300.f, -1200.f, 800.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维雾体", meta=(DisplayName="浓雾体半尺寸", ToolTip="厘米。XYZ控制体积范围，Z决定雾墙高度。")) FVector FogBankExtent = FVector(1700.f, 2000.f, 1200.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="三维雾体", meta=(DisplayName="浓雾体权重", ClampMin="0", ClampMax="1")) float FogBankStrength = 0.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内视野", meta=(DisplayName="启用观察者清雾", ToolTip="世界空间球形范围。游戏跟随玩家；编辑器以镜头位置为中心。仅清雾范围跟随观察者，雾纹固定在世界。")) bool bUseScreenVision = true;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="基础视野半径", ToolTip="以视口高度为1，按宽高比修正，不随分辨率变化。", ClampMin="0.02", ClampMax="2")) float VisionRadius = .22f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="视野退散宽度", ClampMin="0.01", ClampMax="1")) float VisionFeather = .16f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="清雾区域", meta=(DisplayName="观察者清雾半径", ToolTip="厘米。游戏以玩家为中心，编辑器以镜头为中心；完全三维的球形区域。", ClampMin="0", ClampMax="20000")) float WorldVisionRadius = 450.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="清雾区域", meta=(DisplayName="观察者清雾过渡宽度", ToolTip="厘米。三维清雾边界的厚度，仍保留近处薄空气。", ClampMin="1", ClampMax="20000")) float WorldVisionFeather = 650.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内视野", meta=(DisplayName="灯光清雾强度", ToolTip="按真实灯的位置、半径与聚光锥近似清雾范围。不是敌人可见性或遮挡查询；原生体积阴影仍负责散射遮挡。", ClampMin="0", ClampMax="1")) float LightClearStrength = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内视野", meta=(DisplayName="灯光交界体积响应", ClampMin="0", ClampMax="2")) float LightBoundaryStrength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团边缘", meta=(DisplayName="远雾微弱体积感", ToolTip="外圈、全雾及暗部的微弱起伏；主要体积起伏只发生在交界。", ClampMin="0", ClampMax="0.25")) float OuterVolumeStrength = .025f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="笔触贴图")) TSoftObjectPtr<UTexture2D> StrokeTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/YogVision/Textures/T_ScreenVisionStrokes.T_ScreenVisionStrokes")));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="方向流图", ToolTip="线性RG向量，RG*2-1解码，0.5为静止；不作为灰度笔触使用。")) TSoftObjectPtr<UTexture2D> FlowTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/YogVision/Textures/T_ScreenBrushFlow.T_ScreenBrushFlow")));
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="流图平铺尺度", ClampMin="0.1", ClampMax="8")) float FlowMapScale = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="空气雾密度", ToolTip="每米的基础消光密度，由原生体积材质按厘米转换；越大则远处越厚。", ClampMin="0", ClampMax="5")) float Density = .10f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="近处薄空气密度", ToolTip="每米。深度增长前仍保留的薄空气，最大不超过空气雾密度；空气雾密度为0时也归零。", ClampMin="0", ClampMax="5")) float NearDensity = .006f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="清雾区域", meta=(DisplayName="镜头近区残留空气密度", ToolTip="每米。镜头近区实际路径保留的绝对消光密度，最大不超过近处薄空气密度。", ClampMin="0", ClampMax="5")) float ClearMinDensity = .003f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="镜头距离增浓起点", ToolTip="厘米。与镜头的三维距离；旋转镜头不改变密度场。达到此处后由近密度向基础密度增长。", ClampMin="0", ClampMax="50000")) float NearOffset = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="深度增长距离", ToolTip="厘米。雾密度随深度增浓的过渡距离。", ClampMin="1", ClampMax="50000")) float DepthRamp = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="深度增长曲线", ToolTip="控制近清远浓的增长形状；1为线性，高值让变化更集中在远处。", ClampMin="0.1", ClampMax="8")) float DepthPower = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="清雾区域", meta=(DisplayName="镜头近区清晰半径", ToolTip="厘米。镜头周围三维球形薄雾区，与视口方向无关。", ClampMin="0", ClampMax="20000")) float ClearRadius = 300.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="清雾区域", meta=(DisplayName="镜头近区退散宽度", ToolTip="厘米。从镜头近区残留密度过渡到薄空气的三维距离。", ClampMin="1", ClampMax="20000")) float ClearFeather = 650.f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="旧版清雾高度", ToolTip="仅保留旧草稿兼容；镜头深度模式不使用此参数。")) float ClearHeight = 2000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="世界高度基准", ToolTip="厘米。固定世界Z高度；此高度以上开始衰减，不随镜头移动。", ClampMin="-20000", ClampMax="50000")) float CameraHeightOffset = 1100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气密度", meta=(DisplayName="雾高度衰减", ToolTip="厘米。密度相对世界高度基准衰减的尺度。", ClampMin="10", ClampMax="50000")) float HeightFalloff = 1600.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团边缘", meta=(DisplayName="静态雾团密度起伏", ToolTip="宽缓、连续的世界空间雾团，不随时间漂移；笔刷也直接调制实际消光。0为均匀空气。", ClampMin="0", ClampMax="1")) float NoiseAmount = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团边缘", meta=(DisplayName="雾团空间频率", ToolTip="数值越小雾团越大。0.006对应数米级连续体块；与体积笔触尺度独立。", ClampMin="0.00001", ClampMax="1")) float NoiseScale = .006f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(ClampMin="-2000", ClampMax="2000")) float WindSpeed = 12.f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(ClampMin="0", ClampMax="5000")) float BoundaryNoise = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="美术配色", meta=(DisplayName="使用美术材质配色", ToolTip="从配色材质有效继承参数Fog Color 01、Fog Color 02与lerp_fog读取颜色；关闭后使用手动环境底色与远景雾色。")) bool bUseArtistPalette = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="美术配色", meta=(DisplayName="镜头配色过渡距离", ToolTip="厘米。近色沿镜头深度过渡到远色的距离。", ClampMin="1", ClampMax="100000")) float PaletteDistance = 3500.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="美术配色", meta=(DisplayName="美术配色强度", ToolTip="0至5。与材质lerp_fog共同控制配色表现，不修改原美术材质或草稿颜色。", ClampMin="0", ClampMax="5")) float PaletteStrength = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内受光", meta=(DisplayName="雾内环境底光", ToolTip="0至5。叠加到体积雾材质的微弱环境亮度。", ClampMin="0", ClampMax="5")) float AmbientIntensity = .25f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="旧版远景弱化起点", ToolTip="保留旧草稿兼容；新版统一空气雾使用真实体积覆盖权重，不再使用此距离。")) float FarStart = 2200.f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="旧版远景弱化完整距离", ToolTip="保留旧草稿兼容；新版统一空气雾使用真实体积覆盖权重，不再使用此距离。")) float FarEnd = 5000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景结构", meta=(DisplayName="雾中对比压低", ToolTip="0至1。按同一原生体积雾的实际覆盖权重，压低结构对比并向该处真实雾色收拢。", ClampMin="0", ClampMax="1")) float FarFlatten = .85f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景结构", meta=(DisplayName="结构描边强度", ToolTip="0至2。雾中主要形体边缘的表现强度。", ClampMin="0", ClampMax="2")) float StructureStrength = .4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景结构", meta=(DisplayName="细节保留强度", ToolTip="0至1。雾中细节结构保留的程度。", ClampMin="0", ClampMax="1")) float DetailStrength = .18f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景结构", meta=(DisplayName="屏幕描边宽度", ToolTip="像素。远端结构边缘的采样宽度。", ClampMin="0.25", ClampMax="8")) float EdgeWidth = 1.2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方向塑形", meta=(DisplayName="自动读取场景方向光", ToolTip="使用当前世界可见且有效的方向光塑造雾团；优先大气太阳的最低索引，再比较亮度与稳定路径。没有有效方向光时使用下方手动方向和颜色，不修改场景灯。")) bool bUseSceneDirectionalLight = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方向塑形", meta=(DisplayName="手动塑形光水平角", ToolTip="角度。手动模式或未找到有效场景方向光时使用的世界水平朝向，不移动场景灯光。", ClampMin="-180", ClampMax="180")) float FormLightYaw = -45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方向塑形", meta=(DisplayName="手动塑形光俯仰角", ToolTip="角度。手动模式或未找到有效场景方向光时使用的仰角。", ClampMin="-89", ClampMax="89")) float FormLightPitch = 35.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方向塑形", meta=(DisplayName="方向亮部强度", ToolTip="0至2。美术控制的雾团亮侧强度，自动读取场景光时也保留此乘数。", ClampMin="0", ClampMax="2")) float FormLightStrength = .4f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方向塑形", meta=(DisplayName="方向暗部强度", ToolTip="0至1。美术控制的雾团暗侧强度，自动读取场景光时也保留此乘数。", ClampMin="0", ClampMax="1")) float FormShadowStrength = .16f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="方向塑形", meta=(DisplayName="手动塑形光色", ToolTip="线性颜色。手动模式或未找到有效场景方向光时使用，不修改场景光源。")) FLinearColor FormLightColor = FLinearColor(.8f, .9f, 1.f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="粗笔刷与暗部排线强度", ToolTip="控制实际雾内和暗部的粗粝擦痕对比。0关闭笔刷，保留体积雾和结构塑形。", ClampMin="0", ClampMax="1")) float StrokeStrength = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="笔刷密度（越小越粗）", ToolTip="世界空间三平面体积采样。1对应12米纹理平铺，值越大笔刷越小；不随视口尺寸或镜头旋转变化。", ClampMin="0.25", ClampMax="16")) float StrokeScale = 3.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="雾内笔触流速", ToolTip="每秒。控制原生雾内笔触的缓慢流动，负值反向；0保持静止。", ClampMin="-5", ClampMax="5")) float StrokeFlowSpeed = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内笔触", meta=(DisplayName="雾内笔触扭曲", ToolTip="0至1。控制原生雾内笔触流动时的形状扰动。", ClampMin="0", ClampMax="1")) float StrokeWarp = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团边缘", meta=(DisplayName="清雾交界不规则度", ToolTip="与笔触扭曲共同控制视野交界的轻微起伏；静态厚雾预设保持克制。", ClampMin="0", ClampMax="1")) float BoundaryDetail = .45f;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(ClampMin="0.0001", ClampMax="1")) float BoundaryDetailScale = .02f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾团边缘", meta=(DisplayName="体积形体强度", ToolTip="0至2。控制雾团在空间中的明暗与体积起伏。", ClampMin="0", ClampMax="2")) float VolumeFormStrength = .5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内受光", meta=(DisplayName="雾散射反照率", ToolTip="0至1的线性颜色。控制体积雾接受光照后散射的比例。")) FLinearColor FogAlbedo = FLinearColor(.8f, .8f, .8f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内受光", meta=(DisplayName="手动环境底色", ToolTip="关闭美术材质配色时使用；开启时颜色来自原MI。", EditCondition="!bUseArtistPalette")) FLinearColor AmbientColor = FLinearColor(.016f, .035f, .065f, 1.f);
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="旧版手动远景雾色", ToolTip="保留旧草稿兼容；新版收尾修饰读取实际原生雾散射颜色，此字段不再单独控制收尾颜色。")) FLinearColor FarColor = FLinearColor(.008f, .016f, .032f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景结构", meta=(DisplayName="结构描边颜色", ToolTip="用于保留远端微弱结构的描边颜色。")) FLinearColor EdgeColor = FLinearColor(.002f, .004f, .008f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积质量", meta=(DisplayName="体积雾计算距离", ToolTip="厘米。原生体积雾覆盖范围；默认200米，为材质雾淡出留足距离。", ClampMin="5000", ClampMax="100000")) float ViewDistance = 20000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积质量", meta=(DisplayName="体积网格像素尺寸", ToolTip="像素。较小值提高体积雾分辨率并增加显存和耗时；全局设置由激活预览暂时接管。", ClampMin="4", ClampMax="64")) int32 GridPixelSize = 8;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积质量", meta=(DisplayName="体积网格深度层数", ToolTip="较高值提升深度细节并增加耗时。全局设置由激活预览暂时接管。", ClampMin="16", ClampMax="256")) int32 GridSizeZ = 128;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积质量", meta=(DisplayName="体积历史权重", ToolTip="0至0.99。较高值更平稳，也可能增加移动拖影。", ClampMin="0", ClampMax="0.99")) float HistoryWeight = .9f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="体积质量", meta=(DisplayName="局部灯体积边缘柔化", ToolTip="以体积网格单元为单位。0关闭；1柔化约一个单元的聚光灯和矩形灯体积边缘。全局设置由激活预览临时接管并恢复。", ClampMin="0", ClampMax="4")) float LightSoftFading = 1.f;

    void Sanitize();
};

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="YOG 原生空气雾"))
class YOGVISIONRUNTIME_API AYogAirFogVolume : public AActor
{
    GENERATED_BODY()
public:
    AYogAirFogVolume();
    virtual void Tick(float DeltaSeconds) override;
    virtual bool ShouldTickIfViewportsOnly() const override { return true; }
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;
    virtual void BeginDestroy() override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="空气雾") TObjectPtr<USceneComponent> Root;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="空气雾") TObjectPtr<UStaticMeshComponent> VolumeBounds;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="空气雾") TObjectPtr<UExponentialHeightFogComponent> Carrier;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气雾", meta=(DisplayName="游戏开始自动启用")) bool bAutoActivate = true;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="旧版跟随玩家", ToolTip="保留旧接口兼容；空气雾始终以当前镜头为基准，此值不影响表现。")) bool bFollowPlayer = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气雾", meta=(DisplayName="启用远景结构修饰")) bool bFinishEnabled = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="雾内受光", meta=(DisplayName="临时管理局部灯体积散射", ToolTip="只影响当前世界的可移动或固定局部灯：已有动态阴影的灯临时启用体积阴影，无阴影补光临时不参与体积散射。关闭、保存或结束时恢复，保留之后的人工修改；不会强开普通阴影。")) bool bManageLocalLightScattering = true;
    UPROPERTY(BlueprintReadWrite, Category="兼容参数", meta=(DisplayName="旧版观察者", ToolTip="保留旧接口兼容，不影响镜头空气雾。")) TWeakObjectPtr<AActor> FocusActor;
    UPROPERTY(BlueprintReadOnly, Category="兼容参数", meta=(DisplayName="旧版观察位置", ToolTip="保留旧接口兼容，不影响镜头空气雾。")) FVector FocusPosition = FVector::ZeroVector;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="美术配色", meta=(DisplayName="美术配色材质", ToolTip="只读取有效继承参数Fog Color 01、Fog Color 02与lerp_fog；不会修改原材质。留空使用插件自带兼容配色。")) TObjectPtr<UMaterialInterface> PaletteMaterial;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="空气雾", meta=(DisplayName="空气雾参数")) FYogAirFogParameters Parameters;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="空气雾", meta=(DisplayName="体积覆盖尺寸", ToolTip="厘米。普通Cube的完整尺寸，必须覆盖待观察走廊及远雾范围。")) FVector VolumeSize = FVector(60000.f, 60000.f, 24000.f);

    UFUNCTION(BlueprintCallable, Category="YOG|AirFog") bool SetAirFogEnabled(bool bEnabled);
    UFUNCTION(BlueprintPure, Category="YOG|AirFog") bool IsAirFogEnabled() const { return bDesiredEnabled; }
    UFUNCTION(BlueprintCallable, Category="YOG|AirFog") void ApplyParameters(FYogAirFogParameters NewParameters);
    UFUNCTION(BlueprintPure, Category="YOG|AirFog") FYogAirFogParameters GetParameters() const { return Parameters; }
    UFUNCTION(BlueprintCallable, Category="YOG|AirFog") void SetFocusActor(AActor* Actor);
    UFUNCTION(BlueprintCallable, Category="YOG|AirFog") void SetFocusPosition(FVector Position);
    UFUNCTION(BlueprintCallable, Category="YOG|AirFog") void SetPaletteMaterial(UMaterialInterface* Material);
    UFUNCTION(BlueprintPure, Category="YOG|AirFog") FString GetStatusText() const;
    UFUNCTION(BlueprintPure, Category="YOG|AirFog") FString GetFormLightStatusText() const;
    UFUNCTION(BlueprintPure, Category="YOG|AirFog") UMaterialInstanceDynamic* GetDensityMID() const { return DensityMID; }
    UFUNCTION(BlueprintPure, Category="YOG|AirFog") UMaterialInstanceDynamic* GetFinishMID() const;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="雾内视野") FLinearColor ScreenVisionSource = FLinearColor::Transparent;
    UPROPERTY(Transient, VisibleAnywhere, BlueprintReadOnly, Category="雾内视野") int32 ScreenLightCount = 0;
#if WITH_EDITOR
    bool HasTickedThisFrame() const { return LastUpdateFrame == GFrameCounter; }
    // Editor service supplies the active viewport even while it is occluded.
    void SetEditorCameraAnchor(const TOptional<FVector>& Location)
    {
        EditorCameraAnchor = Location.IsSet() && !Location->ContainsNaN() ? Location : TOptional<FVector>();
    }
#endif
    UFUNCTION(BlueprintCallable, Category="YOG|AirFog") void SuspendForSave(bool bSuspend);

private:
    bool ActivateAirFog();
    void DeactivateAirFog();
    bool PrepareResources();
    void UpdateNativeParameters();
    void UpdateMaterialParameters();
    void UpdateFinishRegistration();
    FVector ResolveCameraAnchor() const;
    void ResolveArtistPalette(FLinearColor& NearColor, FLinearColor& FarColor, float& FogMix);
    void ResolveDirectionalFormLight(float& Yaw, float& Pitch, FLinearColor& Color);
    void SuppressOtherHeightFog();
    void RestoreOtherHeightFog();
    void ManageLocalLightScattering();
    void RestoreLocalLightScattering();
    struct FLocalLightState
    {
        bool bOriginalVolumetricShadow = false;
        float OriginalScattering = 0.f;
        bool bLastVolumetricShadow = false;
        float LastScattering = 0.f;
        bool bOwnsVolumetricShadow = false;
        bool bOwnsScattering = false;
        bool bExternalVolumetricShadow = false;
        bool bExternalScattering = false;
    };
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> DensityMID;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> FinishMID;
    TMap<TWeakObjectPtr<UExponentialHeightFogComponent>, bool> OtherFogVisibility;
    TMap<TWeakObjectPtr<ULocalLightComponent>, FLocalLightState> LocalLightStates;
    // Per-field opt-outs survive internal/save suspension, but not an explicit off/on.
    TMap<TWeakObjectPtr<ULocalLightComponent>, uint8> LocalLightOptOuts;
    bool bDesiredEnabled = false;
    bool bActive = false;
    bool bFinishRegistered = false;
    UPROPERTY() bool bHasExplicitFocusPosition = false;
    bool bHasConsoleLease = false;
    bool bTriedDefaultPalette = false;
    bool bPaletteFallback = false;
    int32 SaveSuspensionDepth = 0;
    float OtherFogScanAccumulator = 0.f;
    float RetryAccumulator = 0.f;
    int32 LocalFogVolumeCount = 0;
    int32 ManagedLocalLightCount = 0;
    FString LastError;
    FString FormLightStatus;
#if WITH_EDITOR
    uint64 LastUpdateFrame = MAX_uint64;
    TOptional<FVector> EditorCameraAnchor;
#endif
};
