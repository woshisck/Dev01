#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BackpackStyleDataAsset.generated.h"

/**
 * 背包 UI 视觉配置 DataAsset
 *
 * 使用方法：
 *   1. 在 Content Browser 右键 → Miscellaneous → Data Asset → 选 BackpackStyleDataAsset
 *   2. 命名为 DA_BackpackStyle（建议放 Content/UI/Playtest_UI/ 下）
 *   3. 在 WBP_BackpackScreen 的 Details 面板 → Style DA 字段填入该资产
 *   4. 修改颜色和尺寸后，重新打开背包即可看到效果（无需重新编译）
 */
UCLASS(BlueprintType)
class DEVKIT_API UBackpackStyleDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    // =========================================================
    // 主格子颜色
    // =========================================================

    /** 空格（未激活区） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor EmptyColor = FLinearColor(0.40f, 0.40f, 0.42f, 1.f);

    /** 空格（激活区，热度足够时扩展区域） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor EmptyActiveColor = FLinearColor(0.15f, 0.35f, 0.75f, 1.f);

    /** 有符文 + 已激活 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor OccupiedActiveColor = FLinearColor(0.10f, 0.55f, 1.00f, 1.f);

    /** 有符文 + 未激活（超出激活区） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor OccupiedInactiveColor = FLinearColor(0.55f, 0.35f, 0.05f, 1.f);

    /** 选中高亮 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor SelectedColor = FLinearColor(1.00f, 0.82f, 0.10f, 1.f);

    /** 拖拽悬浮目标格 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor HoverColor = FLinearColor(0.10f, 0.80f, 0.20f, 1.f);

    /** 被抓起的源格（半暗） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子颜色")
    FLinearColor GrabbedSourceColor = FLinearColor(0.25f, 0.15f, 0.03f, 1.f);

    // =========================================================
    // 左侧待放置区颜色
    // =========================================================

    /** 待放置区：有符文的槽 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "待放置区颜色")
    FLinearColor PendingHasRuneColor = FLinearColor(0.12f, 0.08f, 0.22f, 1.f);

    /** 待放置区：空槽（与主格子 EmptyColor 保持一致即可） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "待放置区颜色")
    FLinearColor PendingEmptyColor = FLinearColor(0.40f, 0.40f, 0.42f, 1.f);

    // =========================================================
    // 格子尺寸
    // =========================================================

    /** 每格宽/高（px），主格子和待放置格共用 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸", meta = (ClampMin = "32", ClampMax = "128"))
    float CellSize = 64.f;

    /** 格子间距（px） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸", meta = (ClampMin = "0", ClampMax = "16"))
    float CellPadding = 2.f;

    /** 格子圆角半径（0 = 直角） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸", meta = (ClampMin = "0", ClampMax = "20"))
    float CellCornerRadius = 3.f;

    /** 图标内边距（图标与格子边缘的距离） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "格子尺寸", meta = (ClampMin = "0", ClampMax = "24"))
    float IconPadding = 6.f;
};
