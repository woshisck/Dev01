#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "SNode_Base.generated.h"

class UStoryEngineSubsystem;
class AStoryFlowProxy;

/**
 * 所有导演系统 Story FA 节点的基类。
 * 提供访问 StoryEngineSubsystem / PlayerController / StoryFlowProxy 的便捷方法。
 * 只在 UStoryFlowAsset 编辑器中可见（AllowedAssetClasses）。
 */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API USNode_Base : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	UStoryEngineSubsystem* GetStoryEngine() const;
	APlayerController* GetPlayerController() const;
	AStoryFlowProxy* GetStoryProxy() const;
};
