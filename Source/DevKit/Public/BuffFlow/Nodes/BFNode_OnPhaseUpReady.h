#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnPhaseUpReady.generated.h"

class UBackpackGridComponent;

/**
 * 事件节点：当满足升阶条件（热度达到上限 + LastHit）时触发
 * 绑定 BackpackGridComponent::OnPhaseUpReady 非动态委托
 * 触发后由 BFNode_IncrementPhase 执行实际升阶
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Phase Up Ready", Category = "BuffFlow|Phase"))
class DEVKIT_API UBFNode_OnPhaseUpReady : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

protected:
    virtual void ExecuteInput(const FName& PinName) override;
    virtual void Cleanup() override;

private:
    void HandlePhaseUpReady();
    TWeakObjectPtr<UBackpackGridComponent> BoundBGC;
};
