#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BackpackGridWidget.generated.h"

class UUniformGridPanel;
class USizeBox;
class URuneSlotWidget;
class UBackpackStyleDataAsset;
class UButton;
class UTextBlock;
enum class EBackpackCellState : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHeatPhaseButtonClicked, int32, Phase);

/**
 * 主格子子 Widget（WBP_BackpackGrid）。
 * 负责格子布局创建（BuildGrid）和视觉刷新（RefreshCells）。
 * 每格实例化一个 WBP_RuneSlot（URuneSlotWidget），由其负责格子动效。
 * 符文包围框通过 NativePaint 直接绘制，不需要额外的 CanvasPanel 覆盖层。
 *
 * Designer 里放一个 UniformGridPanel，命名 "BackpackGrid"。
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBackpackGridWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Designer 中放 UniformGridPanel，命名 "BackpackGrid" */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> BackpackGrid;

    /** 包裹 UniformGridPanel 的 SizeBox，命名 "GridSizeBox"，BuildGrid 时动态设置精确像素尺寸以保证格子 1:1 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> GridSizeBox;

    /** 格子实例类（填 WBP_RuneSlot）。留空时使用基类 C++ 默认实现（纯色格子，无动效）。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
    TSubclassOf<URuneSlotWidget> RuneSlotClass;

    /** 视觉风格配置（填 DA_BackpackStyle） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TObjectPtr<UBackpackStyleDataAsset> StyleDA;

    /** 热度阶段预览点按钮（放在 WBP_BackpackGrid 内，命名 "HeatPhaseDot0/1/2"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> HeatPhaseDot0;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> HeatPhaseDot1;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> HeatPhaseDot2;

    /** 手柄模式提示文本（命名 "GamepadHintLabel"） */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> GamepadHintLabel;

    /** 用户点击热度点时广播（BackpackScreenWidget 订阅后更新 PreviewPhase） */
    UPROPERTY(BlueprintAssignable, Category = "Backpack|HeatPreview")
    FOnHeatPhaseButtonClicked OnHeatPhaseButtonClicked;

    /** 缓存格子数，BuildGrid 后更新，供坐标计算使用 */
    int32 CachedGridW = 5;
    int32 CachedGridH = 5;

    /**
     * 动态创建格子并实例化 RuneSlotWidget。
     * 由 BackpackScreenWidget.NativeConstruct 调用一次。
     */
    void BuildGrid();

    /** 刷新三个点按钮颜色和手柄提示可见性，由 BackpackScreenWidget 调用 */
    void RefreshHeatPhaseButtons(int32 PreviewPhase, bool bIsGamepadMode);

    /**
     * 刷新全部格子的视觉状态和符文图标。
     * BackpackScreenWidget.OnGridNeedsRefresh 调用此函数。
     */
    void RefreshCells(FIntPoint SelectedCell,
                      FIntPoint HoverCell,
                      FIntPoint GrabbedFromCell,
                      bool bGrabbing,
                      int32 PreviewPhase = -1);

    /**
     * 屏幕绝对坐标 → 格子坐标。失败返回 false。
     */
    bool GetCellAtScreenPos(const FVector2D& AbsPos, int32& OutCol, int32& OutRow) const;

    /** 放置失败反馈：指定格子红闪+抖动 */
    void FlashAndShakeCell(int32 Col, int32 Row);

    /** 返回 BackpackGrid 的 CachedGeometry，供手柄图标定位使用 */
    FGeometry GetGridGeometry() const;

protected:
    virtual void NativeConstruct() override;
    virtual int32 NativePaint(const FPaintArgs& Args,
                              const FGeometry& AllottedGeometry,
                              const FSlateRect& MyCullingRect,
                              FSlateWindowElementList& OutDrawElements,
                              int32 LayerId,
                              const FWidgetStyle& InWidgetStyle,
                              bool bParentEnabled) const override;

private:
    /** 每格对应的 RuneSlotWidget 实例（Row*GridW + Col 索引） */
    UPROPERTY()
    TArray<TObjectPtr<URuneSlotWidget>> CachedSlots;

    /** NativePaint 绘制所需缓存数据（RefreshCells 更新） */
    FGuid  CachedSelectedGuid;
    FGuid  CachedHoverGuid;
    bool   CachedBGrabbing    = false;
    FIntPoint CachedGrabbedFromCell = FIntPoint(-1, -1);

    UFUNCTION() void OnHeatPhaseDot0Clicked();
    UFUNCTION() void OnHeatPhaseDot1Clicked();
    UFUNCTION() void OnHeatPhaseDot2Clicked();
};
