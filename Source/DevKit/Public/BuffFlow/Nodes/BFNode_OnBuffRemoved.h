#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnBuffRemoved.generated.h"

/**
 * 延时节点：当任意 BuffFlow 停止时触发（监听 BuffFlowComponent::OnBuffFlowStopped）
 * 可用于 Buff 结束时的清理效果
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Buff Removed", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_OnBuffRemoved : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleBuffRemoved(FGuid RuneGuid);

private:
	bool bBound = false;
};
