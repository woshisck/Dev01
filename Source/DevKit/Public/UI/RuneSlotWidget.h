#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/BackpackScreenWidget.h"   // EBackpackCellState
#include "RuneSlotWidget.generated.h"

class UImage;
class UBackpackStyleDataAsset;

/**
 * 单个背包格子 Widget（WBP_RuneSlot）。
 *
 * C++ 负责：格子背景颜色/纹理、符文 Icon 显示、激活区特效层显隐。
 * Blueprint 负责：各状态切换动效（选中光晕、拖拽弹跳、放置闪烁等）。
 *
 * Designer 层级：
 *   [Root] Overlay / Canvas Panel
 *   ├── Image  命名 "CellBG"           ← 格子背景（颜色/纹理，C++ 控制）
 *   ├── Image  命名 "ActiveZoneOverlay" ← 激活区特效层（动画材质，C++ 控制显隐）
 *   └── Image  命名 "CellIcon"          ← 符文图标（C++ 控制）
 *
 * 以上三个 BindWidgetOptional，缺少时 C++ 跳过，不会报错。
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API URuneSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // =========================================================
    // Designer 绑定控件（BindWidgetOptional：不放也不报错）
    // =========================================================

    /** 格子背景图（颜色 or 纹理，C++ 每次刷新时更新） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CellBG;

    /**
     * 激活区特效叠加层（绑定动画材质后自动播放光效/扰动）。
     * 只在激活区格子上显示，非激活格子 Collapsed。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> ActiveZoneOverlay;

    /** 符文图标（C++ 根据格子内容设置纹理和透明度） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> CellIcon;

    /**
     * 选中边框叠加层（在 Designer 中放在 CellIcon 上方，命名 "SelectionBorder"）。
     * 选中时显示黄色边框，不选中时 Collapsed。
     * 推荐使用带透明中心的边框贴图，或直接用 FillImage 纯色（alpha 0.25 左右）。
     */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> SelectionBorder;

    // =========================================================
    // C++ 驱动接口（由 BackpackGridWidget 调用）
    // =========================================================

    /**
     * 设置格子视觉状态（颜色/纹理 + 激活区覆盖层）。
     * 仅在状态发生变化时才触发对应 BlueprintImplementableEvent，避免每帧重复调用动画。
     */
    void SetSlotState(EBackpackCellState State,
                      bool bSelected,
                      bool bHovered,
                      bool bGrabbing,
                      const UBackpackStyleDataAsset* Style,
                      float ZoneOpacity = 1.f,
                      bool bGlowZone = false);

    /**
     * 设置符文图标。Tex=null 时隐藏图标。
     * Opacity：被手柄抓起时传 0.3，正常为 1.0。
     */
    void SetRuneIcon(UTexture2D* Tex, float Opacity);

    /**
     * 绑定激活区动画材质（BuildGrid 时由 BackpackGridWidget 调用一次）。
     * 材质内部用 Time 节点驱动，无需 C++ Tick 更新参数。
     */
    void SetActiveZoneMaterial(UMaterialInstanceDynamic* DynMat);

    /** 放置失败时：红闪 + 水平抖动（阻尼正弦，约 0.4s） */
    void ShakeAndFlash();

    // =========================================================
    // Blueprint 动效钩子（实现动效时在 BP 里 override 这些事件）
    // =========================================================

    /** 格子基础状态变化（空/激活区/有符文等），可驱动背景色彩动画 */
    UFUNCTION(BlueprintImplementableEvent, Category = "RuneSlot|Animation")
    void OnCellStateChanged(EBackpackCellState NewState);

    /** 选中状态变化（true=选中黄框出现，false=消失） */
    UFUNCTION(BlueprintImplementableEvent, Category = "RuneSlot|Animation")
    void OnSelectedChanged(bool bIsSelected);

    /** 拖拽悬浮目标变化（true=鼠标/手柄拖拽悬浮到此格） */
    UFUNCTION(BlueprintImplementableEvent, Category = "RuneSlot|Animation")
    void OnHoveredChanged(bool bIsHovered);

    /**
     * 手柄抓取状态变化（true=此格符文被手柄抓起，变暗+图标半透明）。
     * 鼠标拖拽由 GrabbedRuneIcon 直接驱动，不走此事件。
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "RuneSlot|Animation")
    void OnGrabbingChanged(bool bIsGrabbing);

    /** 符文图标变化（可在 BP 里播放"符文出现"动画） */
    UFUNCTION(BlueprintImplementableEvent, Category = "RuneSlot|Animation")
    void OnRuneIconSet(UTexture2D* Icon, float Opacity);

protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
    // 上一帧状态缓存，避免无变化时重复触发 BP 事件
    EBackpackCellState PrevState    = EBackpackCellState::Empty;
    bool bPrevSelected  = false;
    bool bPrevHovered   = false;
    bool bPrevGrabbing  = false;

    // 抖动状态
    bool  bShaking      = false;
    float ShakeElapsed  = 0.f;
    static constexpr float ShakeDuration = 0.4f;
};
