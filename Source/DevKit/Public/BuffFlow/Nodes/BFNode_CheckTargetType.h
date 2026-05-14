#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_CheckTargetType.generated.h"

/**
 * 目标判断节点：判断上次伤害目标是敌人还是自己
 * 通常接在"当造成伤害时"或"当受到伤害时"节点之后
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Check Target Type", Category = "BuffFlow|Condition"))
class DEVKIT_API UBFNode_CheckTargetType : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};