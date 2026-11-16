#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "YogVisionSettings.generated.h"

USTRUCT(BlueprintType)
struct YOGVISIONRUNTIME_API FYogVisionParameters
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="视野", meta=(DisplayName="基础视野半径", ToolTip="厘米。玩家中心的直接可见范围，受墙体阻挡；装备修改器在此基础上叠加。", ClampMin="0", ClampMax="20000")) float BaseSightRadius = 420.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵视感知半径", ToolTip="厘米。不受墙体阻挡的感知范围，只显示迷雾中的位置与移动，不揭示身份。", ClampMin="0", ClampMax="20000")) float SpiritRadius = 1800.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵视强度", ToolTip="0至1。控制感知灵体的显现强度，0关闭感知；可由装备修改器叠加。", ClampMin="0", ClampMax="1")) float SpiritStrength = .65f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵体雾团扰动", ToolTip="0至3。敌人在迷雾中搅动雾团的强度；0关闭这层扰动，不改变灵视范围。", ClampMin="0", ClampMax="3")) float SpiritDisturbance = 1.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵体雾团高度", ToolTip="厘米。控制移动扰动雾团的竖向厚度，使感知从地面延伸到身体附近。", ClampMin="10", ClampMax="2000")) float SpiritVolumeHeight = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="移动尾雾长度", ToolTip="厘米。敌人移动方向后方的拖曳雾团长度；0去除尾迹。", ClampMin="0", ClampMax="4000")) float SpiritTrailLength = 220.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="迷雾", meta=(DisplayName="迷雾浓度", ToolTip="0至1。控制视野外覆盖程度，较高值压低环境细节。", ClampMin="0", ClampMax="1")) float FogDensity = .78f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="迷雾", meta=(DisplayName="迷雾内部对比", ToolTip="0至1。雾内保留的光影对比，较低值形成弱对比空间。", ClampMin="0", ClampMax="1")) float FogContrast = .20f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="迷雾", meta=(DisplayName="迷雾底色亮度", ToolTip="0至2。调整迷雾的微弱底光，避免雾内结构完全变黑。", ClampMin="0", ClampMax="2")) float FogBrightness = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景雾", meta=(DisplayName="远景雾起始距离", ToolTip="厘米。从当前相机到场景表面的距离达到此值时，开始增加远景雾。", ClampMin="0", ClampMax="50000")) float DistanceFogStart = 1200.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景雾", meta=(DisplayName="远景雾完整距离", ToolTip="厘米。达到此距离时使用完整远景雾浓度；必须大于起始距离，系统会自动修正。", ClampMin="1", ClampMax="100000")) float DistanceFogEnd = 4000.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="远景雾", meta=(DisplayName="远景雾浓度", ToolTip="0至1。远处的额外雾覆盖，使远景逐步沉入雾色；0关闭距离雾。", ClampMin="0", ClampMax="1")) float DistanceFogDensity = .9f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="暗部", meta=(DisplayName="暗部阈值", ToolTip="0至1。低于此屏幕亮度的区域参与暗部雾化。", ClampMin="0", ClampMax="1")) float DarkThreshold = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="暗部", meta=(DisplayName="暗部雾化强度", ToolTip="0至1。控制视野范围内暗处的雾与笔触覆盖。", ClampMin="0", ClampMax="1")) float DarkFogStrength = .45f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="结构描边强度", ToolTip="0至2。控制深度与法线形成的结构边缘描绘。", ClampMin="0", ClampMax="2")) float EdgeStrength = .35f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="流动笔触强度", ToolTip="0至1。控制雾效与暗部中钢笔及油画纹理的可见程度。", ClampMin="0", ClampMax="1")) float StrokeStrength = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="屏幕空间笔触密度", ToolTip="0.25至8。以屏幕画面为基准的笔触重复密度，值越大图案越细密；不随地面世界坐标铺排。", ClampMin="0.25", ClampMax="8")) float StrokeScale = 2.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="笔触", meta=(DisplayName="笔触流动速度", ToolTip="0至10。纹理流动速度，0为静止笔触。", ClampMin="0", ClampMax="10")) float FlowSpeed = .12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="边界", meta=(DisplayName="雾退散过渡宽度", ToolTip="厘米。火光与迷雾交界处的柔和过渡宽度。", ClampMin="1", ClampMax="5000")) float BoundaryWidth = 180.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="边界", meta=(DisplayName="边界扰动强度", ToolTip="0至1。雾退散边界的流动扰动，较高值更活跃。", ClampMin="0", ClampMax="1")) float BoundaryNoise = .2f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="角色", meta=(DisplayName="玩家可读性提亮", ToolTip="0至1。为玩家附近提供轻微的画面提亮，过高会冲淡明暗关系。", ClampMin="0", ClampMax="1")) float PlayerLift = .18f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="迷雾", meta=(DisplayName="迷雾颜色", ToolTip="雾层基调。用于建立低对比冷色或暖色环境。")) FLinearColor FogColor = FLinearColor(.07f, .12f, .20f, 1.f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="灵视", meta=(DisplayName="灵体雾团颜色", ToolTip="灵视扰动雾团的基调；默认采用克制的蓝灰色，不显示敌人身体身份。")) FLinearColor SpiritColor = FLinearColor(.20f, .28f, .38f, 1.f);

    void Sanitize();
};

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="YOG 视野与灵视"))
class YOGVISIONRUNTIME_API UYogVisionSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UYogVisionSettings();
    UPROPERTY(Config, EditAnywhere, Category="启用", meta=(DisplayName="游戏中启用视野迷雾")) bool bEnabled = false;
    UPROPERTY(Config, EditAnywhere, Category="材质", meta=(DisplayName="视野后处理材质", AllowedClasses="/Script/Engine.MaterialInterface")) FSoftObjectPath PostProcessMaterial;
    UPROPERTY(Config, EditAnywhere, Category="美术参数", meta=(DisplayName="视野与灵视参数")) FYogVisionParameters Parameters;
    virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
