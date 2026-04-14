#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Data/RuneDataAsset.h"
#include "RuneDragDropOperation.generated.h"

/**
 * 背包符文拖拽操作载体
 * 携带来源信息（格子坐标）和符文实例数据
 */
UCLASS()
class DEVKIT_API URuneDragDropOperation : public UDragDropOperation
{
    GENERATED_BODY()
public:
    /** 用户实际抓取的格子坐标 */
    int32 SrcCol = -1;
    int32 SrcRow = -1;

    /** 符文在背包中的原始 Pivot（多格形状时用于计算落点偏移） */
    FIntPoint SrcPivot = FIntPoint(-1, -1);

    /** 被拖拽的符文实例（含名称、图标等显示信息） */
    FRuneInstance DraggedRune;
};
