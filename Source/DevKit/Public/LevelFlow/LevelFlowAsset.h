#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.h"
#include "LevelFlowAsset.generated.h"

/**
 * 关卡事件 Flow Asset — 专用于关卡脚本事件，与 BuffFlow 符文系统完全独立。
 * 由 ALevelEventTrigger 或程序化触发，节点使用 ULENode_* 系列。
 */
UCLASS(BlueprintType, meta = (DisplayName = "Level Event Flow"))
class DEVKIT_API ULevelFlowAsset : public UFlowAsset
{
	GENERATED_BODY()
};
