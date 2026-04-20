#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RuneDataAsset.h"
#include "PendingGridWidget.generated.h"

class UUniformGridPanel;
class USizeBox;
class URuneSlotWidget;
class UBackpackStyleDataAsset;

/**
 * 左侧待放置符文槽子 Widget。
 * 使用 RuneSlotWidget 格子（与主背包视觉一致），支持自由放置和手柄导航。
 *
 * Designer 里放 UniformGridPanel，命名 "PendingRuneGrid"（BindWidget 必须）。
 * 外层 SizeBox 命名 "PendingGridSizeBox"（BindWidgetOptional，用于锁定像素尺寸）。
 */
UCLASS(Blueprintable, BlueprintType)
class DEVKIT_API UPendingGridWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Designer 中放 UniformGridPanel，命名 "PendingRuneGrid" */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> PendingRuneGrid;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<USizeBox> PendingGridSizeBox;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    int32 PendingGridCols = 2;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    int32 PendingGridRows = 4;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Style")
    TObjectPtr<UBackpackStyleDataAsset> StyleDA;

    /** 格子 Widget 类（填 WBP_RuneSlot，与主背包共用） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Slot")
    TSubclassOf<URuneSlotWidget> RuneSlotClass;

    /**
     * 创建格子（使用 RuneSlotWidget）。
     * 由 BackpackScreenWidget.NativeConstruct 调用一次。
     */
    void BuildSlots();

    /**
     * 刷新所有格子视觉。
     * @param Grid      平铺数组（Cols×Rows），无效 GUID = 空格
     * @param CursorIdx 手柄光标所在格（-1=无），显示选中边框
     * @param GrabbedIdx 当前抓取/拖拽源格（-1=无），图标半透明
     */
    void RefreshSlots(const TArray<FRuneInstance>& Grid,
                      int32 CursorIdx  = -1,
                      int32 GrabbedIdx = -1);

    /** 屏幕绝对坐标 → 格子索引。落点在面板外则返回 false。 */
    bool GetSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

    /** 屏幕绝对坐标 → 最近格子索引（坐标先 Clamp 到面板内）。 */
    bool GetNearestSlotAtScreenPos(const FVector2D& AbsPos, int32& OutIndex) const;

private:
    UPROPERTY()
    TArray<TObjectPtr<URuneSlotWidget>> CachedSlots;
};
