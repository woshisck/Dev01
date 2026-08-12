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
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar|Material")
    TObjectPtr<UMaterialInterface> LiquidMaterial;

    /**
     * Leak layer material (M_LiquidHealthBarDrips). Optional: the whole leak effect is
     * skipped silently when DripImage is absent, so existing WBPs keep working.
     * Companion shader: /Project/LiquidHealthBarDrips.ush
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health Bar|Material")
    TObjectPtr<UMaterialInterface> DripMaterial;

    // =========================================================
    // 颜色
    // =========================================================

    /** 液体深处/底部颜色（较暗；如深红 0.35,0.02,0.02） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Colors")
    FLinearColor LiquidColorDeep = FLinearColor(0.35f, 0.02f, 0.02f, 1.f);

    /** 液体表面/顶部颜色（较亮；如亮红 0.75,0.08,0.05） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Colors")
    FLinearColor LiquidColorSurface = FLinearColor(0.75f, 0.08f, 0.05f, 1.f);

    /** 液面高光线颜色（偏暖；如橙白 1.0,0.65,0.4） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Colors")
    FLinearColor GlintColor = FLinearColor(1.f, 0.65f, 0.4f, 1.f);

    /** Falling droplet color. Slightly brighter than the deep liquid reads best. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Colors")
    FLinearColor DripColor = FLinearColor(0.55f, 0.03f, 0.02f, 1.f);

    /** Wet residue color smeared under the breach. Keep this darker than DripColor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Colors")
    FLinearColor StainColor = FLinearColor(0.18f, 0.01f, 0.01f, 1.f);

    // =========================================================
    // Leak
    // =========================================================

    /**
     * Health fraction at or below which the leak ramps in. Set 0 to disable leaking.
     * At LeakThreshold the intensity is 0; at 0 health it is 1.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Leak",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LeakThreshold = 0.35f;

    /**
     * Shaping exponent on the leak ramp. >1 keeps the effect subtle until health is
     * genuinely low, which avoids the bar looking broken during normal play.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Leak",
              meta = (ClampMin = "1.0", ClampMax = "4.0"))
    float LeakCurveExponent = 2.f;

    // =========================================================
    // 晃动参数
    // =========================================================

    /**
     * 振荡频率（弧度/秒）
     * 粘稠液体：6-10（约 1-1.5 Hz）；水：15-25（约 2-4 Hz）
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Wobble",
              meta = (ClampMin = "1.0", ClampMax = "25.0"))
    float OscFrequency = 9.f;

    /**
     * 阻尼衰减率（指数衰减的指数系数）
     * 粘稠液体：1.2-2.0；水：3.0-5.0
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Wobble",
              meta = (ClampMin = "0.3", ClampMax = "8.0"))
    float DampingRate = 1.6f;

    /**
     * 最大晃动幅度（UV 空间；血量变化越大晃动越猛，但不超过此值）
     * 推荐 0.03-0.08；过大会使液面超出管道边缘
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Wobble",
              meta = (ClampMin = "0.0", ClampMax = "0.15"))
    float MaxSloshAmplitude = 0.055f;

    // =========================================================
    // 布局映射
    // =========================================================

    /**
     * 帧纹理透明管道区域右边界（UV.x，0-1）。
     * 液体 100% 时填充到此位置；右侧为不透明水晶端头，不需要填充。
     * 根据 FrameImage 纹理透明区域实测调整（默认 0.5）。
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar|Layout",
              meta = (ClampMin = "0.1", ClampMax = "1.0"))
    float FillWindowEnd = 0.5f;

    // =========================================================
    // 接口
    // =========================================================

    /**
     * 设置当前血量百分比（0-1）。
     * 血量变化时自动触发晃动动画；血量变化越大，初始晃动幅度越大。
     */
    UFUNCTION(BlueprintCallable, Category = "Health Bar")
    void SetHealthPercent(float NewPct);

    /** 获取当前血量百分比 */
    UFUNCTION(BlueprintPure, Category = "Health Bar")
    float GetHealthPercent() const { return CurrentPct; }

    /** Current leak strength (0-1) derived from health. Useful for driving extra VFX/SFX. */
    UFUNCTION(BlueprintPure, Category = "Health Bar|Leak")
    float GetLeakIntensity() const;

    /** 将颜色参数重新写入 DynMat（在编辑器修改颜色后调用可实时预览） */
    UFUNCTION(BlueprintCallable, Category = "Health Bar")
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

    /**
     * Leak layer. Optional so existing WBPs keep loading. Must extend below the tube
     * art, otherwise droplets have nowhere to fall and stay invisible.
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> DripImage;

private:
    bool EnsureDynamicMaterial();
    bool EnsureDripMaterial();

    // Pushes LeakIntensity / LeakX to the drip DMI. No-op without a drip layer.
    void UpdateLeakParams();

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> LiquidDynMat;

    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> DripDynMat;

    float CurrentPct = 1.f;   // 当前血量（0-1）
    float SloshAmp   = 0.f;   // 当前晃动幅度
    float SloshPh    = 0.f;   // 当前晃动相位（弧度，持续累加）
    bool  bNeedsTick = false; // 仅在晃动激活时 Tick，避免每帧 DMI 写入开销
    bool  bLoggedMissingMaterialSetup = false;
    bool  bTriedDripFallbackLoad = false; // Only probe the fallback drip asset once
};
