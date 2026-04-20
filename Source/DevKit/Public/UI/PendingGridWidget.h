#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "PendingGridWidget.generated.h"

class UUniformGridPanel;
class USizeBox;
class UImage;
class UBackpackStyleDataAsset;

/**
 * 左侧待放置符文槽子 Widget。
 * 只负责槽位的创建（BuildSlots）和图标刷新（RefreshSlots）。
 * 不持有任何游戏状态，由 BackpackScreenWidget 驱动。
 *
 * Blueprint 中命名此 Widget 实例为 "PendingGridWidget"（供父级 BindWidget）。
 * 该 Widget 自身的 Designer 里放一个 UniformGridPanel，命名为 "PendingRuneGrid"。
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UPendingGridWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Designer 中放 UniformGridPanel，命名 "PendingRuneGrid" */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> PendingRuneGrid;

    /** 包裹 PendingRuneGrid 的 SizeBox，命名 "PendingGridSizeBox"，BuildSlots 时动态设置精确尺寸保证格子 1:1 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> PendingGridSizeBox;

    /** 待放置区列数（2 列） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    int32 PendingGridCols = 2;

    /** 待放置区行数（4 行） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    int32 PendingGridRows = 4;

    /** 视觉风格配置 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TObjectPtr<UBackpackStyleDataAsset> StyleDA;

    /**
     * 动态创建槽位 Overlay（背景 Image + 图标 Image）。
     * 由 BackpackScreenWidget.NativeConstruct 调用一次。
     */
    void BuildSlots();

    /**
     * 刷新槽位图标。
     * BackpackScreenWidget.RefreshPendingRuneSlots 调用此函数。
     *
     * @param PendingRunes 当前待放置符文列表（按顺序填充槽位，多余槽位置灰）
     */
    void RefreshSlots(const TArray<FRuneInstance>& PendingRunes);

    /** 高亮手柄光标所在槽位；Index=-1 清除高亮 */
    void SetGamepadCursor(int32 Index);

    /**
     * 屏幕绝对坐标 → 槽位索引。失败返回 false。
     * BackpackScreenWidget 拖拽事件中调用。
     */
    bool GetSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

    /**
     * 屏幕绝对坐标 → 最近槽位索引（X/Y 钳制到面板内部，不会越界）。
     * 用于取回操作：即使落点在 Pending 面板之外，也能通过 Y 坐标估算目标行。
     * 面板未初始化时返回 false。
     */
    bool GetNearestSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

private:
    /** 槽位背景 UImage */
    UPROPERTY()
    TArray<TObjectPtr<UImage>> CachedPendingBGImages;

    /** 槽位图标 UImage */
    UPROPERTY()
    TArray<TObjectPtr<UImage>> CachedPendingIcons;
};
