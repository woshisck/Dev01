#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BackpackStyleDataAsset.generated.h"

/**
 * 背包 UI 视觉配置 DataAsset（DA_BackpackStyle）
 *
 * 纹理/材质字段留空时自动回退到颜色显示，无需额外设置。
 * 激活区动画材质在 Material Editor 里用 Time 节点驱动，无需 C++ Tick。
 */
UCLASS(BlueprintType)
class DEVKIT_API UBackpackStyleDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    // =========================================================
    // 格子颜色（无纹理时使用）
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor EmptyColor = FLinearColor(0.40f, 0.40f, 0.42f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor EmptyActiveColor = FLinearColor(0.15f, 0.35f, 0.75f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor OccupiedActiveColor = FLinearColor(0.10f, 0.65f, 1.00f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor OccupiedInactiveColor = FLinearColor(0.18f, 0.10f, 0.25f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor SelectedColor = FLinearColor(1.00f, 0.82f, 0.10f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor HoverColor = FLinearColor(0.10f, 0.80f, 0.20f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor GrabbedSourceColor = FLinearColor(1.00f, 0.85f, 0.10f, 1.f);

    // =========================================================
    // 格子背景纹理（有纹理时覆盖颜色，颜色作为 Tint 叠加）
    // =========================================================

    /** 普通空格底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellEmptyTexture;

    /** 激活区空格底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellActiveTexture;

    /** 激活区有符文底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellOccupiedActiveTexture;

    /** 非激活区有符文底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellOccupiedInactiveTexture;

    /** 选中状态底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellSelectedTexture;

    /** 拖拽悬浮目标格底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellHoverTexture;

    /** 被抓起的源格底图 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子纹理")
    TObjectPtr<UTexture2D> CellGrabbedSourceTexture;

    // =========================================================
    // 激活区光效/扰动动画材质
    // =========================================================

    /**
     * 热度激活区特效材质（叠加在格子上层）。
     * 在 Material Editor 里用 Time 节点驱动动画，无需 C++ Tick。
     * 留空时不显示特效层。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "激活区特效")
    TObjectPtr<UMaterialInterface> ActiveZoneAnimMaterial;

    /** 激活区特效层透明度（0=不显示，1=完全不透明） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "激活区特效",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ActiveZoneOverlayOpacity = 0.6f;

    /**
     * 非聚焦区格子透明度。
     * 热度检视模式下当前阶段以外的区域使用此值；
     * 未开启检视时，所有无符文格子也使用此值。
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "激活区特效",
              meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float InactiveZoneOpacity = 0.35f;

    // =========================================================
    // 热度阶段叠加区颜色（热度1最上层，热度3最底层）
    // =========================================================

    // Phase 1=冷白/淡蓝  Phase 2=暖橙  Phase 3=金色
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "热度阶段颜色")
    FLinearColor HeatZone0Color = FLinearColor(0.45f, 0.60f, 1.00f, 1.f);  // Phase1 冷白/淡蓝

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "热度阶段颜色")
    FLinearColor HeatZone1Color = FLinearColor(1.00f, 0.50f, 0.08f, 1.f);  // Phase2 暖橙

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "热度阶段颜色")
    FLinearColor HeatZone2Color = FLinearColor(1.00f, 0.82f, 0.10f, 1.f);  // Phase3 金色

    // =========================================================
    // 待放置区颜色
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "待放置区颜色")
    FLinearColor PendingHasRuneColor = FLinearColor(0.12f, 0.08f, 0.22f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "待放置区颜色")
    FLinearColor PendingEmptyColor = FLinearColor(0.40f, 0.40f, 0.42f, 1.f);

    // =========================================================
    // 格子尺寸
    // =========================================================

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸",
              meta = (ClampMin = "32", ClampMax = "128"))
    float CellSize = 64.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸",
              meta = (ClampMin = "0", ClampMax = "16"))
    float CellPadding = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸",
              meta = (ClampMin = "0", ClampMax = "20"))
    float CellCornerRadius = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸",
              meta = (ClampMin = "0", ClampMax = "24"))
    float IconPadding = 6.f;
};
