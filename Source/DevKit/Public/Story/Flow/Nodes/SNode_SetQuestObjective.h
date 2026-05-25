#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Story/Flow/Nodes/SNode_Base.h"
#include "SNode_SetQuestObjective.generated.h"

/**
 * 新增或更新一条任务目标（写入存档，广播 OnQuestTaskChanged）。
 * 等价于 EStoryEncounterActionKind::SetQuestObjective 在 Story FA 中的节点形式。
 */
UCLASS(meta = (DisplayName = "Set Quest Objective"))
class DEVKIT_API USNode_SetQuestObjective : public USNode_Base
{
	GENERATED_UCLASS_BODY()

public:
	/** 任务 Tag（唯一标识，例如 Quest.FirstRun.FindWeapon）。 */
	UPROPERTY(EditAnywhere, Category = "Quest", meta = (Categories = "Quest"))
	FGameplayTag QuestTaskId;

	/** 显示给玩家的任务文本。 */
	UPROPERTY(EditAnywhere, Category = "Quest", meta = (MultiLine = true))
	FText DisplayText;

	/** 可选：任务来源 Tag（遗圣目录 / 黑夜少女 / 系统等区分用）。空 = 不指定来源。 */
	UPROPERTY(EditAnywhere, Category = "Quest", meta = (Categories = "Quest.Source"))
	FGameplayTag SourceTag;

	/** 可选：关联的 StoryFlag Tag（任务完成时自动检查此 Flag）。空 = 不关联。 */
	UPROPERTY(EditAnywhere, Category = "Quest")
	FGameplayTag RelatedFlagTag;

protected:
	virtual void ExecuteInput(const FName& PinName) override;
};
