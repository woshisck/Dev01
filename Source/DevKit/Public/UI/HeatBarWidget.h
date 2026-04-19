#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HeatBarWidget.generated.h"

class UProgressBar;
class UImage;
class UBackpackGridComponent;

/**
 * 热度条 Widget
 *
 * 蓝图里只需要：
 * 1. 父类选 HeatBarWidget
 * 2. 在 Designer 里按如下层级放置控件（名称必须完全一致）：
 *
 *    [Overlay]                  ← 根节点（或 Canvas Panel）
 *      ├── [Image]  "HeatBarBG"   ← 背景色块，拉满 Overlay
 *      └── [ProgressBar] "HeatBar" ← 热度进度条，同样拉满；
 *                                     Style → Background → Tint Alpha 设为 0（透明）
 *
 * 颜色由 C++ 自动根据阶段切换，蓝图无需任何节点。
 *
 * 阶段颜色规则：
 *   Phase 0 → BG: 深灰,   Fill: #64FFC0
 *   Phase 1 → BG: #64FFC0, Fill: #FFCF72
 *   Phase 2+ → BG: #FFCF72, Fill: #FF403D
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UHeatBarWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // 自动绑定控件（名称必须与 Designer 里一致）
    // =========================================================

    /** 热度条背景色块 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> HeatBarBG;

    /** 热度进度条（Style 里的 Background Tint Alpha 设为 0）*/
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UProgressBar> HeatBar;

    UFUNCTION(BlueprintNativeEvent)
    void HandleHeatBarUpdate(float NormalizedHeat, int32 NewPhase);

    virtual void HandleHeatBarUpdate_Implementation(float NormalizedHeat, int32 NewPhase);

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    TWeakObjectPtr<UBackpackGridComponent> CachedBackpack;
    UBackpackGridComponent* GetBackpack() const;

    int32 CurrentPhase = -1;

    /** 立即刷新颜色和进度（NativeConstruct 也会调用一次以显示初始状态） */
    void RefreshDisplay(float NormalizedHeat, int32 Phase);
};
