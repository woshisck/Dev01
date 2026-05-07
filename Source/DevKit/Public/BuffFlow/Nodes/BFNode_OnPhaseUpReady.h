#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnPhaseUpReady.generated.h"

/**
 * 事件节点（已禁用）：BackpackGridComponent 已移除，此节点不再执行任何操作。
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Phase Up Ready", Category = "BuffFlow|Phase"))
class DEVKIT_API UBFNode_OnPhaseUpReady : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

protected:
    virtual void ExecuteInput(const FName& PinName) override;
    virtual void Cleanup() override;
};
