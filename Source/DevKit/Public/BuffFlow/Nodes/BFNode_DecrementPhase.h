#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_DecrementPhase.generated.h"

/**
 * 动作节点：将 BackpackGridComponent 的阶段 -1（最小 0）
 * 由 BFNode_OnHeatReachedZero → Wait → 此节点 触发
 * 仅在当前阶段 > 0 时生效
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Decrement Phase", Category = "BuffFlow|Phase"))
class DEVKIT_API UBFNode_DecrementPhase : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

protected:
    virtual void ExecuteBuffFlowInput(const FName& PinName) override;
};
