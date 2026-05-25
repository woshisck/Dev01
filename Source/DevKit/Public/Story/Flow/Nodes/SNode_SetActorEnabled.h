#pragma once

#include "CoreMinimal.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_SetActorEnabled.generated.h"

/**
 * 通过 Actor Name 或 Actor Tag 找到场景中的 Actor，设置其可见性和碰撞状态。
 * 等价于 EStoryEncounterActionKind::SetActorEnabled 在 Story FA 中的节点形式。
 */
UCLASS(meta = (DisplayName = "Set Actor Enabled"))
class DEVKIT_API USNode_SetActorEnabled : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 按 Actor 名称匹配（精确匹配 FName）。优先级低于 Tag，两者可同时填写。 */
	UPROPERTY(EditAnywhere, Category = "Actor")
	FName TargetActorName;

	/** 按 Actor Tag 匹配（Actor.Tags 数组中包含此 Tag 的所有 Actor）。 */
	UPROPERTY(EditAnywhere, Category = "Actor")
	FName TargetActorTag;

	/** true = 显示并启用碰撞；false = 隐藏并禁用碰撞。 */
	UPROPERTY(EditAnywhere, Category = "Actor")
	bool bEnabled = true;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
