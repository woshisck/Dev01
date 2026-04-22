#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LiquidHealthBarWidget.generated.h"

class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 液态血条 Widget — C++ 基类（WBP_LiquidHealthBar 的父类）
 *
 * 职责分工：
 *   C++       — 创建 DynMat、写入材质参数、驱动晃动阻尼动画（NativeTick）
 *   Blueprint — 搭建控件层级（LiquidFillImage / FrameImage）、指定材质资源、配置颜色
 *
 * 用法：
 *   1. 新建 WBP_LiquidHealthBar，Parent Class 选 LiquidHealthBarWidget
 *   2. 按层级规格搭建两个 Image（LiquidFillImage / FrameImage）
 *   3. Details → 血条|材质 → 填入 M_LiquidHealthBar
 *   4. Details → 血条|颜色 → 按需调整液体颜色和高光
 *   5. 运行时调用 SetHealthPercent(float) 驱动血量变化
 *
 * 配套材质：M_LiquidHealthBar（User Interface / Translucent）
 *   Custom Node Include: /Project/LiquidHealthBar.ush
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API ULiquidHealthBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // 材质源
    // =========================================================

    /** 液体材质（M_LiquidHealthBar），NativeConstruct 时自动创建 DMI */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "血条|材质")
    TObjectPtr<UMaterialInterface> LiquidMaterial;

    // =========================================================
    // 颜色
    // =========================================================

    /** 液体深处/底部颜色（较暗；如深红 0.35,0.02,0.02） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "血条|颜色")
    FLinearColor LiquidColorDeep = FLinearColor(0.35f, 0.02f, 0.02f, 1.f);

    /** 液体表面/顶部颜色（较亮；如亮红 0.75,0.08,0.05） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "血条|颜色")
    FLinearColor LiquidColorSurface = FLinearColor(0.75f, 0.08f, 0.05f, 1.f);

    /** 液面高光线颜色（偏暖；如橙白 1.0,0.65,0.4） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "血条|颜色")
    FLinearColor GlintColor = FLinearColor(1.f, 0.65f, 0.4f, 1.f);

    // =========================================================
    // 晃动参数
    // =========================================================

    /**
     * 振荡频率（弧度/秒）
     * 粘稠液体：6-10（约 1-1.5 Hz）；水：15-25（约 2-4 Hz）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "血条|晃动",
              meta = (ClampMin = "1.0", ClampMax = "25.0"))
    float OscFrequency = 9.f;

    /**
     * 阻尼衰减率（指数衰减的指数系数）
     * 粘稠液体：1.2-2.0；水：3.0-5.0
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "血条|晃动",
              meta = (ClampMin = "0.3", ClampMax = "8.0"))
    float DampingRate = 1.6f;

    /**
     * 最大晃动幅度（UV 空间；血量变化越大晃动越猛，但不超过此值）
     * 推荐 0.03-0.08；过大会使液面超出管道边缘
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "血条|晃动",
              meta = (ClampMin = "0.0", ClampMax = "0.15"))
    float MaxSloshAmplitude = 0.055f;

    // =========================================================
    // 接口
    // =========================================================

    /**
     * 设置当前血量百分比（0-1）。
     * 血量变化时自动触发晃动动画；血量变化越大，初始晃动幅度越大。
     */
    UFUNCTION(BlueprintCallable, Category = "血条")
    void SetHealthPercent(float NewPct);

    /** 获取当前血量百分比 */
    UFUNCTION(BlueprintPure, Category = "血条")
    float GetHealthPercent() const { return CurrentPct; }

    /** 将颜色参数重新写入 DynMat（在编辑器修改颜色后调用可实时预览） */
    UFUNCTION(BlueprintCallable, Category = "血条")
    void ApplyColors();

protected:
    virtual void NativeConstruct() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    // =========================================================
    // BindWidget（名称必须与 WBP 控件名完全一致）
    // =========================================================

    /** 液体材质渲染层（位于 FrameImage 下层） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UImage> LiquidFillImage;

private:
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> LiquidDynMat;

    float CurrentPct = 1.f;   // 当前血量（0-1）
    float SloshAmp   = 0.f;   // 当前晃动幅度
    float SloshPh    = 0.f;   // 当前晃动相位（弧度，持续累加）
    bool  bNeedsTick = false; // 仅在晃动激活时 Tick，避免每帧 DMI 写入开销
};
