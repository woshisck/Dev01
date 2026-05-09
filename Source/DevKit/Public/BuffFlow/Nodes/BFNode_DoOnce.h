#pragma once

#include "CoreMinimal.h"
#include "BuffFlow/Nodes/BFNode_Base.h"
#include "BFNode_DoOnce.generated.h"

/**
 * Allows execution to pass through only once.
 * Subsequent triggers are ignored until the "Reset" input is called.
 * Mirrors Unreal's Blueprint "Do Once" node.
 */
UCLASS(NotBlueprintable, meta = (DisplayName = "Do Once", Category = "BuffFlow|FlowControl"))
class DEVKIT_API UBFNode_DoOnce : public UBFNode_Base
{
	GENERATED_UCLASS_BODY()

	// 初始关闭 — true = 节点从一开始就处于已触发状态，必须先 Reset 才能通过
	UPROPERTY(EditAnywhere, Category = "BuffFlow", meta = (DisplayName = "初始关闭"))
	bool bStartClosed = false;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
	virtual void Cleanup() override;

private:
	bool bHasTriggered = false;
};
