#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "RuneTooltipWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * 符文悬浮 Tooltip 基类
 *
 * 蓝图使用步骤：
 *   1. 新建 Widget Blueprint，父类选 RuneTooltipWidget
 *   2. Designer 中放置（名称必须完全一致）：
 *        [TextBlock] "TooltipName"   ← 符文名称
 *        [TextBlock] "TooltipDesc"   ← 符文描述
 *        [TextBlock] "TooltipType"   ← 符文类型（增益/减益/无）
 *        [Image]     "TooltipIcon"   ← 符文图标（可选）
 *   3. 将该 BP 作为子控件加入 WBP_BackpackScreen 的 Overlay 顶层
 *        命名为 "RuneTooltip"，初始可见性 Collapsed
 *   4. ShowRuneInfo 时自动变为 Visible，HideTooltip 时变为 Collapsed
 *   5. BackpackScreenWidget 会通过 SetRenderTranslation 定位
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API URuneTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ── 自动绑定控件 ────────────────────────────────────────────
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TooltipName;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TooltipDesc;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TooltipType;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> TooltipIcon;

    // ── 操作 ─────────────────────────────────────────────────────

    /** 显示符文信息并使 Tooltip 可见 */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void ShowRuneInfo(const FRuneInstance& Rune);

    /** 隐藏 Tooltip */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void HideTooltip();

    /** 蓝图可重写：在 ShowRuneInfo 后调用，用于自定义动画或额外逻辑 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
    void OnTooltipShown(const FRuneInstance& Rune);

    /** 蓝图可重写：在 HideTooltip 后调用 */
    UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
    void OnTooltipHidden();
};
