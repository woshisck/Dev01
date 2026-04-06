#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnHeatReachedZero.generated.h"

class UBackpackGridComponent;

/**
 * 事件节点：监听热度的零值边沿
 * OnReachedZero — 热度从 >0 跌落到 0（Phase>0 时）→ 连接 BFNode_PhaseDecayTimer.In
 * OnAboveZero   — 热度从 0 回升到 >0            → 连接 BFNode_PhaseDecayTimer.Cancel
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Heat Reached Zero", Category = "BuffFlow|Phase"))
class DEVKIT_API UBFNode_OnHeatReachedZero : public UBFNode_Base
{
    GENERATED_UCLASS_BODY()

protected:
    virtual void ExecuteInput(const FName& PinName) override;
    virtual void Cleanup() override;

private:
    void HandleHeatReachedZero();
    void HandleHeatAboveZero();
    TWeakObjectPtr<UBackpackGridComponent> BoundBGC;
};
