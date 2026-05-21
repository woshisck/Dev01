#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "GameplayTagContainer.h"
#include "LENode_BroadcastStoryEvent.generated.h"

/**
 * 关卡事件节点：向 StoryEngineSubsystem 广播一个 Story 事件，触发匹配规则。
 * In → 广播事件 → Out（同帧继续）
 */
UCLASS(meta = (DisplayName = "Broadcast Story Event"))
class DEVKIT_API ULENode_BroadcastStoryEvent : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Story", meta = (Categories = "Story.Event"))
	FGameplayTag EventTag;

	UPROPERTY(EditAnywhere, Category = "Story", meta = (Categories = "Story"))
	FGameplayTag ContextTag;

	UPROPERTY(EditAnywhere, Category = "Story", meta = (Categories = "Story.Area"))
	FGameplayTag AreaTag;

	UPROPERTY(EditAnywhere, Category = "Story", meta = (Categories = "Story.Item"))
	FGameplayTag ItemTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
