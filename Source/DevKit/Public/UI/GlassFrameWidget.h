#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GlassFrameWidget.generated.h"

class UImage;
class UBackgroundBlur;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * 液态玻璃框 — C++ 基类（WBP_GlassFrame 的父类）
 *
 * 职责分工：
 *   C++   — 创建 DynMat、写入材质参数、控制 BackgroundBlur 强度
 *   Blueprint — 搭建控件层级（BackgroundBlur / Image / NamedSlot）、指定材质资源、配置尺寸
 *
 * 控件层级（Blueprint Designer 里搭建）：
 *   [Root] Overlay
 *   ├── BackgroundBlur   命名 "GlassBG"          ← 毛玻璃模糊背景
 *   ├── Image            命名 "GlassBorderImage"  ← 玻璃边框材质（SDF + 菲涅尔 + 炫彩）
 *   └── NamedSlot        命名 "Content"           ← 实际内容（格子/图标/HUD缩略图）
 *
 * 用法：
 *   1. 新建 WBP_GlassFrame，Parent Class 选 GlassFrameWidget
 *   2. 按上方层级搭建控件，名称必须完全一致
 *   3. Details → 玻璃框|材质 → 填入 MI_GlassFrame（或 M_GlassFrame）
 *   4. 根据使用场景调整 BlurStrength / CornerRadius 等参数
 *   5. NativeConstruct 时自动创建 DynMat 并应用所有参数
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UGlassFrameWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // 材质源
    // =========================================================

    /** 玻璃边框材质（M_GlassFrame 或已填好参数的 MI）*/
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "玻璃框|材质")
    TObjectPtr<UMaterialInterface> GlassBorderMaterial;

    // =========================================================
    // 模糊
    // =========================================================

    /** BackgroundBlur 强度（背包大框≈14，HUD小图标≈6） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "玻璃框|模糊",
              meta = (ClampMin = "0", ClampMax = "50"))
    float BlurStrength = 14.f;

    // =========================================================
    // 边框形状（传给材质 Custom 节点）
    // =========================================================

    /** 圆角半径（UV 空间，0=直角，0.5=圆形） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "玻璃框|边框",
              meta = (ClampMin = "0", ClampMax = "0.5"))
    float CornerRadius = 0.06f;

    /** 边框宽度（UV 空间） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "玻璃框|边框",
              meta = (ClampMin = "0", ClampMax = "0.2"))
    float BorderWidth = 0.025f;

    // =========================================================
    // 菲涅尔
    // =========================================================

    /** 菲涅尔集中程度（值越大越集中在角落，推荐 2–4） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "玻璃框|菲涅尔",
              meta = (ClampMin = "0.5", ClampMax = "8.0"))
    float FresnelPower = 2.5f;

    // =========================================================
    // 炫彩
    // =========================================================

    /** 炫彩强度（建议 0.1–0.25，太高则失去神秘感） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "玻璃框|炫彩",
              meta = (ClampMin = "0", ClampMax = "1"))
    float IridIntensity = 0.18f;

    /** 炫彩流动速度（建议保持较小，0.02–0.08） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "玻璃框|炫彩",
              meta = (ClampMin = "0", ClampMax = "1"))
    float IridSpeed = 0.04f;

    // =========================================================
    // 接口
    // =========================================================

    /** 将所有参数写入 DynMat + 更新 BackgroundBlur 强度。运行时调整参数后调用。 */
    UFUNCTION(BlueprintCallable, Category = "玻璃框")
    void ApplyGlassStyle();

    /** 获取运行时 DynMat（供 BP 做额外参数扩展）。NativeConstruct 之前为 null。 */
    UFUNCTION(BlueprintPure, Category = "玻璃框")
    UMaterialInstanceDynamic* GetGlassDynMat() const { return GlassDynMat; }

protected:
    virtual void NativeConstruct() override;

    // =========================================================
    // Designer 绑定（BindWidgetOptional：名称不一致时跳过，不崩溃）
    // =========================================================

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UBackgroundBlur> GlassBG;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> GlassBorderImage;

private:
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> GlassDynMat;
};
