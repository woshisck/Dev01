#pragma once

#include "CoreMinimal.h"
#include "Nodes/FlowNode.h"
#include "LENode_Base.generated.h"

/**
 * 所有关卡事件 Flow 节点的基类。
 * 提供访问 PlayerController / GameMode / TutorialManager 的便捷方法。
 */
UCLASS(Abstract, NotBlueprintable)
class DEVKIT_API ULENode_Base : public UFlowNode
{
	GENERATED_UCLASS_BODY()

protected:
	APlayerController* GetPlayerController() const;
	class UTutorialManager* GetTutorialManager() const;
};
