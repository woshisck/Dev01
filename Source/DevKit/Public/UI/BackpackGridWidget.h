#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BackpackGridWidget.generated.h"

class UUniformGridPanel;
class USizeBox;
class URuneSlotWidget;
class UBackpackStyleDataAsset;
enum class EBackpackCellState : uint8;

/**
 * Backpack grid child widget used by WBP_BackpackGrid.
 *
 * The widget owns grid layout and cell refresh only. Heat preview controls are
 * intentionally not part of this widget; the active-zone visualization is
 * driven by the runtime backpack state passed to RefreshCells.
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UBackpackGridWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** WBP control variable name: BackpackGrid */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<UUniformGridPanel> BackpackGrid;

	/** WBP control variable name: GridSizeBox */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> GridSizeBox;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
	TSubclassOf<URuneSlotWidget> RuneSlotClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
	TObjectPtr<UBackpackStyleDataAsset> StyleDA;

	int32 CachedGridW = 5;
	int32 CachedGridH = 5;

	void BuildGrid(UBackpackGridComponent* InBackpack);

	void RefreshCells(UBackpackGridComponent* Backpack,
					  FIntPoint SelectedCell,
					  FIntPoint HoverCell,
					  FIntPoint GrabbedFromCell,
					  bool bGrabbing,
					  int32 PreviewPhase = -1);

	bool GetCellAtScreenPos(const FVector2D& AbsPos, UBackpackGridComponent* Backpack,
							int32& OutCol, int32& OutRow) const;

	void FlashAndShakeCell(int32 Col, int32 Row);

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
