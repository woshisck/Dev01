#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BackpackGridWidget.generated.h"

class UUniformGridPanel;
class URuneSlotWidget;
class UBackpackGridComponent;
class UBackpackStyleDataAsset;
enum class EBackpackCellState : uint8;

/**
 * 主格子子 Widget（WBP_BackpackGrid）。
 * 负责格子布局创建（BuildGrid）和视觉刷新（RefreshCells）。
 * 每格实例化一个 WBP_RuneSlot（URuneSlotWidget），由其负责格子动效。
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

    /** 格子实例类（填 WBP_RuneSlot）。留空时使用基类 C++ 默认实现（纯色格子，无动效）。 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
    TSubclassOf<URuneSlotWidget> RuneSlotClass;

    /** 视觉风格配置（填 DA_BackpackStyle） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TObjectPtr<UBackpackStyleDataAsset> StyleDA;

    /** 缓存格子数，BuildGrid 后更新，供坐标计算使用 */
    int32 CachedGridW = 5;
    int32 CachedGridH = 5;

    /**
     * 动态创建格子并实例化 RuneSlotWidget。
     * 由 BackpackScreenWidget.NativeConstruct 调用一次。
     */
    void BuildGrid(UBackpackGridComponent* InBackpack);

    /**
     * 刷新全部格子的视觉状态和符文图标。
     * BackpackScreenWidget.OnGridNeedsRefresh 调用此函数。
     */
    void RefreshCells(UBackpackGridComponent* Backpack,
                      FIntPoint SelectedCell,
                      FIntPoint HoverCell,
                      FIntPoint GrabbedFromCell,
                      bool bGrabbing);

    /**
     * 屏幕绝对坐标 → 格子坐标。失败返回 false。
     */
    bool GetCellAtScreenPos(const FVector2D& AbsPos, UBackpackGridComponent* Backpack,
                            int32& OutCol, int32& OutRow) const;

    /** 返回 BackpackGrid 的 CachedGeometry，供手柄图标定位使用 */
    FGeometry GetGridGeometry() const;

private:
    /** 每格对应的 RuneSlotWidget 实例（Row*GridW + Col 索引） */
    UPROPERTY()
    TArray<TObjectPtr<URuneSlotWidget>> CachedSlots;
};
