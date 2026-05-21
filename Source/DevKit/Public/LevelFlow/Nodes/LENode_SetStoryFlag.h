#pragma once

#include "CoreMinimal.h"
#include "LevelFlow/Nodes/LENode_Base.h"
#include "GameplayTagContainer.h"
#include "Story/StoryRuleTypes.h"
#include "LENode_SetStoryFlag.generated.h"

/**
 * 关卡事件节点：直接设置 StoryFlag，绕过规则链。
 * 适合在演出序列中确保性地写 Flag，不依赖规则条件。
 * In → 设置 Flag → Out（同帧继续）
 */
UCLASS(meta = (DisplayName = "Set Story Flag"))
class DEVKIT_API ULENode_SetStoryFlag : public ULENode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = "Story", meta = (Categories = "Story.Flag"))
	FGameplayTag FlagTag;

	UPROPERTY(EditAnywhere, Category = "Story")
	EStoryFlagScope Scope = EStoryFlagScope::Save;

	UPROPERTY(EditAnywhere, Category = "Story")
	bool bValue = true;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
