#pragma once

#include "CoreMinimal.h"
#include "FlowAsset.h"
#include "StoryFlowAsset.generated.h"

/**
 * 导演系统（Story Director）专用 Flow Asset。
 * 与 ULevelFlowAsset / UYogRuneFlowAsset 平级，用于故事节点 NodeEventFlow 字段。
 * 节点使用 USNode_* 系列；LENode_* / BFNode_* 不在此 FA 中显示。
 */
UCLASS(BlueprintType, meta = (DisplayName = "Story Director Flow"))
class DEVKIT_API UStoryFlowAsset : public UFlowAsset
{
	GENERATED_BODY()
};
