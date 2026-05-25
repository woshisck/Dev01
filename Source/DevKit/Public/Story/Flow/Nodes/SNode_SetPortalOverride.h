#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_SetPortalOverride.generated.h"

/**
 * 强制指定传送门目的地（按序号），或清除已有的强制覆盖。
 * 等价于 EStoryEncounterActionKind::SetPortalOverride 在 Story FA 中的节点形式。
 */
UCLASS(meta = (DisplayName = "Set Portal Override"))
class DEVKIT_API USNode_SetPortalOverride : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** true = 清除强制覆盖，恢复正常传送门逻辑（忽略 PortalIndex）。 */
	UPROPERTY(EditAnywhere, Category = "Portal")
	bool bClearOverride = false;

	/** 强制传送到的传送门序号（bClearOverride = false 时生效）。 */
	UPROPERTY(EditAnywhere, Category = "Portal", meta = (EditCondition = "!bClearOverride", ClampMin = "0"))
	int32 PortalIndex = 0;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
