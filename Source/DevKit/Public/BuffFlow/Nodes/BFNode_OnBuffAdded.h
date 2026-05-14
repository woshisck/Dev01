#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_OnBuffAdded.generated.h"

/**
 * 延时节点：当任意 BuffFlow 启动时触发（监听 BuffFlowComponent::OnBuffFlowStarted）
 * 可用于连锁 Buff 效果
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "On Buff Added", Category = "BuffFlow|Trigger"))
class DEVKIT_API UBFNode_OnBuffAdded : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

protected:
	virtual void ExecuteBuffFlowInput(const FName& PinName) override;
	virtual void Cleanup() override;

	UFUNCTION()
	void HandleBuffAdded(FGuid RuneGuid);

private:
	bool bBound = false;
};
