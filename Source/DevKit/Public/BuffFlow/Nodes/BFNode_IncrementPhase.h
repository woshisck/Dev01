#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_IncrementPhase.generated.h"

/**
 * 动作节点：将 BackpackGridComponent 的阶段 +1（最大 3）
 * 由 BFNode_OnPhaseUpReady 触发
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Increment Phase", Category = "BuffFlow|Phase"))
class DEVKIT_API UBFNode_IncrementPhase : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

protected:
    virtual void ExecuteInput(const FName& PinName) override;
};
